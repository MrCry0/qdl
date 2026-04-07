#!/bin/bash
# SPDX-License-Identifier: BSD-3-Clause

set -e

SCRIPT_PATH="$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"
TMPDIR=$(mktemp -d)
trap 'rm -rf "$TMPDIR"' EXIT

while [[ $# -gt 0 ]]; do
    case "$1" in
        --builddir)
            builddir="$2"
            shift 2
            ;;
        *)
            echo "Unknown option: $1" >&2
            exit 1
            ;;
    esac
done

cc -O2 -Wall -g \
	$(pkg-config --cflags libxml-2.0) \
	-o "${TMPDIR}/test_read_op_execute_open_fail" \
	"${SCRIPT_PATH}/test_read_op_execute_open_fail.c" \
	"${SCRIPT_PATH}/../read.c" \
	"${SCRIPT_PATH}/../util.c" \
	"${SCRIPT_PATH}/../ux.c" \
	"${SCRIPT_PATH}/../oscompat.c" \
	$(pkg-config --libs libxml-2.0)

"${TMPDIR}/test_read_op_execute_open_fail"
