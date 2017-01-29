#include <stdint.h>
#include <string.h>

#include <openssl/err.h> // err_load_crypto_strings
#include <openssl/evp.h> // openssl_add_all_algorithms
#include <openssl/rand.h> // RAND_bytes

#include "libomemo.h"


void omemo_default_crypto_init(void) {
  OpenSSL_add_all_algorithms();
  ERR_load_crypto_strings();
}

int omemo_default_crypto_random_bytes(uint8_t ** buf_pp, size_t buf_len, void * user_data_p) {
  (void) user_data_p;

  if (!buf_pp) {
    return OMEMO_ERR_NULL;
  }

  uint8_t * buf_p = malloc(sizeof(uint8_t) * buf_len);
  if (!buf_p) {
    return OMEMO_ERR_NOMEM;
  }

  if (!RAND_bytes(buf_p, buf_len)) {
    free (buf_p);
    return OMEMO_ERR_CRYPTO;
  }

  *buf_pp = buf_p;

  return 0;
}

int omemo_default_crypto_aes_gcm_encrypt( const uint8_t * plaintext_p, size_t plaintext_len,
                                          const uint8_t * iv_p, size_t iv_len,
                                          const uint8_t * key_p, size_t key_len,
                                          size_t tag_len,
                                          void * user_data_p,
                                          uint8_t ** ciphertext_pp, size_t * ciphertext_len_p,
                                          uint8_t ** tag_pp) {
  (void) user_data_p;

  if (!plaintext_p || !iv_p || !key_p || !ciphertext_pp || !ciphertext_len_p) {
    return OMEMO_ERR_NULL;
  }

  int ret_val = 0;

  const EVP_CIPHER * cipher_p = (void *) 0;
  EVP_CIPHER_CTX * cipher_ctx_p = (void *) 0;
  uint8_t * buf_p = (void *) 0;
  int out_len = 0;
  size_t ct_len = 0;
  uint8_t * ct_buf_p = (void *) 0;
  uint8_t * tag_p = (void *) 0;

  // as it is, OMEMO uses only aes-128
  switch(key_len) {
    case 16:
      cipher_p = EVP_aes_128_gcm();
      break;
    case 24:
      cipher_p = EVP_aes_192_gcm();
      break;
    case 32:
      cipher_p = EVP_aes_256_gcm();
      break;
    default:
      ret_val = OMEMO_ERR_CRYPTO;
      goto cleanup;
  }

  cipher_ctx_p = EVP_CIPHER_CTX_new();
  if (!cipher_ctx_p) {
    ret_val = OMEMO_ERR_CRYPTO;
    goto cleanup;
  }

  if (!EVP_EncryptInit_ex(cipher_ctx_p, cipher_p, (void *) 0, (void *) 0, (void *) 0)) {
    ret_val = OMEMO_ERR_CRYPTO;
    goto cleanup;
  }

  // right now, should be OMEMO_AES_GCM_IV_LENGTH
  if (!EVP_CIPHER_CTX_ctrl(cipher_ctx_p, EVP_CTRL_GCM_SET_IVLEN, iv_len, (void*) 0)) {
    ret_val = OMEMO_ERR_CRYPTO;
    goto cleanup;
  }

  if (!EVP_EncryptInit_ex(cipher_ctx_p, (void *) 0, (void *) 0, key_p, iv_p)) {
    ret_val = OMEMO_ERR_CRYPTO;
    goto cleanup;
  }

  (void) EVP_CIPHER_CTX_set_padding(cipher_ctx_p, 0);

  // since it's a stream cipher the ct length should be the same as the pt length, but who knows
  buf_p = malloc(sizeof(uint8_t) * (plaintext_len + EVP_MAX_BLOCK_LENGTH));
  if (!buf_p) {
    ret_val = OMEMO_ERR_NOMEM;
    goto cleanup;
  }

  if (!EVP_EncryptUpdate(cipher_ctx_p, buf_p, &out_len, plaintext_p, plaintext_len)) {
    ret_val = OMEMO_ERR_CRYPTO;
    goto cleanup;
  }

  ct_len = out_len;

  // again, since it's a stream cipher, there should be left nothing to write, but who knows
  if (!EVP_EncryptFinal_ex(cipher_ctx_p, buf_p + ct_len, &out_len)) {
    ret_val = OMEMO_ERR_CRYPTO;
    goto cleanup;
  }
  ct_len += out_len;

  // tag_len should be 16
  tag_p = malloc(sizeof(uint8_t) * tag_len);
  if (!tag_p) {
    ret_val = OMEMO_ERR_NOMEM;
    goto cleanup;
  }

  if (!EVP_CIPHER_CTX_ctrl(cipher_ctx_p, EVP_CTRL_GCM_GET_TAG, tag_len, tag_p)) {
    ret_val = OMEMO_ERR_CRYPTO;
    goto cleanup;
  }

  ct_buf_p = malloc(sizeof(uint8_t) * ct_len);
  if (!ct_buf_p) {
    ret_val = OMEMO_ERR_NOMEM;
    goto cleanup;
  }
  (void) memcpy(ct_buf_p, buf_p, ct_len);

  *ciphertext_pp = ct_buf_p;
  *ciphertext_len_p = ct_len;
  *tag_pp = tag_p;

cleanup:
  if (ret_val) {
    free(tag_p);
    free(ct_buf_p);
  }
  free(buf_p);
  EVP_CIPHER_CTX_free(cipher_ctx_p);
  return ret_val;
}

int omemo_default_crypto_aes_gcm_decrypt( const uint8_t * ciphertext_p, size_t ciphertext_len,
                                          const uint8_t * iv_p, size_t iv_len,
                                          const uint8_t * key_p, size_t key_len,
                                          uint8_t * tag_p, size_t tag_len,
                                          void * user_data_p,
                                          uint8_t ** plaintext_pp, size_t * plaintext_len_p) {
  (void) user_data_p;

  if (!ciphertext_p || !iv_p || !key_p || !tag_p || !plaintext_pp || !plaintext_len_p) {
    return OMEMO_ERR_NULL;
  }

  int ret_val = 0;

  const EVP_CIPHER * cipher_p = (void *) 0;
  EVP_CIPHER_CTX * cipher_ctx_p = (void *) 0;
  uint8_t * buf_p = (void *) 0;
  int out_len = 0;
  size_t pt_len = 0;
  uint8_t * pt_buf_p = (void *) 0;

  // as it is, OMEMO uses only aes-128
  switch(key_len) {
    case 16:
      cipher_p = EVP_aes_128_gcm();
      break;
    case 24:
      cipher_p = EVP_aes_192_gcm();
      break;
    case 32:
      cipher_p = EVP_aes_256_gcm();
      break;
    default:
      ret_val = OMEMO_ERR_CRYPTO;
      goto cleanup;
  }


  cipher_ctx_p = EVP_CIPHER_CTX_new();
  if (!cipher_ctx_p) {
    ret_val = OMEMO_ERR_CRYPTO;
    goto cleanup;
  }

  if (!EVP_DecryptInit_ex(cipher_ctx_p, cipher_p, (void *) 0, (void *) 0, (void *) 0)) {
    ret_val = OMEMO_ERR_CRYPTO;
    goto cleanup;
  }

  if (!EVP_CIPHER_CTX_ctrl(cipher_ctx_p, EVP_CTRL_GCM_SET_IVLEN, iv_len, (void *) 0)) {
    ret_val = OMEMO_ERR_CRYPTO;
    goto cleanup;
  }

  if (!EVP_DecryptInit_ex(cipher_ctx_p, NULL, NULL, key_p, iv_p)) {
    ret_val = OMEMO_ERR_CRYPTO;
    goto cleanup;
  }

  buf_p = malloc(sizeof(uint8_t) * (ciphertext_len + EVP_MAX_BLOCK_LENGTH));
  if (!buf_p) {
    ret_val = OMEMO_ERR_CRYPTO;
    goto cleanup;
  }

  if (!EVP_DecryptUpdate(cipher_ctx_p, buf_p, &out_len, ciphertext_p, ciphertext_len)) {
    ret_val = OMEMO_ERR_CRYPTO;
    goto cleanup;
  }
  pt_len = out_len;

  if (!EVP_CIPHER_CTX_ctrl(cipher_ctx_p, EVP_CTRL_GCM_SET_TAG, tag_len, tag_p)) {
    ret_val = OMEMO_ERR_CRYPTO;
    goto cleanup;
  }

  if (!EVP_DecryptFinal_ex(cipher_ctx_p, buf_p + pt_len, &out_len)) {
    ret_val = OMEMO_ERR_AUTH_FAIL;
    goto cleanup;
  }
  pt_len += out_len;

  pt_buf_p = malloc(sizeof(uint8_t) * pt_len);
  if (!pt_buf_p) {
    ret_val = OMEMO_ERR_NOMEM;
    goto cleanup;
  }
  (void) memcpy(pt_buf_p, buf_p, pt_len);

  *plaintext_pp = pt_buf_p;
  *plaintext_len_p = pt_len;

cleanup:
  if (ret_val) {
    free(pt_buf_p);
  }
  free(buf_p);
  EVP_CIPHER_CTX_free(cipher_ctx_p);

  return ret_val;
}

void omemo_default_crypto_teardown(void) {
  EVP_cleanup();
  ERR_free_strings();
}
