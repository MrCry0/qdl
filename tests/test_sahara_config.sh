#!/bin/bash
# SPDX-License-Identifier: BSD-3-Clause

set -e

SCRIPT_PATH="$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"
FLAT_BUILD=${SCRIPT_PATH}/data
REP_ROOT=${SCRIPT_PATH}/..
SAHARA_CFG=${FLAT_BUILD}/sahara_config_test.xml

cleanup() {
	rm -f "${SAHARA_CFG}"
}
trap cleanup EXIT

cd "${FLAT_BUILD}"

make_sahara_cfg() {
	cat > "${SAHARA_CFG}" <<EOF
<?xml version="1.0" ?>
<sahara_config>
    <images>
        <image image_id="13" image_path="$1" />
    </images>
</sahara_config>
EOF
}

expect_success() {
	local desc="$1"
	local image_path="$2"

	make_sahara_cfg "${image_path}"
	if ! ${REP_ROOT}/qdl --dry-run "${SAHARA_CFG}" rawprogram0.xml patch0.xml \
			2>/dev/null; then
		echo "${desc}: FAIL"
		exit 1
	fi
	echo "${desc}: OK"
}

expect_failure() {
	local desc="$1"
	local image_path="$2"

	make_sahara_cfg "${image_path}"
	if ${REP_ROOT}/qdl --dry-run "${SAHARA_CFG}" rawprogram0.xml patch0.xml \
			2>/dev/null; then
		echo "${desc}: FAIL (expected failure)"
		exit 1
	fi
	echo "${desc}: OK (correctly rejected)"
}

# Pure relative [A-Za-z]+: resolved relative to the XML's directory
expect_success  "pure relative"    "prog_firehose_ddr.elf"

# Relative with ../: navigates up from the XML's directory (tests/data/ -> tests/)
expect_success  "relative ../"     "../run_tests.sh"

# POSIX absolute: used as-is
expect_success  "absolute /"       "${FLAT_BUILD}/prog_firehose_ddr.elf"

# Windows paths: on Linux these are not recognised as absolute, so base_path
# is prepended and the resulting path does not exist — expected to fail
expect_failure  "Windows C:\\"     'C:\prog_firehose_ddr.elf'
expect_failure  "Windows \\\\"     '\\server\prog_firehose_ddr.elf'
expect_failure  "Windows ..\\"     '..\prog_firehose_ddr.elf'
