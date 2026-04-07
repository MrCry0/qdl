// SPDX-License-Identifier: BSD-3-Clause

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "../read.h"
#include "../qdl.h"

bool qdl_debug;

int gpt_find_by_name(struct qdl_device *qdl, const char *name,
		     int *partition, unsigned int *start_sector,
		     unsigned int *num_sectors)
{
	(void)qdl;
	(void)name;
	(void)partition;
	(void)start_sector;
	(void)num_sectors;
	return -1;
}

int read_cmd_add(const char *address, const char *filename);
int read_op_execute(struct qdl_device *qdl,
		    int (*apply)(struct qdl_device *qdl,
			       struct read_op *read_op, int fd));

static int apply_should_not_run(struct qdl_device *qdl,
			       struct read_op *read_op, int fd)
{
	(void)qdl;
	(void)read_op;
	(void)fd;
	assert(!"apply callback should not run when open() fails");
	return 0;
}

int main(void)
{
	int ret;
	char template[] = "/tmp/qdl-read-op-XXXXXX";
	char path[sizeof(template) + 16];
	char *tmpdir;

	tmpdir = mkdtemp(template);
	assert(tmpdir);

	snprintf(path, sizeof(path), "%s/out.bin/file.bin", tmpdir);
	ret = read_cmd_add("0/1+1", path);
	assert(ret == 0);

	ret = read_op_execute(NULL, apply_should_not_run);
	assert(ret == -ENOENT || ret == -ENOTDIR);

	rmdir(tmpdir);
	return 0;
}
