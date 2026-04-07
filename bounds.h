/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef __BOUNDS_H__
#define __BOUNDS_H__

#include <stdbool.h>
#include <stddef.h>

static inline bool qdl_bounds_contains(size_t total, size_t offset, size_t len)
{
	return offset <= total && len <= total - offset;
}

#endif
