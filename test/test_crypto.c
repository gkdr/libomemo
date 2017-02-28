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

void test_padding_offset(void ** state) {
  (void) state;

  unsigned int bucket[32];
  unsigned int offset, filled=0;

  memset((void*)&bucket[0],'\0',sizeof(unsigned int)*32);

  /* check that buckets are empty */
  for (unsigned int i = 0; i < 32; i++)
    assert_int_equal(bucket[i], 0);

  /* record some random offset values */
  for (unsigned int i = 0; i < 10000; i++)
    if (omemo_padding_random_offset(32, &offset) == 0) {
      assert_in_range(offset, 0, 32);
      bucket[offset]++;
    }

  /* check the range of offsets */
  for (unsigned int i = 0; i < 32; i++)
    if (bucket[i] > 0) filled++;
  assert_true(filled >= 30);
}


void test_padding_length(void ** state) {
  (void) state;

  size_t paddings[] = {
    0, MINIMUM_PADDED_LENGTH,
    31, MINIMUM_PADDED_LENGTH,
    256, MINIMUM_PADDED_LENGTH,
    257, MINIMUM_PADDED_LENGTH*2,
    512, MINIMUM_PADDED_LENGTH*2,
    513, MINIMUM_PADDED_LENGTH*4,
    1024, MINIMUM_PADDED_LENGTH*4,
    1025, MINIMUM_PADDED_LENGTH*8,
    0, 0
  };

  unsigned int i = 0;
  while (paddings[i+1] != 0) {
    assert_int_equal((int)omemo_padding_length(paddings[i], 2048),paddings[i+1]);
    i += 2;
  }
}

void test_padding_add(void ** state) {
  (void) state;

  char paddedtext[1024];
  char plaintext[1024];
  char * emptystring="";
  size_t paddedtext_len = 0;

  sprintf(plaintext,"%s","This is a test");

  assert_int_equal(strlen(plaintext), 14);
  assert_int_equal(omemo_padding_add((uint8_t *)plaintext, strlen(plaintext),
                                     (uint8_t *)paddedtext, 1024,
                                     &paddedtext_len), 0);
  assert_int_equal(paddedtext_len, MINIMUM_PADDED_LENGTH);
  assert_int_equal(omemo_padding_add((uint8_t *)emptystring, strlen(emptystring),
                                     (uint8_t *)paddedtext, 1024,
                                     &paddedtext_len), 1);

  /* make a longer plaintext string */
  memset((void*)plaintext,'a',270);
  plaintext[270] = 0;
  assert_int_equal(strlen(plaintext), 270);

  /* check that the padding length increases */
  assert_int_equal(omemo_padding_add((uint8_t *)plaintext, strlen(plaintext),
                                     (uint8_t *)paddedtext, 1024,
                                     &paddedtext_len), 0);
  assert_int_equal(paddedtext_len, MINIMUM_PADDED_LENGTH*2);
}

void test_padding_remove(void ** state) {
  (void) state;

  char paddedtext[1024];
  char unpaddedtext[1024];
  char * plaintext="This is a test";
  size_t paddedtext_len = 0;
  size_t unpaddedtext_len;

  assert_int_equal(omemo_padding_add((uint8_t *)plaintext, strlen(plaintext),
                                     (uint8_t *)paddedtext, 1024,
                                     &paddedtext_len), 0);
  assert_int_equal(paddedtext_len, MINIMUM_PADDED_LENGTH);

  omemo_padding_remove((uint8_t *)paddedtext, paddedtext_len,
                       (uint8_t *)unpaddedtext, &unpaddedtext_len);
  assert_int_equal(unpaddedtext_len, strlen(plaintext));
  assert_int_equal(strlen(plaintext), strlen(unpaddedtext));
  assert_int_equal(strcmp(plaintext, unpaddedtext), 0);
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

  ciphertext_p[0] = 0x00;

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
      cmocka_unit_test(test_padding_length),
      cmocka_unit_test(test_padding_add),
      cmocka_unit_test(test_padding_remove),
      cmocka_unit_test(test_padding_offset),
      cmocka_unit_test(test_aes_gcm_encrypt_decrypt)
  };

  return cmocka_run_group_tests_name("omemo default crypto", tests, openssl_init, openssl_teardown);
}
