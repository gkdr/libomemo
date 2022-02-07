#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include "../src/libomemo_crypto.c"

int openssl_init(void ** state) {
  (void) state;

  omemo_default_crypto_init();

  return 0;
}

int openssl_teardown(void ** state) {
  (void) state;

  omemo_default_crypto_teardown();

  return 0;
}

void test_random_bytes(void ** state) {
  (void) state;

  assert_int_not_equal(omemo_default_crypto_random_bytes((void *) 0, 0, (void *) 0), 0);

  uint8_t * buf = (void *) 0;
  assert_int_equal(omemo_default_crypto_random_bytes(&buf, 4, (void *) 0), 0);
  free(buf);
}

void test_aes_gcm_encrypt_decrypt(void ** state) {
  (void) state;
  char * msg = "hello aes";
  uint8_t * plaintext_p = (uint8_t *) msg;
  size_t plaintext_len = strlen(msg) + 1;

  size_t iv_len = OMEMO_AES_GCM_IV_LENGTH;
  uint8_t * iv_p = (void *) 0;
  assert_int_equal(omemo_default_crypto_random_bytes(&iv_p, iv_len, (void *) 0), 0);

  size_t key_len = OMEMO_AES_128_KEY_LENGTH;
  uint8_t * key_p = (void *) 0;
  assert_int_equal(omemo_default_crypto_random_bytes(&key_p, key_len, (void *) 0), 0);

  size_t tag_len = OMEMO_AES_GCM_TAG_LENGTH;

  uint8_t * ciphertext_p = (void *) 0;
  size_t ciphertext_len = 0;

  uint8_t * tag_p = (void *) 0;

  assert_int_equal(omemo_default_crypto_aes_gcm_encrypt(plaintext_p, plaintext_len,
                                                        iv_p, iv_len,
                                                        key_p, key_len,
                                                        tag_len,
                                                        (void *) 0,
                                                        &ciphertext_p, &ciphertext_len,
                                                        &tag_p), 0);


  uint8_t * result_p = (void *) 0;
  size_t result_len = 0;

  assert_int_equal(omemo_default_crypto_aes_gcm_decrypt(ciphertext_p, ciphertext_len,
                                                        iv_p, iv_len,
                                                        key_p, key_len,
                                                        tag_p, tag_len,
                                                        (void *) 0,
                                                        &result_p, &result_len), 0);

  assert_int_equal(plaintext_len, result_len);
  assert_memory_equal(plaintext_p, result_p, plaintext_len);

  // now change the ct so that the tag verification fails
  ciphertext_p[0] += 1;
  assert_int_equal(omemo_default_crypto_aes_gcm_decrypt(ciphertext_p, ciphertext_len,
                                                        iv_p, iv_len,
                                                        key_p, key_len,
                                                        tag_p, tag_len,
                                                        (void *) 0,
                                                        &result_p, &result_len), OMEMO_ERR_AUTH_FAIL);

  free(result_p);
  free(tag_p);
  free(ciphertext_p);
  free(key_p);
  free(iv_p);
}

int main(void) {
  const struct CMUnitTest tests[] = {
      cmocka_unit_test(test_random_bytes),
      cmocka_unit_test(test_aes_gcm_encrypt_decrypt)
  };

  return cmocka_run_group_tests_name("omemo default crypto", tests, openssl_init, openssl_teardown);
}
