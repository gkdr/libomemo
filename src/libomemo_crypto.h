/**
 * libomemo
 * Copyright (C) 2017
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#pragma once

#include <inttypes.h>

/**
 * If these default implementations are used, OpenSSL needs to be initialised.
 * In case it is used for providing axolotl crypto, the init functions must NOT be called.
 */
void omemo_default_crypto_init(void);

int omemo_default_crypto_random_bytes(uint8_t ** buf_pp, size_t buf_len, void * user_data_p);

int omemo_default_crypto_aes_gcm_encrypt( const uint8_t * plaintext_p, size_t plaintext_len,
                                          const uint8_t * iv_p, size_t iv_len,
                                          const uint8_t * key_p, size_t key_len,
                                          size_t tag_len,
                                          void * user_data_p,
                                          uint8_t ** ciphertext_pp, size_t * ciphertext_len_p,
                                          uint8_t ** tag_pp);

int omemo_default_crypto_aes_gcm_decrypt( const uint8_t * ciphertext_p, size_t ciphertext_len,
                                          const uint8_t * iv_p, size_t iv_len,
                                          const uint8_t * key_p, size_t key_len,
                                          uint8_t * tag_p, size_t tag_len,
                                          void * user_data_p,
                                          uint8_t ** plaintext_pp, size_t * plaintext_len_p);

void omemo_default_crypto_teardown(void);
