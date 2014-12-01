/**
 *  Copyright (c) 2014, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 */
#include "parser.h"

#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/syscall.h>  // Olga: to get thread id, temp

//#include "generic.h"
#include "mcrouter/lib/mc/ascii_client.h"
#include "mcrouter/lib/mc/umbrella_protocol.h"

static unsigned long _num_partial_messages = 0;
unsigned long mc_parser_num_partial_messages() {
  return _num_partial_messages;
}

void mc_parser_reset_num_partial_messages() {
  _num_partial_messages = 0;
}

pthread_mutex_t tbuf_queue_lock; // Overkill. Now it's all done on 1 thread. Olga
// uncontended mutex lock is unlocked in user space. Olga
#define TBUF_QUEUESIZE 1000
#define TBUF_PREALLOCATED_SIZE 1024

// todo: move to separate class, take void *, + init with function pointers(void *) to get/set size
typedef struct {
    tbuf_type* q[TBUF_QUEUESIZE+1];  /* body of queue */
    int first;              /* position of first element */
    int last;               /* position of last element */
    int count;              /* number of queue elements */
    int numCreatedBuffers;  /* total num new buffers created */
} queue_type;

queue_type *tbuf_queue = NULL;

void tbuf_release_buffer(tbuf_type *tbuf)
{
    if (!tbuf || tbuf->buf == NULL) {
        return;
    }

    pthread_mutex_lock(&tbuf_queue_lock);
    if (tbuf_queue->count >= TBUF_QUEUESIZE)
        // Can't happen, since we don't allow creation of > TBUF_QUEUESIZE
        printf("****Warning: queue overflow enqueue\n");
    else {
        //printf("*********** %ld: Returning buffer size = %i, ql: %i\n", (long) syscall(SYS_gettid), buf->size, tbuf_queue->count);
        tbuf_queue->last = (tbuf_queue->last+1) % TBUF_QUEUESIZE;
        tbuf_queue->q[ tbuf_queue->last ] = tbuf;
        tbuf_queue->count = tbuf_queue->count + 1;
    }
    pthread_mutex_unlock(&tbuf_queue_lock);
}

tbuf_type *tbuf_acquire_buffer(size_t len)
{
    tbuf_type *buf = NULL;

    // initialize object queue if first time
    if (!tbuf_queue) {
        tbuf_queue = (queue_type *) malloc(sizeof(queue_type));
        tbuf_queue->last = TBUF_QUEUESIZE - 1;
        tbuf_queue->count = 0;
        tbuf_queue->numCreatedBuffers = 0;
    }

    // access the queue
    pthread_mutex_lock(&tbuf_queue_lock);
    if (tbuf_queue->count <= 0) {
        // Done with queue
        pthread_mutex_unlock(&tbuf_queue_lock);
        if (tbuf_queue->numCreatedBuffers >= TBUF_QUEUESIZE) {
            //printf("*********** Max number of buffers created: %i\n", QUEUESIZE);
            pthread_mutex_unlock(&tbuf_queue_lock);
            return NULL;
        }
        // Don't enqueue, recursive lock, this buffer will be returned to queue when dereferenced.
        size_t nalloc = (len > TBUF_PREALLOCATED_SIZE)? len : TBUF_PREALLOCATED_SIZE;
        //("*********** %ld: Allocating new buffer size = %i, ql: %i\n", (long) syscall(SYS_gettid), nalloc, tbuf_queue->count);
        buf = malloc(nalloc);
        memset(buf, 0, sizeof(*buf));
        buf->size = nalloc;
        tbuf_queue->numCreatedBuffers++;
        return buf;
    } else {
        buf = tbuf_queue->q[ tbuf_queue->first ];
        //printf("*********** Re-using buffer size = %i, need %i, ql: %i\n", buf->_allocated_size, len, tbuf_queue->count);
        tbuf_queue->first = (tbuf_queue->first+1) % TBUF_QUEUESIZE;
        tbuf_queue->count = tbuf_queue->count - 1;
        // Done with queue
        pthread_mutex_unlock(&tbuf_queue_lock);

        // see if the buffer size needs to be increased.
        if (len > buf->size) {
            //printf("*********** %ld: Reallocating new buffer size = %i, need %i, ql: %i\n", (long) syscall(SYS_gettid), buf->size. len, tbuf_queue->count);
            buf = realloc(buf, len);
            memset(buf, 0, sizeof(*buf));
            buf->size = len;
        }
    }

    return buf;
}

void tbuf_delete_buffers()
{
    pthread_mutex_lock(&tbuf_queue_lock);
    while (tbuf_queue->count <= 0) {
        tbuf_type *buf = tbuf_queue->q[ tbuf_queue->first ];
        tbuf_queue->first = (tbuf_queue->first+1) % TBUF_QUEUESIZE;
        tbuf_queue->count = tbuf_queue->count - 1;
        free(buf);
    }
    pthread_mutex_unlock(&tbuf_queue_lock);
}

void mc_string_new(nstring_t *nstr, const char *buf, size_t len) {
  nstr->str = malloc(len + 1);
  FBI_ASSERT(nstr->str);
  strncpy(nstr->str, buf, len);
  nstr->str[len] = '\0';
  nstr->len = len;
}

void mc_parser_init(mc_parser_t *parser,
                    parser_type_t parser_type,
                    void (*msg_ready)(void *c, uint64_t id, mc_msg_t *req),
                    void (*parse_error)(void *context, parser_error_t error),
                    void *context) {
  FBI_ASSERT(msg_ready);
  FBI_ASSERT(parse_error);
  memset(parser, 0, sizeof(*parser));

  // initialize object queue if first time
  if (!tbuf_queue) {
      tbuf_queue = (queue_type *) malloc(sizeof(queue_type));
      tbuf_queue->last = TBUF_QUEUESIZE - 1;
      tbuf_queue->count = 0;
      tbuf_queue->numCreatedBuffers = 0;
  }

  switch (parser_type) {
    case reply_parser:
      parser->ragel_start_state = get_reply_ragel_start_state();
      break;

    case request_parser:
      parser->ragel_start_state = get_request_ragel_start_state();
      break;

    case request_reply_parser:
      parser->ragel_start_state = get_request_reply_ragel_start_state();
      break;
  }

  parser->parser_state = parser_idle;
  parser->msg_ready = msg_ready;
  parser->parse_error = parse_error;
  parser->context = context;
  parser->known_protocol = mc_unknown_protocol;

  parser->record_skip_key = false;
  parser->in_skipped_key = 0;
  parser->in_key = 0;
  parser->bad_key = 0;
  parser->tbuf = NULL;

  um_parser_init(&parser->um_parser);
}

/**
 * Ensure tbuf can hold n additional bytes, grow the buffer if needed in
 * a multiple of MC_TOKEN_INCREMENT as long as it doesn't exceed
 * MC_TOKEN_MAX_LEN
 * return 0 on success
 */
int mc_parser_ensure_tbuf(mc_parser_t *parser, int n) {
  FBI_ASSERT(parser);

  // tlen could be zero now
  FBI_ASSERT(parser->te >= parser->tbuf->buf);
  size_t tlen = parser->te - parser->tbuf->buf;
  size_t tbuf_len = tlen + n + 1;
  size_t padding = tbuf_len % MC_TOKEN_INCREMENT;
  if(padding != 0) {
    tbuf_len += (MC_TOKEN_INCREMENT - padding);
  }

  if(parser->tbuf->size < tbuf_len && tbuf_len <= MC_TOKEN_MAX_LEN) {
    parser->tbuf = tbuf_acquire_buffer(tbuf_len);
    //parser->tbuf = (char*) realloc(parser->tbuf, tbuf_len);
    parser->tbuf->size = tbuf_len;
    parser->te = parser->tbuf->buf + tlen;
  }
  int ret = 0;
  if(tbuf_len > MC_TOKEN_MAX_LEN) {
    ret = -1;
  }
  return ret;
}

void mc_parser_cleanup_tbuf(mc_parser_t *parser) {
  FBI_ASSERT(parser != NULL);
  tbuf_release_buffer(parser->tbuf);
  //free(parser->tbuf);
  parser->tbuf = NULL;
  parser->te = NULL;
}

mc_protocol_t mc_parser_determine_protocol(mc_parser_t *parser,
                                           uint8_t first_byte) {
  return (first_byte == ENTRY_LIST_MAGIC_BYTE)
    ? mc_umbrella_protocol
    : mc_ascii_protocol;
}

void mc_parser_parse(mc_parser_t *parser, const uint8_t *buf, size_t len) {
  FBI_ASSERT(len > 0);

  if (parser->known_protocol == mc_unknown_protocol) {
    parser->known_protocol =
      (buf[0] == ENTRY_LIST_MAGIC_BYTE)
      ? mc_umbrella_protocol
      : mc_ascii_protocol;
  }

  int success = 0;
  switch (parser->known_protocol) {
    case mc_ascii_protocol:
      if (parser->parser_state == parser_idle) {
        parser->parser_state = parser_msg_header;
      }
      success = (_on_ascii_rx(parser, (char*)buf, len) == 1);
      break;

    case mc_umbrella_protocol:
      success = !um_consume_buffer(&parser->um_parser, buf, len,
                                   parser->msg_ready, parser->context);
      break;

    default:
      FBI_ASSERT(!"parser->protocol was never set");
  }

  if (!success) {
    parser->parse_error(parser->context,
                        errno == ENOMEM ?
                        parser_out_of_memory :
                        parser_malformed_request);
  }
}

void mc_parser_reset(mc_parser_t *parser) {
  FBI_ASSERT(parser != NULL);
  if (parser->msg != NULL) {
    _num_partial_messages++; // An estimation is good enough here
    mc_msg_decref(parser->msg);
    parser->msg = NULL;
  }

  parser->parser_state = parser_idle;
  parser->off = 0;
  parser->resid = 0;
  parser->known_protocol = mc_unknown_protocol;

  um_parser_reset(&parser->um_parser);
}
