/**
 *  Copyright (c) 2014, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 */
#ifndef FB_MEMCACHE_MC_MSG_H
#define FB_MEMCACHE_MC_MSG_H

#include <inttypes.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdint.h>

#include "mcrouter/lib/fbi/decls.h"
#include "mcrouter/lib/fbi/nstring.h"

__BEGIN_DECLS

#define MSG_NOT_REFCOUNTED -12345
#define MEMCACHE_HASH_STOP "|#|"
#define SERVER_ERROR_BUSY 307

/*
 * memcache request op
 */

/** memcache request types */
typedef enum mc_op_e {
  mc_op_unknown = 0,
  mc_op_echo,
  mc_op_quit,
  mc_op_version,
  mc_op_servererr, ///< not a real op
  mc_op_get,
  mc_op_set,
  mc_op_add,
  mc_op_replace,
  mc_op_append,
  mc_op_prepend,
  mc_op_cas,
  mc_op_delete,
  mc_op_incr,
  mc_op_decr,
  mc_op_flushall,
  mc_op_flushre,
  mc_op_stats,
  mc_op_verbosity,
  mc_op_lease_get,
  mc_op_lease_set,
  mc_op_shutdown,
  mc_op_end,
  mc_op_metaget,
  mc_op_exec,
  mc_op_gets,
  mc_op_get_service_info, ///< Queries various service state
  mc_op_get_count,
  mc_op_bump_count,
  mc_op_get_unique_count,
  mc_op_bump_unique_count,
  mc_nops // placeholder
} mc_op_t;

mc_op_t mc_op_from_string(const char* str);
static inline const char* mc_op_to_string(const mc_op_t op) {
  static const char* const strings[] = {
    "unknown",
    "echo",
    "quit",
    "version",
    "servererr",
    "get",
    "set",
    "add",
    "replace",
    "append",
    "prepend",
    "cas",
    "delete",
    "incr",
    "decr",
    "flushall",
    "flushre",
    "stats",
    "verbosity",
    "lease-get",
    "lease-set",
    "shutdown",
    "end",
    "metaget",
    "exec",
    "gets",
    "get-service-info",
    "get-count",
    "bump-count",
    "get-unique-count",
    "bump-unique-count",
    };

  return strings[op < mc_nops ? op : mc_op_unknown];
}

/*
 * memcache reply result
 */

/** mcc response types. */
typedef enum mc_res_e {
  mc_res_unknown = 0,
  mc_res_deleted,
  mc_res_found,
  mc_res_foundstale, /* hot-miss w/ stale data */
  mc_res_notfound,
  mc_res_notfoundhot, /* hot-miss w/o stale data */
  mc_res_notstored,
  mc_res_stalestored,
  mc_res_ok,
  mc_res_stored,
  mc_res_exists,
  /* soft errors -- */
  mc_res_ooo, /* out of order (UDP) */
  mc_res_timeout, /* request timeout (connection was already established) */
  mc_res_connect_timeout,
  mc_res_connect_error,
  mc_res_busy, /* the request was refused for load shedding */
  mc_res_try_again, /* this request was refused, but we should keep sending
                       requests to this box */
  mc_res_tko, /* total knock out - the peer is down for the count */
  /* hard errors -- */
  mc_res_bad_command,
  mc_res_bad_key,
  mc_res_bad_flags,
  mc_res_bad_exptime,
  mc_res_bad_lease_id,
  mc_res_bad_cas_id,
  mc_res_bad_value,
  mc_res_aborted,
  mc_res_client_error,
  mc_res_local_error, /* an error internal to libmc */
 /* an error was reported by the remote server.

    TODO I think this can also be triggered by a communications problem or
    disconnect, but I think these should be separate errors. (fugalh) */
  mc_res_remote_error,
  /* in progress -- */
  mc_res_waiting,
  mc_nres // placeholder
} mc_res_t;

static inline const char* mc_res_to_string(const mc_res_t result) {
  static const char* const mc_res_strings[] = {
    "mc_res_unknown",
    "mc_res_deleted",
    "mc_res_found",
    "mc_res_foundstale",
    "mc_res_notfound",
    "mc_res_notfoundhot",
    "mc_res_notstored",
    "mc_res_stalestored",
    "mc_res_ok",
    "mc_res_stored",
    "mc_res_exists",
    /* soft errors -- */
    "mc_res_ooo",
    "mc_res_timeout",
    "mc_res_connect_timeout",
    "mc_res_connect_error",
    "mc_res_busy",
    "mc_res_try_again",
    "mc_res_tko",
    /* hard errors -- */
    "mc_res_bad_command",
    "mc_res_bad_key",
    "mc_res_bad_flags",
    "mc_res_bad_exptime",
    "mc_res_bad_lease_id",
    "mc_res_bad_cas_id",
    "mc_res_bad_value",
    "mc_res_aborted",
    "mc_res_client_error",
    "mc_res_local_error",
    "mc_res_remote_error",
    /* in progress -- */
    "mc_res_waiting"};
  return mc_res_strings[result < mc_nres ? result : mc_res_unknown];
}

enum mc_msg_flags_t {
    MC_MSG_FLAG_PHP_SERIALIZED = 0x1,
    MC_MSG_FLAG_COMPRESSED = 0x2,
    MC_MSG_FLAG_FB_SERIALIZED = 0x4,
    MC_MSG_FLAG_FB_COMPACT_SERIALIZED = 0x8,
    MC_MSG_FLAG_NZLIB_COMPRESSED = 0x800,
    MC_MSG_FLAG_QUICKLZ_COMPRESSED = 0x2000,
    MC_MSG_FLAG_SNAPPY_COMPRESSED = 0x4000,
    MC_MSG_FLAG_BIG_VALUE = 0X8000,
    MC_MSG_FLAG_NEGATIVE_CACHE = 0x10000,
    /* Bits reserved for application-specific extension flags: */
    MC_MSG_FLAG_USER_1 = 0x100000000LL,
    MC_MSG_FLAG_USER_2 = 0x200000000LL,
    MC_MSG_FLAG_USER_3 = 0x400000000LL,
    MC_MSG_FLAG_USER_4 = 0x800000000LL,
    MC_MSG_FLAG_USER_5 = 0x1000000000LL,
    MC_MSG_FLAG_USER_6 = 0x2000000000LL,
    MC_MSG_FLAG_USER_7 = 0x4000000000LL,
    MC_MSG_FLAG_USER_8 = 0x8000000000LL,
    MC_MSG_FLAG_USER_9 = 0x10000000000LL,
    MC_MSG_FLAG_USER_10 = 0x20000000000LL,
    MC_MSG_FLAG_USER_11 = 0x40000000000LL,
    MC_MSG_FLAG_USER_12 = 0x80000000000LL,
    MC_MSG_FLAG_USER_13 = 0x100000000000LL,
    MC_MSG_FLAG_USER_14 = 0x200000000000LL,
    MC_MSG_FLAG_USER_15 = 0x400000000000LL,
    MC_MSG_FLAG_USER_16 = 0x800000000000LL
};

/*
 * memcache request/reply.
 *
 * NOTE: update src/py/__init__.py when you update this struct!
 *
 */
typedef struct mc_msg_s {
  int _refcount;

  mc_op_t op;
  mc_res_t result;

  uint32_t err_code; ///< application specific error code

  uint64_t flags; // is_transient for metaget
  uint32_t exptime;
  uint32_t number; ///< flushall delay, verbosity, age in metaget

  uint64_t delta; ///< arithmetic
  uint64_t lease_id;
  uint64_t cas;

  nstring_t* stats; ///< array of 2*number nstrings for stats. freed for stats replies.
  nstring_t key; ///< get/set key, stats arg, flushre regexp
  nstring_t value; ///< storage value

  double lowval; ///< being used in counter ops
  double highval; ///< being used in counter ops

  struct in6_addr ip_addr; ///< metaget
  uint8_t ipv; ///< metaget

#ifndef LIBMC_FBTRACE_DISABLE
  /** NULL if not fbtracing. */
  struct mc_fbtrace_info_s* fbtrace_info;
#endif

  bool noreply;

  void *context; ///< get/set key, stats arg, flushre regexp
  size_t _extra_size;
  size_t _allocated_size;
} mc_msg_t;

/*
 * memcache request functions
 */

const char* mc_req_to_string(const mc_msg_t* req);

static inline int mc_req_has_key(const mc_msg_t* req) {
  switch (req->op) {
    case mc_op_get:
    case mc_op_set:
    case mc_op_add:
    case mc_op_replace:
    case mc_op_append:
    case mc_op_prepend:
    case mc_op_cas:
    case mc_op_delete:
    case mc_op_incr:
    case mc_op_decr:
    case mc_op_metaget:
    case mc_op_gets:
    case mc_op_get_count:
    case mc_op_bump_count:
    case mc_op_get_unique_count:
    case mc_op_bump_unique_count:
      return 1;

    default:
      return 0;
  }
}

/** Does request have a value (excluding fixed width integer values) */
static inline int mc_req_has_value(const mc_msg_t* req) {
  switch (req->op) {
    case mc_op_set:
    case mc_op_add:
    case mc_op_replace:
    case mc_op_append:
    case mc_op_lease_set:
    case mc_op_cas:
    case mc_op_bump_unique_count:
      return 1;

    default:
      return 0;
  }
}

/*
 * memcache reply functions
 */

void mc_msg_init_not_refcounted(mc_msg_t* msg);

/**
 * Construct a new request object. The memory for the object is allocated
 * on the heap. Optionally extra memory is allocated at the end of the request
 * structure. The reference count on the new structure is set to 1.
 *
 * @param extra_size Number of extra bytes to allocate.
 * @return Pointer to the newely allocated request structure or NULL on malloc
 *         failure.
 * @note The object returned by this function must be freed by calling
 *       mc_msg_decref()
 */
mc_msg_t* mc_msg_new(size_t extra_size);

/**
 * Construct a new request object with a copy of key embedded in it using
 * mc_msg_new.
 */
mc_msg_t *mc_msg_new_with_key(const char *key);

mc_msg_t *mc_msg_new_with_key_full(const char *key, size_t nkey);

/**
 * Construct a new request object with a copy of key and value
 * embedded in it using mc_msg_new.
 */
mc_msg_t *mc_msg_new_with_key_and_value(const char *key,
                                        const char *value,
                                        size_t nvalue);

mc_msg_t *mc_msg_new_with_key_and_value_full(const char *key,
                                             size_t nkey,
                                             const char *value,
                                             size_t nvalue);

void mc_msg_copy(mc_msg_t *dst, const mc_msg_t *src);
void mc_msg_shallow_copy(mc_msg_t *dst, const mc_msg_t *src);
mc_msg_t* mc_msg_dup(const mc_msg_t *msg);

/**
 * Contruct a copy of 'msg' with 'key_append' appended to the key. The new key
 * will always be embedded as if constructed via mc_msg_new_with_key().
 * Note: This function does not support the 'stats' field and will return NULL
 * if 'msg->stats' is not NULL.
 */
mc_msg_t* mc_msg_dup_append_key_full(const mc_msg_t *msg,
                                     const char* key_append,
                                     size_t nkey_append);

mc_msg_t* mc_msg_realloc(mc_msg_t *msg, size_t new_exta_size);
int mc_msg_grow(mc_msg_t **msg, size_t len, void **field_ptr);
mc_msg_t* mc_msg_incref(mc_msg_t*);
void mc_msg_decref(mc_msg_t*);
void mc_msg_compress(mc_msg_t **msgP);
int mc_msg_decompress(mc_msg_t **msgP);
void mc_msg_nzlib_compress(mc_msg_t **msgP);
int mc_msg_nzlib_decompress(mc_msg_t **msgP);

static inline int mc_res_is_err(const mc_res_t result) {
  return result >= mc_res_ooo &&
    result < mc_res_waiting;
}

/*
 * On by default
 *
 * Turn this off only if you're sure that *all* your mc_msg_t's are
 * referenced by a single thread throughout their lifetime
 * (i.e. you can have multiple threads, but don't pass mc_msg_t's around)
 *
 * In a dbg build, you won't see the performance benefits
 * of disabling these, because an atomic operation is used anyways
 * to help you make sure you know what you're doing.
 */
void mc_msg_use_atomic_refcounts(int enable);

/*
 * On by default. Disable if you want to save some atomic operations
 * and you don't care about this field.
 */
void mc_msg_track_num_outstanding(int enable);
uint64_t mc_msg_num_outstanding();

/* Return 1 if pointer p lies within mc_msg_t msg's body
 *        0 if pointer p to p + len does not overlap at all
 * assert that p <-> p + len does not partially overlap
 */
int mc_msg_contains(const mc_msg_t *msg, void *p, size_t n);

/** Returns true if req has no key, or if that key is valid */
int mc_client_req_is_valid(const mc_msg_t* req);

__END_DECLS

#endif
