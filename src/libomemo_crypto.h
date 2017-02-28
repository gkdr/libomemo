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

int omemo_padding_random_offset(unsigned int max_offset,
                                unsigned int * offset);

size_t omemo_padding_length(size_t plaintext_len, size_t paddedtext_max_len);

int omemo_padding_add(uint8_t * plaintext_p, size_t plaintext_len,
                      uint8_t * paddedtext_p, size_t paddedtext_max_len,
                      size_t * paddedtext_len);

void omemo_padding_remove(uint8_t * paddedtext_p, size_t paddedtext_len,
                          uint8_t * plaintext_p, size_t * plaintext_len);
