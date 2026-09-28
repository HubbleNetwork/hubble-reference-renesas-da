/*
 * Copyright (c) 2026 Hubble Network, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Mbed TLS configuration. The image runs from SysRAM, so code, constants and data
 * all come out of the same pool. Each size option below is chosen with that in mind.
 */

#ifndef _HUBBLE_MBEDTLS_CONFIG_H_
#define _HUBBLE_MBEDTLS_CONFIG_H_

#include <stddef.h>

#include "hubble_config.h"

#define MBEDTLS_AES_C
#define MBEDTLS_CIPHER_MODE_CTR
#define MBEDTLS_CIPHER_C
#define MBEDTLS_CMAC_C

/* CTR and CMAC both only ever encrypt: drops the inverse tables and aes_decrypt. */
#define MBEDTLS_BLOCK_CIPHER_NO_DECRYPT

/*
 * Const tables rather than tables generated into .bss on first use. Both land
 * in SysRAM, but this way there is no generator code and no init-done flag.
 */
#define MBEDTLS_AES_ROM_TABLES

/* Tradeoff between code size and speed. Equally secure with or without this set. */
#define MBEDTLS_AES_FEWER_TABLES

/* Don't needlessly add support for 256-bit AES. */
#if defined(CONFIG_HUBBLE_NETWORK_KEY_128) && !defined(CONFIG_HUBBLE_NETWORK_KEY_256)
#define MBEDTLS_AES_ONLY_128_BIT_KEY_LENGTH
#endif

/* Definitions needed to support calloc / free usage. Heap needed by CMAC. */
#define MBEDTLS_PLATFORM_C
#define MBEDTLS_PLATFORM_MEMORY
#define MBEDTLS_PLATFORM_CALLOC_MACRO hubble_mbedtls_calloc
#define MBEDTLS_PLATFORM_FREE_MACRO   hubble_mbedtls_free

void *hubble_mbedtls_calloc(size_t n, size_t size);
void hubble_mbedtls_free(void *ptr);

#endif /* _HUBBLE_MBEDTLS_CONFIG_H_ */
