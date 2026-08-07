// SPDX-License-Identifier: BSD-3-Clause
#define _FILE_OFFSET_BITS 64

#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include <cmocka.h>
#include <libxml/parser.h>
#include <libxml/tree.h>

#include "contents.h"
#include "file.h"
#include "qdl.h"

#define TEST_XMLDIR "/mock/xml"
#define TEST_CONFIG_FILE TEST_XMLDIR "/sahara_config.xml"
#define TEST_IMAGE_ID "13"

#ifdef _WIN32
const char *__progname = "test_sahara_config_xml";
#endif

static const char *existing_paths[3];

static void set_existing_path(const char *path)
{
	existing_paths[0] = path;
	existing_paths[1] = NULL;
}

static bool mock_path_exists(const char *path)
{
	for (size_t i = 0; existing_paths[i]; i++) {
		if (!strcmp(path, existing_paths[i]))
			return true;
	}

	return false;
}

int contents_resolve_path(struct contents_filter *filter, const char *filename, struct pathbuf *path)
{
	(void)filter;
	(void)filename;
	(void)path;

	return 0;
}

int qdl_file_open(struct qdl_zip *qzip, const char *filename, struct qdl_file *file)
{
	(void)qzip;

	if (!mock_path_exists(filename))
		return -1;

	file->type = QDL_FILE_TYPE_POSIX;
	file->size = 1;
	file->fd = -1;
	file->zip_file = NULL;
	return 0;
}

void qdl_file_close(struct qdl_file *file)
{
	file->type = QDL_FILE_TYPE_UNKNOWN;
}

void *qdl_file_load(struct qdl_file *file, size_t *len)
{
	(void)file;
	*len = 1;
	return malloc(1);
}

void ux_err(const char *fmt, ...)
{
	(void)fmt;
}

void ux_info(const char *fmt, ...)
{
	(void)fmt;
}

void ux_log(const char *fmt, ...)
{
	(void)fmt;
}

void ux_debug(const char *fmt, ...)
{
	(void)fmt;
}

/*
 * Runs decode_sahara_config() against a raw XML (or non-XML) blob. Frees
 * blob.ptr unconditionally: decode_sahara_config() consumes and NULLs it
 * out on every path that gets past the "<?xml" magic check, but leaves it
 * owned by the caller on early returns (non-XML blob, unparsable XML), so
 * an unconditional free() (a safe no-op on NULL) covers every case without
 * the test needing to track which path was taken.
 */
static int run_decode(const char *xml, struct sahara_image *images)
{
	struct sahara_image blob;
	int ret;

	blob.name = strdup(TEST_CONFIG_FILE);
	blob.ptr = strdup(xml);
	blob.len = strlen(xml);

	memset(images, 0, MAPPING_SZ * sizeof(*images));
	ret = decode_sahara_config(&blob, images, NULL);

	free(blob.name);
	free(blob.ptr);
	return ret;
}

static int decode_config(const char *image_path, struct sahara_image *images)
{
	char xml[512];
	int ret;

	ret = snprintf(xml, sizeof(xml),
		       "<?xml version=\"1.0\" ?>"
		       "<sahara_config>"
		       "<images>"
		       "<image image_id=\"" TEST_IMAGE_ID "\" image_path=\"%s\" />"
		       "</images>"
		       "</sahara_config>",
		       image_path);
	assert_true(ret > 0 && (size_t)ret < sizeof(xml));

	return run_decode(xml, images);
}

static void test_pure_relative_resolved_against_xml_dir(void **state)
{
	struct sahara_image images[MAPPING_SZ];
	int ret;

	(void)state;
	set_existing_path(TEST_XMLDIR "/prog_firehose_ddr.elf");

	ret = decode_config("prog_firehose_ddr.elf", images);
	if (ret != 1)
		fail_msg("pure relative image_path should be accepted, got %d", ret);
	if (strcmp(images[13].name, TEST_XMLDIR "/prog_firehose_ddr.elf"))
		fail_msg("expected image resolved against XML dir, got '%s'", images[13].name);

	sahara_images_free(images, MAPPING_SZ);
}

static void test_relative_dotdot_navigates_up_from_xml_dir(void **state)
{
	struct sahara_image images[MAPPING_SZ];
	int ret;

	(void)state;
	set_existing_path(TEST_XMLDIR "/../run_tests.sh");

	ret = decode_config("../run_tests.sh", images);
	if (ret != 1)
		fail_msg("relative ../ image_path should be accepted, got %d", ret);
	if (strcmp(images[13].name, TEST_XMLDIR "/../run_tests.sh"))
		fail_msg("expected image path kept relative to XML dir, got '%s'", images[13].name);

	sahara_images_free(images, MAPPING_SZ);
}

static void test_posix_absolute_path_used_as_is(void **state)
{
	struct sahara_image images[MAPPING_SZ];
	int ret;

	(void)state;
	set_existing_path("/opt/images/prog_firehose_ddr.elf");

	ret = decode_config("/opt/images/prog_firehose_ddr.elf", images);
	if (ret != 1)
		fail_msg("absolute image_path should be accepted, got %d", ret);
	if (strcmp(images[13].name, "/opt/images/prog_firehose_ddr.elf"))
		fail_msg("expected absolute image path used as-is, got '%s'", images[13].name);

	sahara_images_free(images, MAPPING_SZ);
}

/*
 * Windows-style paths are not recognised as absolute on POSIX (see
 * path_is_absolute() in oscompat.h), so they take the relative-to-XML-dir
 * branch instead. Nothing on the mock filesystem matches the resulting
 * path, so decoding must fail rather than silently misresolve.
 */
static void test_windows_backslash_drive_is_rejected(void **state)
{
	struct sahara_image images[MAPPING_SZ];
	int ret;

	(void)state;
	set_existing_path(NULL);

	ret = decode_config("C:\\prog_firehose_ddr.elf", images);
	if (ret != -1)
		fail_msg("Windows drive path should be rejected on POSIX, got %d", ret);
}

static void test_windows_unc_path_is_rejected(void **state)
{
	struct sahara_image images[MAPPING_SZ];
	int ret;

	(void)state;
	set_existing_path(NULL);

	ret = decode_config("\\\\server\\prog_firehose_ddr.elf", images);
	if (ret != -1)
		fail_msg("Windows UNC path should be rejected on POSIX, got %d", ret);
}

static void test_windows_backslash_dotdot_is_rejected(void **state)
{
	struct sahara_image images[MAPPING_SZ];
	int ret;

	(void)state;
	set_existing_path(NULL);

	ret = decode_config("..\\prog_firehose_ddr.elf", images);
	if (ret != -1)
		fail_msg("Windows backslash ../ should be rejected on POSIX, got %d", ret);
}

/*
 * A blob that does not start with "<?xml" is not a sahara_config archive
 * at all (it is presumably a plain Sahara image) - decode_sahara_config()
 * must say so (0) without touching the images array, rather than treating
 * it as a decode failure (-1).
 */
static void test_non_xml_blob_is_not_a_config(void **state)
{
	struct sahara_image images[MAPPING_SZ];
	int ret;

	(void)state;
	set_existing_path(NULL);

	ret = run_decode("this is a raw firehose image, not XML", images);
	if (ret != 0)
		fail_msg("non-XML blob should be reported as 'not a config', got %d", ret);
	if (images[13].name)
		fail_msg("images array should be untouched when blob is not a config");
}

static void test_unparsable_xml_is_rejected(void **state)
{
	struct sahara_image images[MAPPING_SZ];
	int ret;

	(void)state;
	set_existing_path(NULL);

	ret = run_decode("<?xml version=\"1.0\" ?><sahara_config><images>", images);
	if (ret != -1)
		fail_msg("unparsable XML should be rejected, got %d", ret);
}

static void test_wrong_root_element_is_rejected(void **state)
{
	struct sahara_image images[MAPPING_SZ];
	int ret;

	(void)state;
	set_existing_path(NULL);

	ret = run_decode("<?xml version=\"1.0\" ?><not_sahara_config/>", images);
	if (ret != -1)
		fail_msg("wrong root element should be rejected, got %d", ret);
}

static void test_missing_images_node_is_rejected(void **state)
{
	struct sahara_image images[MAPPING_SZ];
	int ret;

	(void)state;
	set_existing_path(NULL);

	ret = run_decode("<?xml version=\"1.0\" ?><sahara_config/>", images);
	if (ret != -1)
		fail_msg("sahara_config without an images node should be rejected, got %d", ret);
}

static void test_image_id_zero_is_rejected(void **state)
{
	struct sahara_image images[MAPPING_SZ];
	int ret;

	(void)state;
	set_existing_path(TEST_XMLDIR "/prog_firehose_ddr.elf");

	ret = run_decode("<?xml version=\"1.0\" ?><sahara_config><images>"
			  "<image image_id=\"0\" image_path=\"prog_firehose_ddr.elf\" />"
			  "</images></sahara_config>", images);
	if (ret != -1)
		fail_msg("image_id=0 should be rejected, got %d", ret);
}

static void test_image_id_out_of_range_is_rejected(void **state)
{
	struct sahara_image images[MAPPING_SZ];
	int ret;

	(void)state;
	set_existing_path(TEST_XMLDIR "/prog_firehose_ddr.elf");

	ret = run_decode("<?xml version=\"1.0\" ?><sahara_config><images>"
			  "<image image_id=\"128\" image_path=\"prog_firehose_ddr.elf\" />"
			  "</images></sahara_config>", images);
	if (ret != -1)
		fail_msg("image_id=MAPPING_SZ should be rejected, got %d", ret);
}

static void test_missing_image_path_is_rejected(void **state)
{
	struct sahara_image images[MAPPING_SZ];
	int ret;

	(void)state;
	set_existing_path(NULL);

	ret = run_decode("<?xml version=\"1.0\" ?><sahara_config><images>"
			  "<image image_id=\"1\" />"
			  "</images></sahara_config>", images);
	if (ret != -1)
		fail_msg("missing image_path attribute should be rejected, got %d", ret);
}

static void test_multiple_images_are_all_resolved(void **state)
{
	struct sahara_image images[MAPPING_SZ];
	int ret;

	(void)state;
	existing_paths[0] = TEST_XMLDIR "/first.elf";
	existing_paths[1] = "/opt/second.elf";
	existing_paths[2] = NULL;

	ret = run_decode("<?xml version=\"1.0\" ?><sahara_config><images>"
			  "<image image_id=\"1\" image_path=\"first.elf\" />"
			  "<image image_id=\"2\" image_path=\"/opt/second.elf\" />"
			  "</images></sahara_config>", images);
	if (ret != 1)
		fail_msg("multi-image config should be accepted, got %d", ret);
	if (strcmp(images[1].name, TEST_XMLDIR "/first.elf"))
		fail_msg("expected image 1 resolved against XML dir, got '%s'", images[1].name);
	if (strcmp(images[2].name, "/opt/second.elf"))
		fail_msg("expected image 2 used as-is, got '%s'", images[2].name);

	sahara_images_free(images, MAPPING_SZ);
}

int main(void)
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(test_pure_relative_resolved_against_xml_dir),
		cmocka_unit_test(test_relative_dotdot_navigates_up_from_xml_dir),
		cmocka_unit_test(test_posix_absolute_path_used_as_is),
		cmocka_unit_test(test_windows_backslash_drive_is_rejected),
		cmocka_unit_test(test_windows_unc_path_is_rejected),
		cmocka_unit_test(test_windows_backslash_dotdot_is_rejected),
		cmocka_unit_test(test_non_xml_blob_is_not_a_config),
		cmocka_unit_test(test_unparsable_xml_is_rejected),
		cmocka_unit_test(test_wrong_root_element_is_rejected),
		cmocka_unit_test(test_missing_images_node_is_rejected),
		cmocka_unit_test(test_image_id_zero_is_rejected),
		cmocka_unit_test(test_image_id_out_of_range_is_rejected),
		cmocka_unit_test(test_missing_image_path_is_rejected),
		cmocka_unit_test(test_multiple_images_are_all_resolved),
	};

	return cmocka_run_group_tests(tests, NULL, NULL);
}
