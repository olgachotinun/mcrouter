#!/usr/bin/env bash
#
# Description: this script builds mcrouter (and as part of build, folly and fbthrift)
#
# this script takes two parameters: 
# 1: folder where mcrouter will be build (and some intermediate components)
# 2: (optional) number of parallel processes to speedup build process (passed to make)
#
#

set -ex

[ -n "$1" ] || ( echo "Install dir missing"; exit 1 )

cd $(dirname $0)

./get_and_build_everything_centos.sh centos $1 $2
