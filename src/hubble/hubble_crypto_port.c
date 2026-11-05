/*
 * Copyright (c) 2026 Hubble Network, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Allocation and free functions for MbedTLS. */

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "ke_mem.h"

#include <mbedtls/platform.h>

void *hubble_mbedtls_calloc(size_t n, size_t size)
{
	void *ptr;

	if ((n == 0U) || (size == 0U) || (n > (UINT32_MAX / size))) {
		return NULL;
	}

	ptr = ke_malloc((uint32_t)(n * size), KE_MEM_NON_RETENTION);
	if (ptr != NULL) {
		memset(ptr, 0, n * size);
	}

	return ptr;
}

void hubble_mbedtls_free(void *ptr)
{
	if (ptr != NULL) {
		ke_free(ptr);
	}
}
