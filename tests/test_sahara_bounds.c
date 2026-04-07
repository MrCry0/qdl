// SPDX-License-Identifier: BSD-3-Clause

#include <assert.h>
#include <limits.h>
#include <stdint.h>

#include "../qdl.h"

int main(void)
{
	assert(qdl_bounds_contains(16, 0, 16));
	assert(qdl_bounds_contains(16, 8, 8));
	assert(!qdl_bounds_contains(16, 17, 1));
	assert(!qdl_bounds_contains(SIZE_MAX, SIZE_MAX - 7, 16));
	assert(qdl_bounds_contains(SIZE_MAX, SIZE_MAX - 7, 7));

	return 0;
}
