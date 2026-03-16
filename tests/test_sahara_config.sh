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

run_qdl_with_sahara_cfg() {
	${REP_ROOT}/qdl --dry-run "${SAHARA_CFG}" rawprogram0.xml patch0.xml
}

# Test 1: relative image_path — ELF is resolved relative to the XML directory
cat > "${SAHARA_CFG}" <<EOF
<?xml version="1.0" ?>
<sahara_config>
    <images>
        <image image_id="13" image_path="prog_firehose_ddr.elf" />
    </images>
</sahara_config>
EOF

echo "Testing sahara_config with relative image_path..."
run_qdl_with_sahara_cfg
echo "relative image_path: OK"

# Test 2: absolute image_path — regression test for absolute path handling
cat > "${SAHARA_CFG}" <<EOF
<?xml version="1.0" ?>
<sahara_config>
    <images>
        <image image_id="13" image_path="${FLAT_BUILD}/prog_firehose_ddr.elf" />
    </images>
</sahara_config>
EOF

echo "Testing sahara_config with absolute image_path..."
run_qdl_with_sahara_cfg
echo "absolute image_path: OK"
