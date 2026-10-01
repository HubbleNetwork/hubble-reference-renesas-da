/*
 * Copyright (c) 2026 Hubble Network, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * hubble/port/crypto.h on SDK6's own AES.
 *
 * The Renesas SDK supports only 128-bit AES. Its AES-CMAC is used as-is, but
 * there is no AES-CTR, so CTR is built here out of single-block ECB.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "aes_cbc.h"
#include "aes_cmac.h"

#include <hubble/port/crypto.h>

#if CONFIG_HUBBLE_KEY_SIZE != 16
#error "DA145xx crypto is 128-bit only: select CONFIG_HUBBLE_NETWORK_KEY_128"
#endif

void hubble_crypto_zeroize(void *buf, size_t len)
{
	volatile uint8_t *p = (volatile uint8_t *)buf;

	while (len--) {
		*p++ = 0;
	}
}

int hubble_crypto_init(void)
{
	/* The hardware AES comes up with the BLE core; CMAC keeps no state. */
	return 0;
}

int hubble_crypto_aes_ctr(const uint8_t key[CONFIG_HUBBLE_KEY_SIZE],
			  uint8_t nonce_counter[HUBBLE_NONCE_BUFFER_SIZE],
			  const uint8_t *data, size_t len, uint8_t *output)
{
	uint8_t stream_block[HUBBLE_AES_BLOCK_SIZE];
	int ret = 0;

	if (len == 0U) {
		return 0;
	}

	/*
	 * CTR is not a cipher in its own right -- it turns a block cipher into
	 * a stream cipher. Per 16-byte block: encrypt the counter to get key
	 * stream, XOR the data with it, then step the counter.
	 */
	for (size_t off = 0U; off < len; off += HUBBLE_AES_BLOCK_SIZE) {
		size_t chunk = len - off;
		uint8_t status;

		/* Short final chunk unless len is a multiple of the block. */
		if (chunk > HUBBLE_AES_BLOCK_SIZE) {
			chunk = HUBBLE_AES_BLOCK_SIZE;
		}

		/*
		 * Key stream for this block: AES-ECB(key, counter).
		 *
		 * SDK6 exposes no ECB entry point, but CBC over a single block
		 * with a NULL IV is exactly it -- aes_cbc_encrypt() skips the
		 * IV XOR when IV_128 is NULL, and its chaining loop never runs
		 * when block_count is 1. It is also the same composition
		 * aes_cmac_generate() uses, so the hardware's byte ordering is
		 * already accounted for.
		 */
		status = aes_cbc_encrypt(nonce_counter, 1U, stream_block, 1U,
					 key, CONFIG_HUBBLE_KEY_SIZE, NULL);
		if (status != AES_CBC_ERR_NO_ERR) {
			ret = -1;
			goto out;
		}

		/*
		 * XOR the key stream into the data. Encrypting and decrypting
		 * are the same operation;
		 */
		aes_array_xor(&data[off], stream_block, chunk, &output[off]);

		/*
		 * Step the counter for the next block: treat the whole 16-byte
		 * block as one big-endian integer and add 1, carrying from the
		 * last byte up.
		 */
		for (int i = HUBBLE_AES_BLOCK_SIZE - 1; i >= 0; i--) {
			nonce_counter[i]++;
			if (nonce_counter[i] != 0U) {
				break;
			}
		}
	}

out:
	/* Key stream plus ciphertext recovers plaintext; do not leave it. */
	hubble_crypto_zeroize(stream_block, sizeof(stream_block));

	return ret;
}

int hubble_crypto_cmac(const uint8_t key[CONFIG_HUBBLE_KEY_SIZE],
		       const uint8_t *data, size_t len,
		       uint8_t output[HUBBLE_AES_BLOCK_SIZE])
{
	uint8_t status;

	/*
	 * The real limit is tighter than the uint16_t length parameter.
	 * aes_cmac_generate() keeps the block count in a uint8_t, so
	 * anything past 255 blocks overflows and will result in an
	 * incorrect MAC.
	 */
	if (len > (255U * HUBBLE_AES_BLOCK_SIZE)) {
		return -1;
	}

	/*
	 * aes_cmac_generate() is CBC-MAC underneath and passes
	 * aes_cbc_encrypt()'s status through unchanged; it has no error enum
	 * of its own, so the CBC one is the right thing to compare against.
	 */
	status = aes_cmac_generate(data, (uint16_t)len, key, output,
				   HUBBLE_AES_BLOCK_SIZE);

	return (status == AES_CBC_ERR_NO_ERR) ? 0 : -1;
}
