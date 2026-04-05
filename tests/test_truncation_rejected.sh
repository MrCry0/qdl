#!/bin/bash
# SPDX-License-Identifier: BSD-3-Clause

set -e

SCRIPT_PATH="$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"
FLAT_BUILD=${SCRIPT_PATH}/data
REP_ROOT=${SCRIPT_PATH}/..
TMPDIR=$(mktemp -d)
trap 'rm -rf "$TMPDIR"' EXIT

PROGRAMMER=${FLAT_BUILD}/prog_firehose_ddr.elf
RAWPROGRAM_OVERFLOW=${TMPDIR}/rawprogram-overflow.xml
OUTFILE=${TMPDIR}/out.bin

cat > "${RAWPROGRAM_OVERFLOW}" <<'EOF'
<?xml version="1.0" ?>
<data>
  <program
    SECTOR_SIZE_IN_BYTES="512"
    file_sector_offset="0"
    filename="boot.img"
    label="boot"
    num_partition_sectors="4294967296"
    physical_partition_number="0"
    start_sector="1" />
</data>
EOF

"${FLAT_BUILD}/generate_flat_build.sh"

cd "${FLAT_BUILD}"

echo "Reject oversized XML sector count"
if "${REP_ROOT}/qdl" --dry-run "${PROGRAMMER}" "${RAWPROGRAM_OVERFLOW}"; then
	echo "qdl unexpectedly accepted oversized XML sector count"
	exit 1
fi

echo "Reject oversized CLI sector address"
if "${REP_ROOT}/qdl" --dry-run "${PROGRAMMER}" read 0/4294967296+1 "${OUTFILE}"; then
	echo "qdl unexpectedly accepted oversized CLI sector address"
	exit 1
fi

echo "Reject oversized CLI sector length"
if "${REP_ROOT}/qdl" --dry-run "${PROGRAMMER}" read 0/1+4294967296 "${OUTFILE}"; then
	echo "qdl unexpectedly accepted oversized CLI sector length"
	exit 1
fi
