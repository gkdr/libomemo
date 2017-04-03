#include <stdint.h>
#include <string.h>

#include <gcrypt.h>

#include "libomemo.h"

#define MINIMUM_PADDED_LENGTH   256
#define PADDING_CHARACTER       0x200B
#define PADDING_CHAR1           ((PADDING_CHARACTER>>12) | 0xE0)
#define PADDING_CHAR2           (((PADDING_CHARACTER>>6) & 0x3F) | 0x80)
#define PADDING_CHAR3           ((PADDING_CHARACTER & 0x3F) | 0x80)
#define PADDING_CHARACTER_BYTES 3

/* text below this length will be padded to a number of fixed lengths */
#define SHORT_MESSAGE_LENGTH  4096

int omemo_default_crypto_random_bytes(uint8_t ** buf_pp, size_t buf_len,
                                      void * user_data_p) {
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

/* returns a random offset value for the beginning of a padded message */
int omemo_padding_random_offset(unsigned int max_offset,
                                unsigned int * offset) {
  uint8_t * buf = (void *) 0;
  if (omemo_default_crypto_random_bytes(&buf, sizeof(unsigned int),
                                        (void *) 0) != 0)
    return OMEMO_ERR_NOMEM;

  *offset =
      (unsigned int)((buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3]) %
      (max_offset/PADDING_CHARACTER_BYTES);

  free(buf);
  return 0;
}

/* returns the padded length for the given string length */
size_t omemo_padding_length(size_t plaintext_len, size_t paddedtext_max_len) {
  size_t pad = MINIMUM_PADDED_LENGTH;

  while (plaintext_len > pad)
    pad *= 2;

  if (pad > paddedtext_max_len)
    pad = paddedtext_max_len;

  return pad;
}

/* Pad the text to a minimum of 255 characters, by adding random amount of
   spaces before and after plaintext. */
int omemo_padding_add(uint8_t * plaintext_p, size_t plaintext_len,
                      uint8_t * paddedtext_p, size_t paddedtext_max_len,
                      size_t * paddedtext_len) {
  unsigned int offset, i;
  size_t pad;

  /* empty string */
  if (plaintext_len == 0)
    return 1;

  /* maximum padded text length is too short */
  if (paddedtext_max_len < plaintext_len)
    return 2;

  /* get the padding length */
  pad = omemo_padding_length(plaintext_len, paddedtext_max_len);

  /* get a random offset for the beginning of the plaintext
     within the padded text */
  if (omemo_padding_random_offset(pad - plaintext_len - 1,
                                  &offset) != 0)
    return 3;

  /* pad the beginning with 3 byte utf-8 */
  for (i = 0; i < offset*PADDING_CHARACTER_BYTES;
       i += PADDING_CHARACTER_BYTES) {
    paddedtext_p[i] = PADDING_CHAR1;
    paddedtext_p[i+1] = PADDING_CHAR2;
    paddedtext_p[i+2] = PADDING_CHAR3;
  }

  /* copy the plaintext into the padded text at the given offset */
  memcpy((void*)&paddedtext_p[i], (void*)plaintext_p, plaintext_len);

  /* append spaces as a filler if needed, to make the remaining
     number of characters divisible by 3.
     This will be at most two characters. */
  i = offset*PADDING_CHARACTER_BYTES + plaintext_len;
  while ((pad - i) % 3 != 0)
      paddedtext_p[i++] = ' ';

  /* add 3 byte utf-8 padding after the end of the plaintext */
  while (i <= pad-PADDING_CHARACTER_BYTES) {
    paddedtext_p[i++] = PADDING_CHAR1;
    paddedtext_p[i++] = PADDING_CHAR2;
    paddedtext_p[i++] = PADDING_CHAR3;
  }

  /* unexpected string length */
  if (i != pad)
      return 4;

  /* add a string terminator */
  paddedtext_p[i] = 0;

  /* return the padded text length */
  *paddedtext_len = i;

  return 0;
}

void omemo_padding_remove(uint8_t * paddedtext_p, size_t paddedtext_len,
                          uint8_t * plaintext_p, size_t * plaintext_len) {
  unsigned int start_offset=0, end_offset, i;

  /* empty string */
  if (paddedtext_len == 0) {
    plaintext_p[0] = 0;
    *plaintext_len = 0;
    return;
  }

  /* find the start of the text */
  while (start_offset < paddedtext_len-PADDING_CHARACTER_BYTES) {
    if (!((paddedtext_p[start_offset] == PADDING_CHAR1) &&
          (paddedtext_p[start_offset+1] == PADDING_CHAR2) &&
          (paddedtext_p[start_offset+2] == PADDING_CHAR3)))
      break;
    start_offset+=PADDING_CHARACTER_BYTES;
  }

  /* find the end of the text */
  end_offset=(unsigned int)paddedtext_len-1;
  while (end_offset > start_offset+PADDING_CHARACTER_BYTES) {
    if (paddedtext_p[end_offset] != PADDING_CHAR3)
        break;
    if (paddedtext_p[end_offset-1] != PADDING_CHAR2)
        break;
    if (paddedtext_p[end_offset-2] != PADDING_CHAR1)
        break;
    end_offset-=PADDING_CHARACTER_BYTES;
  }
  while (end_offset > start_offset) {
    if ((paddedtext_p[end_offset] != ' ') &&
        (paddedtext_p[end_offset] != 0))
      break;
    end_offset--;
  }

  /* get the unpadded text */
  for (i = start_offset; i <= end_offset; i++)
    plaintext_p[i-start_offset] = paddedtext_p[i];
  plaintext_p[i - start_offset] = 0;

  /* return the unpadded length */
  *plaintext_len = (end_offset - start_offset) + 1;
}

void omemo_default_crypto_init(void) {
  (void) gcry_check_version((void *) 0);
  gcry_control(GCRYCTL_SUSPEND_SECMEM_WARN);
  gcry_control (GCRYCTL_INIT_SECMEM, 16384, 0);
  gcry_control (GCRYCTL_RESUME_SECMEM_WARN);
  gcry_control(GCRYCTL_USE_SECURE_RNDPOOL);
  gcry_control (GCRYCTL_INITIALIZATION_FINISHED, 0);

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

  int algo = 0;
  gcry_cipher_hd_t cipher_hd;
  uint8_t * out_p = (void *) 0;
  uint8_t * tag_p = (void *) 0;

  char * paddedtext_p = (void *) 0;
  size_t paddedtext_len = 0;

  /* for short messages pad to a few fixed lengths */
  if (plaintext_len < SHORT_MESSAGE_LENGTH) {
    paddedtext_p =
      (char *)malloc(sizeof(char)*omemo_padding_length(plaintext_len,
                                                       SHORT_MESSAGE_LENGTH));
    if (!paddedtext_p)
      goto cleanup;
    if (omemo_padding_add((uint8_t *) plaintext_p, plaintext_len,
                          (uint8_t *) paddedtext_p, SHORT_MESSAGE_LENGTH,
                          &paddedtext_len) != 0) {
      goto cleanup;
    }
    plaintext_p = (uint8_t *)paddedtext_p;
    plaintext_len = paddedtext_len;
  }

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
  if (paddedtext_p)
      free(paddedtext_p);
  if (ret_val) {
    free(out_p);
    free(tag_p);
  }
  gcry_cipher_close(cipher_hd);

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

  int algo = 0;
  gcry_cipher_hd_t cipher_hd;
  uint8_t * out_p = (void *) 0;
  uint8_t * unpaddedtext_p = (void *) 0;
  size_t unpaddedtext_len = 0;

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

  unpaddedtext_p =
    (uint8_t *)malloc(sizeof(uint8_t *)*ciphertext_len);
  if (!unpaddedtext_p) {
    ret_val = OMEMO_ERR_NOMEM;
    goto cleanup;
  }
  omemo_padding_remove(out_p, ciphertext_len,
                       unpaddedtext_p, &unpaddedtext_len);


  *plaintext_pp = unpaddedtext_p;
  *plaintext_len_p = unpaddedtext_len+1;

cleanup:
  if (ret_val || (unpaddedtext_len > 0))
    free(out_p);

  gcry_cipher_close(cipher_hd);

  return ret_val;
}


void omemo_default_crypto_teardown(void) {

}
