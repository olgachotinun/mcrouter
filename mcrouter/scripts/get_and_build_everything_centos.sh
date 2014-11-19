#!/usr/bin/env bash

set -ex

ORDER=$1

PKG_DIR=$2/pkgs
INSTALL_DIR=$2/install

MAKE_ARGS=$3

mkdir -p "$PKG_DIR" "$INSTALL_DIR"

cd $(dirname $0)

for script in $(ls "order_$ORDER/" | egrep '^[0-9]+_.*[^~]$' | sort -n); do
	"./order_$ORDER/$script" "$PKG_DIR" "$INSTALL_DIR" "$MAKE_ARGS"
done

printf "%s\n" "Mcrouter installed in $INSTALL_DIR/bin/mcrouter"
