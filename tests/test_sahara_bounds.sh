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

cc -O2 -Wall -g -o "${TMPDIR}/test_sahara_bounds" \
	"${SCRIPT_PATH}/test_sahara_bounds.c"

"${TMPDIR}/test_sahara_bounds"
