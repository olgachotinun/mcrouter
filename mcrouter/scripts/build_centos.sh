#!/usr/bin/env bash

set -ex

[ -n "$1" ] || ( echo "Install dir missing"; exit 1 )

cd $(dirname $0)

./get_and_build_everything_centos.sh centos $1 $2
