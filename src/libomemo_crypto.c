#include <stdint.h>
#include <string.h>

#include <gcrypt.h>

#include "libomemo.h"


void omemo_default_crypto_init(void) {
  (void) gcry_check_version((void *) 0);
  gcry_control(GCRYCTL_SUSPEND_SECMEM_WARN);
  gcry_control (GCRYCTL_INIT_SECMEM, 16384, 0);
  gcry_control (GCRYCTL_RESUME_SECMEM_WARN);
  gcry_control(GCRYCTL_USE_SECURE_RNDPOOL);
  gcry_control (GCRYCTL_INITIALIZATION_FINISHED, 0);

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

  gcry_randomize(buf_p, buf_len, GCRY_STRONG_RANDOM);

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
  int hd_is_init = 0;

  int algo = 0;
  gcry_cipher_hd_t cipher_hd;
  uint8_t * out_p = (void *) 0;
  uint8_t * tag_p = (void *) 0;

  switch(key_len) {
    case 16:
      algo = GCRY_CIPHER_AES128;
      break;
    case 24:
      algo = GCRY_CIPHER_AES192;
      break;
    case 32:
      algo = GCRY_CIPHER_AES256;
      break;
    default:
      ret_val = OMEMO_ERR_CRYPTO;
      goto cleanup;
  }

  ret_val = gcry_cipher_open(&cipher_hd, algo, GCRY_CIPHER_MODE_GCM, GCRY_CIPHER_SECURE);
  if (ret_val) {
    ret_val = -ret_val;
    goto cleanup;
  }
  hd_is_init = 1;

  ret_val = gcry_cipher_setkey(cipher_hd, key_p, key_len);
  if (ret_val) {
    ret_val = -ret_val;
    goto cleanup;
  }

  ret_val = gcry_cipher_setiv(cipher_hd, iv_p, iv_len);
  if (ret_val) {
    ret_val = -ret_val;
    goto cleanup;
  }

  out_p = malloc(sizeof(uint8_t) * plaintext_len);
  if (!out_p) {
    ret_val = OMEMO_ERR_NOMEM;
    goto cleanup;
  }

  ret_val = gcry_cipher_encrypt(cipher_hd, out_p, plaintext_len, plaintext_p, plaintext_len);
  if (ret_val) {
    ret_val = -ret_val;
    goto cleanup;
  }

  tag_p = malloc(sizeof(uint8_t) * tag_len);
  if (!tag_p) {
    ret_val = OMEMO_ERR_NOMEM;
    goto cleanup;
  }

  ret_val = gcry_cipher_gettag(cipher_hd, tag_p, tag_len);
  if (ret_val) {
    ret_val = -ret_val;
    goto cleanup;
  }

  *ciphertext_pp = out_p;
  *ciphertext_len_p = plaintext_len;
  *tag_pp = tag_p;

cleanup:
  if (ret_val) {
    free(out_p);
    free(tag_p);
  }
  if (hd_is_init) {
    gcry_cipher_close(cipher_hd);
  }

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
  int hd_is_init = 0;

  int algo = 0;
  gcry_cipher_hd_t cipher_hd;
  uint8_t * out_p = (void *) 0;

  switch(key_len) {
    case 16:
      algo = GCRY_CIPHER_AES128;
      break;
    case 24:
      algo = GCRY_CIPHER_AES192;
      break;
    case 32:
      algo = GCRY_CIPHER_AES256;
      break;
    default:
      ret_val = OMEMO_ERR_CRYPTO;
      goto cleanup;
  }

  ret_val = gcry_cipher_open(&cipher_hd, algo, GCRY_CIPHER_MODE_GCM, GCRY_CIPHER_SECURE);
  if (ret_val) {
    ret_val = -ret_val;
    goto cleanup;
  }
  hd_is_init = 1;

  ret_val = gcry_cipher_setkey(cipher_hd, key_p, key_len);
  if (ret_val) {
    ret_val = -ret_val;
    goto cleanup;
  }

  ret_val = gcry_cipher_setiv(cipher_hd, iv_p, iv_len);
  if (ret_val) {
    ret_val = -ret_val;
    goto cleanup;
  }

  out_p = malloc(sizeof(uint8_t) * ciphertext_len);
  if (!out_p) {
    ret_val = OMEMO_ERR_NOMEM;
    goto cleanup;
  }

  ret_val = gcry_cipher_decrypt(cipher_hd, out_p, ciphertext_len, ciphertext_p, ciphertext_len);
  if (ret_val) {
    ret_val = -ret_val;
    goto cleanup;
  }

  ret_val = gcry_cipher_checktag(cipher_hd, tag_p, tag_len);
  if (ret_val) {
    ret_val = OMEMO_ERR_AUTH_FAIL;
    goto cleanup;
  }

  *plaintext_pp = out_p;
  *plaintext_len_p = ciphertext_len;

cleanup:
  if (ret_val) {
    free(out_p);
  }

  if (hd_is_init) {
    gcry_cipher_close(cipher_hd);
  }

  return ret_val;
}


void omemo_default_crypto_teardown(void) {

}
