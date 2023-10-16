#include <inttypes.h>
#include <stdarg.h> // vsnprintf
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <glib.h> // b64, glist

#include <mxml.h>

#include "libomemo.h"

#define HINTS_XMLNS "urn:xmpp:hints"

#define OMEMO_NS_SEPARATOR "."
#define OMEMO_NS_SEPARATOR_FINAL ":"
#define OMEMO_NS "eu.siacs.conversations.axolotl"

#define PEP_NODE_NAME "node"
#define DEVICELIST_PEP_NAME "devicelist"
#define BUNDLE_PEP_NAME "bundles"

#define OMEMO_DEVICELIST_PEP_NODE OMEMO_NS OMEMO_NS_SEPARATOR DEVICELIST_PEP_NAME

#define MESSAGE_NODE_NAME "message"
#define BODY_NODE_NAME "body"
#define ENCRYPTED_NODE_NAME "encrypted"
#define HEADER_NODE_NAME "header"
#define IV_NODE_NAME "iv"
#define KEY_NODE_NAME "key"
#define PAYLOAD_NODE_NAME "payload"
#define STORE_NODE_NAME "store"

#define PUBLISH_NODE_NAME "publish"
#define ITEMS_NODE_NAME "items"
#define ITEM_NODE_NAME "item"
#define LIST_NODE_NAME "list"
#define BUNDLE_NODE_NAME "bundle"
#define SIGNED_PRE_KEY_NODE_NAME "signedPreKeyPublic"
#define SIGNATURE_NODE_NAME "signedPreKeySignature"
#define PREKEYS_NODE_NAME "prekeys"
#define PRE_KEY_NODE_NAME "preKeyPublic"
#define IDENTITY_KEY_NODE_NAME "identityKey"
#define DEVICE_NODE_NAME "device"

#define XMLNS_ATTR_NAME "xmlns"
#define MESSAGE_NODE_FROM_ATTR_NAME "from"
#define MESSAGE_NODE_TO_ATTR_NAME "to"
#define HEADER_NODE_SID_ATTR_NAME "sid"
#define KEY_NODE_RID_ATTR_NAME "rid"
#define KEY_NODE_PREKEY_ATTR_NAME "prekey"
#define KEY_NODE_PREKEY_ATTR_VAL_TRUE "true"
#define PUBLISH_NODE_NODE_ATTR_NAME "node"
#define SIGNED_PRE_KEY_NODE_ID_ATTR_NAME "signedPreKeyId"
#define PRE_KEY_NODE_ID_ATTR_NAME "preKeyId"
#define DEVICE_NODE_ID_ATTR_NAME "id"

#define OMEMO_DB_DEFAULT_FN "omemo.sqlite"

#define NO_OMEMO_MSG "You received an OMEMO encrypted message, but your client does not support it."

#define EME_NODE_NAME "encryption"
#define EME_XMLNS "urn:xmpp:eme:0"
#define EME_NAMESPACE_ATTR_NAME "namespace"
#define EME_NAME_ATTR_NAME "name"

struct omemo_bundle {
  char * device_id;
  mxml_node_t * signed_pk_node_p;
  mxml_node_t * signature_node_p;
  mxml_node_t * identity_key_node_p;
  mxml_node_t * pre_keys_node_p;
  size_t pre_keys_amount;
};

struct omemo_devicelist {
  char * from;
  GList * id_list_p;
  mxml_node_t * list_node_p;
};

struct omemo_message {
  mxml_node_t * message_node_p;
  mxml_node_t * header_node_p;
  mxml_node_t * payload_node_p;
  uint8_t * key_p;
  size_t key_len;
  uint8_t * iv_p;
  size_t iv_len;
  size_t tag_len; //tag is appended to key buf, i.e. tag_p = key_p + key_len
};

/**
 * Mostly helps dealing with the device ids that come as an int and have to be a string for XML.
 *
 * @param in The int to convert.
 * @param out Will be set to the number string, needs to be free()d.
 * @return Returns the length on success, and negative on error.
 */
static int int_to_string(uint32_t in, char ** out) {
  int len;
  size_t buf_len;
  char * int_string;

  len = snprintf((void *) 0, 0, "%i", in);
  if (len < 0) {
    return -1;
  }
  buf_len = len + 1;
  int_string = malloc(buf_len);
  if (!int_string) {
    return OMEMO_ERR_NOMEM;
  }
  memset(int_string, 0, buf_len);

  int result = snprintf(int_string, buf_len, "%i", in);
  if (result != len) {
    free(int_string);
    return -1;
  }

  *out = int_string;
  return len;
}

/**
 * Helps basic sanity checking of received XML.
 *
 * @param node_p Pointer to the current node.
 * @param next_node_func The function that returns the next node (e.g. child or sibling)
 * @param expected_name The name of the expected node.
 * @param next_node_pp Will be set to a pointer to the next node if it is the right one.
 * @return 0 on success, negative on error (specifically, OMEMO_ERR_MALFORMED_XML)
 */
static int expect_next_node(mxml_node_t * node_p, mxml_node_t * (*next_node_func)(mxml_node_t * node_p), char * expected_name, mxml_node_t ** next_node_pp) {
  mxml_node_t * next_node_p = next_node_func(node_p);
  if (!next_node_p) {
    return OMEMO_ERR_MALFORMED_XML;
  }

  const char * element_name = mxmlGetElement(next_node_p);
  if (!element_name) {
    return OMEMO_ERR_MALFORMED_XML;
  }

  if (strncmp(mxmlGetElement(next_node_p), expected_name, strlen(expected_name))) {
    return OMEMO_ERR_MALFORMED_XML;
  }
  *next_node_pp = next_node_p;
  return 0;
}

#define log_err(format, ...) \
  do { \
    if (getenv("LIBOMEMO_DEBUG")) { \
      fprintf(stderr, "libomemo - error in %s: ", __func__); \
      fprintf(stderr, format, __VA_ARGS__); \
      fprintf(stderr, "\n"); \
    } \
  } while(0)

int omemo_bundle_create(omemo_bundle ** bundle_pp) {
  omemo_bundle * bundle_p = malloc(sizeof(omemo_bundle));
  if (!bundle_p) {
    return OMEMO_ERR_NOMEM;
  }
  memset(bundle_p, 0, sizeof(omemo_bundle));

  *bundle_pp = bundle_p;

  return 0;
}

int omemo_bundle_set_device_id(omemo_bundle * bundle_p, uint32_t device_id) {
  char * id_string = (void *) 0;
  int ret = int_to_string(device_id, &id_string);
  if (ret <= 0) {
    return ret;
  }

  bundle_p->device_id = id_string;

  return OMEMO_SUCCESS;
}

uint32_t omemo_bundle_get_device_id(omemo_bundle * bundle_p) {
  return strtol(bundle_p->device_id, (void *) 0, 0);
}

int omemo_bundle_set_signed_pre_key(omemo_bundle * bundle_p, uint32_t pre_key_id, uint8_t * data_p, size_t data_len) {
  int ret_val = 0;

  mxml_node_t * signed_pre_key_node_p = (void *) 0;
  char * pre_key_id_string = (void *) 0;
  gchar * b64_string = (void *) 0;

  signed_pre_key_node_p = mxmlNewElement(MXML_NO_PARENT, SIGNED_PRE_KEY_NODE_NAME);
  if (int_to_string(pre_key_id, &pre_key_id_string) <= 0) {
    ret_val = -1;
    goto cleanup;
  }
  mxmlElementSetAttr(signed_pre_key_node_p, SIGNED_PRE_KEY_NODE_ID_ATTR_NAME, pre_key_id_string);

  b64_string = g_base64_encode(data_p, data_len);
  (void) mxmlNewOpaque(signed_pre_key_node_p, b64_string);

  bundle_p->signed_pk_node_p = signed_pre_key_node_p;

cleanup:
  if (ret_val < 0) {
    mxmlDelete(signed_pre_key_node_p);
  }
  g_free(b64_string);
  free(pre_key_id_string);

  return ret_val;
}

int omemo_bundle_get_signed_pre_key(omemo_bundle * bundle_p, uint32_t * pre_key_id_p, uint8_t ** data_pp, size_t * data_len_p) {
  int ret_val = 0;

  const char * b64_string = (void *) 0;
  guchar * data_p = (void *) 0;
  gsize len = 0;
  const char * pre_key_id_string = (void *) 0;

  if (!bundle_p || !bundle_p->signed_pk_node_p) {
    ret_val = OMEMO_ERR_NULL;
    goto cleanup;
  }

  b64_string = mxmlGetOpaque(bundle_p->signed_pk_node_p);
  if (!b64_string) {
    ret_val = OMEMO_ERR_MALFORMED_BUNDLE_NO_SPK_DATA;
    goto cleanup;
  }

  pre_key_id_string = mxmlElementGetAttr(bundle_p->signed_pk_node_p, SIGNED_PRE_KEY_NODE_ID_ATTR_NAME);
  if (!pre_key_id_string) {
    ret_val = OMEMO_ERR_MALFORMED_BUNDLE_NO_SPK_ID_ATTR;
    goto cleanup;
  }

  data_p = g_base64_decode(b64_string, &len);

  *pre_key_id_p = strtol(pre_key_id_string, (void *) 0, 0);
  *data_pp = data_p;
  *data_len_p = len;

cleanup:

  return ret_val;
}

int omemo_bundle_set_signature(omemo_bundle * bundle_p, uint8_t * data_p, size_t data_len) {
  mxml_node_t * signature_node_p = mxmlNewElement(MXML_NO_PARENT, SIGNATURE_NODE_NAME);

  gchar * b64_string = g_base64_encode(data_p, data_len);
  (void) mxmlNewOpaque(signature_node_p, b64_string);

  bundle_p->signature_node_p = signature_node_p;

  g_free(b64_string);

  return 0;
}

int omemo_bundle_get_signature(omemo_bundle * bundle_p, uint8_t ** data_pp, size_t * data_len_p) {
  int ret_val = 0;

  const char * b64_string = (void *) 0;
  guchar * data_p = (void *) 0;
  gsize len = 0;

  if (!bundle_p || !bundle_p->signature_node_p) {
    ret_val = OMEMO_ERR_NULL;
    goto cleanup;
  }

  b64_string = mxmlGetOpaque(bundle_p->signature_node_p);
  if (!b64_string) {
    ret_val = OMEMO_ERR_MALFORMED_BUNDLE_NO_SIG_DATA;
    goto cleanup;
  }

  data_p = g_base64_decode(b64_string, &len);

  *data_pp = data_p;
  *data_len_p = len;

cleanup:

  return ret_val;
}

int omemo_bundle_set_identity_key(omemo_bundle * bundle_p, uint8_t * data_p, size_t data_len) {
  mxml_node_t * identity_node_p = mxmlNewElement(MXML_NO_PARENT, IDENTITY_KEY_NODE_NAME);

  gchar * b64_string = g_base64_encode(data_p, data_len);
  (void) mxmlNewOpaque(identity_node_p, b64_string);

  bundle_p->identity_key_node_p = identity_node_p;

  g_free(b64_string);

  return 0;
}

int omemo_bundle_get_identity_key(omemo_bundle * bundle_p, uint8_t ** data_pp, size_t * data_len_p) {
  int ret_val = 0;

  const char * b64_string = (void *) 0;
  guchar * data_p = (void *) 0;
  gsize len = 0;

  if (!bundle_p || !bundle_p->identity_key_node_p) {
    ret_val = OMEMO_ERR_NULL;
    goto cleanup;
  }

  b64_string = mxmlGetOpaque(bundle_p->identity_key_node_p);
  if (!b64_string) {
    ret_val = OMEMO_ERR_MALFORMED_BUNDLE_NO_IK_DATA;
    goto cleanup;
  }

  data_p = g_base64_decode(b64_string, &len);

  *data_pp = data_p;
  *data_len_p = len;

cleanup:
  return ret_val;
}

int omemo_bundle_add_pre_key(omemo_bundle * bundle_p, uint32_t pre_key_id, uint8_t * data_p, size_t data_len) {
  int ret_val = 0;

  mxml_node_t * prekeys_node_p = (void *) 0;
  mxml_node_t * pre_key_node_p = (void *) 0;
  char * pre_key_id_string = (void *) 0;
  gchar * b64_string = (void *) 0;

  prekeys_node_p = bundle_p->pre_keys_node_p;
  if (!prekeys_node_p) {
    prekeys_node_p = mxmlNewElement(MXML_NO_PARENT, PREKEYS_NODE_NAME);
  }

  pre_key_node_p = mxmlNewElement(MXML_NO_PARENT, PRE_KEY_NODE_NAME);
  if (int_to_string(pre_key_id, &pre_key_id_string) <= 0) {
    ret_val = -1;
    goto cleanup;
  }
  mxmlElementSetAttr(pre_key_node_p, PRE_KEY_NODE_ID_ATTR_NAME, pre_key_id_string);

  b64_string = g_base64_encode(data_p, data_len);
  (void) mxmlNewOpaque(pre_key_node_p, b64_string);

  mxmlAdd(prekeys_node_p, MXML_ADD_AFTER, MXML_ADD_TO_PARENT, pre_key_node_p);

  bundle_p->pre_keys_node_p = prekeys_node_p;
  bundle_p->pre_keys_amount++;

cleanup:
  if (ret_val < 0) {
    mxmlDelete(pre_key_node_p);
  }
  g_free(b64_string);
  free(pre_key_id_string);

  return ret_val;
}


int omemo_bundle_get_random_pre_key(omemo_bundle * bundle_p, uint32_t * pre_key_id_p, uint8_t ** data_pp, size_t * data_len_p) {
  int ret_val = 0;

  mxml_node_t * pre_key_node_p = (void *) 0;
  gint32 random = 0;
  mxml_node_t * next_p = (void *) 0;
  const char * b64_string = (void *) 0;
  guchar * data_p = (void *) 0;
  gsize len = 0;
  const char * pre_key_id_string = (void *) 0;

  if (!bundle_p || !bundle_p->pre_keys_node_p) {
    ret_val = OMEMO_ERR_NULL;
    goto cleanup;
  }

  ret_val = expect_next_node(bundle_p->pre_keys_node_p, mxmlGetFirstChild, PRE_KEY_NODE_NAME, &pre_key_node_p);
  if (ret_val) {
    goto cleanup;
  }

  random = g_random_int_range(0, bundle_p->pre_keys_amount);
  next_p = pre_key_node_p;
  for (int i = 0; i < random; i++) {
    next_p = mxmlGetNextSibling(next_p);
    if (!next_p) {
      log_err("failed to forward pointer to desired item %d out of %zu items at index %d", random, bundle_p->pre_keys_amount, i);
      ret_val = OMEMO_ERR_MALFORMED_BUNDLE;
      goto cleanup;
    }
  }

  pre_key_id_string = mxmlElementGetAttr(next_p, PRE_KEY_NODE_ID_ATTR_NAME);
  if (!pre_key_id_string) {
    ret_val = OMEMO_ERR_MALFORMED_BUNDLE_NO_PREKEY_ID_ATTR;
    goto cleanup;
  }

  b64_string = mxmlGetOpaque(next_p);
  if (!b64_string) {
    ret_val = OMEMO_ERR_MALFORMED_BUNDLE_NO_PREKEY_DATA;
    goto cleanup;
  }

  data_p = g_base64_decode(b64_string, &len);

  *pre_key_id_p = strtol(pre_key_id_string, (void *) 0, 0);
  *data_pp = data_p;
  *data_len_p = len;

cleanup:

  return ret_val;
}

int omemo_bundle_export(omemo_bundle * bundle_p, char ** publish) {
  int ret_val = 0;

  char * node_value = (void *) 0;
  const char * format = "%s%s%s%s%s";
  size_t len = 0;
  mxml_node_t * publish_node_p = (void *) 0;
  mxml_node_t * item_node_p = (void *) 0;
  mxml_node_t * bundle_node_p = (void *) 0;
  char * out = (void *) 0;

  if (!bundle_p->device_id || !bundle_p->signed_pk_node_p || !bundle_p->signature_node_p || !bundle_p->identity_key_node_p || !bundle_p->pre_keys_node_p) {
    ret_val = -1;
    goto cleanup;
  }
  if (bundle_p->pre_keys_amount < 20) {
    ret_val = -2;
    goto cleanup;
  }
  if (bundle_p->pre_keys_amount < 100) {
    //TODO: maybe issue warning
    //FIXME: numbers into constants
  }

  len = snprintf((void *) 0, 0, format, OMEMO_NS, OMEMO_NS_SEPARATOR, BUNDLE_PEP_NAME, OMEMO_NS_SEPARATOR_FINAL, bundle_p->device_id) + 1;
  node_value = malloc(len);
  if (snprintf(node_value, len, format, OMEMO_NS, OMEMO_NS_SEPARATOR, BUNDLE_PEP_NAME, OMEMO_NS_SEPARATOR_FINAL, bundle_p->device_id) <= 0) {
    ret_val = -4;
    goto cleanup;
  }

  publish_node_p = mxmlNewElement(MXML_NO_PARENT, PUBLISH_NODE_NAME);
  mxmlElementSetAttr(publish_node_p, PUBLISH_NODE_NODE_ATTR_NAME, node_value);

  item_node_p = mxmlNewElement(publish_node_p, ITEM_NODE_NAME);

  bundle_node_p = mxmlNewElement(item_node_p, BUNDLE_NODE_NAME);
  mxmlElementSetAttr(bundle_node_p, "xmlns", OMEMO_NS);

  mxmlAdd(bundle_node_p, MXML_ADD_AFTER, MXML_ADD_TO_PARENT, bundle_p->signed_pk_node_p);
  mxmlAdd(bundle_node_p, MXML_ADD_AFTER, MXML_ADD_TO_PARENT, bundle_p->signature_node_p);
  mxmlAdd(bundle_node_p, MXML_ADD_AFTER, MXML_ADD_TO_PARENT, bundle_p->identity_key_node_p);
  mxmlAdd(bundle_node_p, MXML_ADD_AFTER, MXML_ADD_TO_PARENT, bundle_p->pre_keys_node_p);

  out = mxmlSaveAllocString(publish_node_p, MXML_NO_CALLBACK);
  if (!out) {
    ret_val = -5;
    goto cleanup;
  }

  *publish = out;

cleanup:
  free(node_value);

  return ret_val;
}

int omemo_bundle_import (const char * received_bundle, omemo_bundle ** bundle_pp) {
  int ret_val = 0;

  omemo_bundle * bundle_p = (void *) 0;
  mxml_node_t * items_node_p = (void *) 0;
  mxml_node_t * item_node_p = (void *) 0;
  mxml_node_t * bundle_node_p = (void *) 0;
  const char * bundle_node_name = (void *) 0;
  char ** split = (void *) 0;
  char * device_id = (void *) 0;
  mxml_node_t * signed_pk_node_p = (void *) 0;
  mxml_node_t * signature_node_p = (void *) 0;
  mxml_node_t * identity_key_node_p = (void *) 0;
  mxml_node_t * prekeys_node_p = (void *) 0;
  mxml_node_t * pre_key_node_p = (void *) 0;
  size_t pre_keys_count = 0;

  ret_val = omemo_bundle_create(&bundle_p);
  if (ret_val) {
    goto cleanup;
  }

  items_node_p = mxmlLoadString((void *) 0, received_bundle, MXML_OPAQUE_CALLBACK);
  if (!items_node_p) {
    log_err("received bundle response is invalid XML: %s", received_bundle);
    ret_val = OMEMO_ERR_MALFORMED_XML;
    goto cleanup;
  }

  ret_val = strncmp(mxmlGetElement(items_node_p), ITEMS_NODE_NAME, strlen(ITEMS_NODE_NAME));
  if (ret_val) {
    ret_val = OMEMO_ERR_MALFORMED_BUNDLE_NO_ITEMS_ELEM;
    goto cleanup;
  }

  bundle_node_name = mxmlElementGetAttr(items_node_p, PEP_NODE_NAME);
  if (!bundle_node_name) {
    ret_val = OMEMO_ERR_MALFORMED_BUNDLE_NO_NODE_ATTR;
    goto cleanup;
  }

  split = g_strsplit(bundle_node_name, OMEMO_NS_SEPARATOR_FINAL, 6);
  if (!g_strcmp0(OMEMO_NS_SEPARATOR, OMEMO_NS_SEPARATOR_FINAL)) {
    device_id = g_strdup(split[5]);
  } else {
    device_id = g_strdup(split[1]);
  }
  bundle_p->device_id = device_id;

  item_node_p = mxmlFindPath(items_node_p, ITEM_NODE_NAME);
  if (!item_node_p) {
    ret_val = OMEMO_ERR_MALFORMED_BUNDLE_NO_ITEM_ELEM;
    goto cleanup;
  }

  bundle_node_p = mxmlFindPath(item_node_p, BUNDLE_NODE_NAME);
  if (!bundle_node_p) {
    ret_val = OMEMO_ERR_MALFORMED_BUNDLE_NO_BUNDLE_ELEM;
    goto cleanup;
  }

  signed_pk_node_p = mxmlFindPath(bundle_node_p, SIGNED_PRE_KEY_NODE_NAME);
  if (!signed_pk_node_p) {
    ret_val = OMEMO_ERR_MALFORMED_BUNDLE_NO_SPK_ELEM;
    goto cleanup;
  }
  signed_pk_node_p = mxmlGetParent(signed_pk_node_p);
  bundle_p->signed_pk_node_p = signed_pk_node_p;

  signature_node_p = mxmlFindPath(bundle_node_p, SIGNATURE_NODE_NAME);
  if (!signature_node_p) {
    ret_val = OMEMO_ERR_MALFORMED_BUNDLE_NO_SIG_ELEM;
    goto cleanup;
  }
  signature_node_p = mxmlGetParent(signature_node_p);
  bundle_p->signature_node_p = signature_node_p;

  identity_key_node_p = mxmlFindPath(bundle_node_p, IDENTITY_KEY_NODE_NAME);
  if (!identity_key_node_p) {
    ret_val = OMEMO_ERR_MALFORMED_BUNDLE_NO_IK_ELEM;
    goto cleanup;
  }
  identity_key_node_p = mxmlGetParent(identity_key_node_p);
  bundle_p->identity_key_node_p = identity_key_node_p;

  prekeys_node_p = mxmlFindPath(bundle_node_p, PREKEYS_NODE_NAME);
  if (!prekeys_node_p) {
    ret_val = OMEMO_ERR_MALFORMED_BUNDLE_NO_PREKEYS_ELEM;
    goto cleanup;
  }
  bundle_p->pre_keys_node_p = prekeys_node_p;

  pre_key_node_p = mxmlFindPath(prekeys_node_p, PRE_KEY_NODE_NAME);
  if (!pre_key_node_p) {
    ret_val = OMEMO_ERR_MALFORMED_BUNDLE_NO_PREKEY_ELEM;
    goto cleanup;
  }
  pre_key_node_p = mxmlGetParent(pre_key_node_p);
  pre_keys_count++;
  pre_key_node_p = mxmlGetNextSibling(pre_key_node_p);

  while (pre_key_node_p) {
    pre_keys_count++;
    pre_key_node_p = mxmlGetNextSibling(pre_key_node_p);
  }
  bundle_p->pre_keys_amount = pre_keys_count;

  mxmlRemove(signed_pk_node_p);
  mxmlRemove(signature_node_p);
  mxmlRemove(identity_key_node_p);
  mxmlRemove(prekeys_node_p);

  *bundle_pp = bundle_p;

cleanup:
  if (ret_val) {
    omemo_bundle_destroy(bundle_p);
  }
  mxmlDelete(items_node_p);
  g_strfreev(split);

  return ret_val;
}

int omemo_bundle_get_pep_node_name(uint32_t device_id, char ** node_name_p) {
  const char * format = "%s%s%s%s%i";
  size_t len = snprintf((void *) 0, 0, format, OMEMO_NS, OMEMO_NS_SEPARATOR, BUNDLE_PEP_NAME, OMEMO_NS_SEPARATOR_FINAL, device_id);
  size_t buf_len = len + 1;

  char * node_name = malloc(buf_len);
  if (!node_name) {
    return -1;
  }

  size_t actual_len = snprintf(node_name, buf_len, format, OMEMO_NS, OMEMO_NS_SEPARATOR, BUNDLE_PEP_NAME, OMEMO_NS_SEPARATOR_FINAL, device_id);
  if (actual_len != len) {
    free(node_name);
    return -2;
  }

  *node_name_p = node_name;
  return 0;
}

void omemo_bundle_destroy(omemo_bundle * bundle_p) {
  if (bundle_p) {
    mxmlDelete(bundle_p->signed_pk_node_p);
    mxmlDelete(bundle_p->signature_node_p);
    mxmlDelete(bundle_p->identity_key_node_p);
    mxmlDelete(bundle_p->pre_keys_node_p);
    free(bundle_p->device_id);
    free(bundle_p);
  }
}

int omemo_devicelist_create(const char * from, omemo_devicelist ** dl_pp) {
  if (!from || !dl_pp) {
    return OMEMO_ERR_NULL;
  }

  int ret_val = 0;

  omemo_devicelist * dl_p = (void *) 0;
  char * from_dup = (void *) 0;
  mxml_node_t * list_node_p = (void *) 0;

  dl_p = malloc(sizeof(omemo_devicelist));
  if (!dl_p) {
    ret_val = OMEMO_ERR_NOMEM;
    goto cleanup;
  }

  from_dup = g_strndup(from, strlen(from));
  if (!from_dup) {
    ret_val = OMEMO_ERR_NOMEM;
    goto cleanup;
  }

  list_node_p = mxmlNewElement(MXML_NO_PARENT, LIST_NODE_NAME);
  mxmlElementSetAttr(list_node_p, XMLNS_ATTR_NAME, OMEMO_NS);

  dl_p->list_node_p = list_node_p;
  dl_p->id_list_p = (void *) 0;
  dl_p->from = from_dup;
  *dl_pp = dl_p;

cleanup:
  if (ret_val) {
    free(from_dup);
    free(dl_p);
  }
  return ret_val;
}

int omemo_devicelist_import(char * received_devicelist, const char * from, omemo_devicelist ** dl_pp) {
  if (!received_devicelist || !from || !dl_pp) {
    return OMEMO_ERR_NULL;
  }

  int ret_val = 0;

  omemo_devicelist * dl_p = (void *) 0;
  mxml_node_t * items_node_p = (void *) 0;
  mxml_node_t * item_node_p = (void *) 0;
  mxml_node_t * list_node_p = (void *) 0;
  mxml_node_t * device_node_p = (void *) 0;
  GList * id_list_p = (void *) 0;

  ret_val = omemo_devicelist_create(from, &dl_p);
  if (ret_val) {
    goto cleanup;
  }

  items_node_p = mxmlLoadString((void *) 0, received_devicelist, MXML_NO_CALLBACK);
  if (!items_node_p) {
    log_err("received devicelist response is invalid XML: %s", received_devicelist);
    ret_val = OMEMO_ERR_MALFORMED_XML;
    goto cleanup;
  }

  ret_val = strncmp(mxmlGetElement(items_node_p), ITEMS_NODE_NAME, strlen(ITEMS_NODE_NAME));
  if (ret_val) {
    ret_val = OMEMO_ERR_MALFORMED_DEVICELIST_NO_ITEMS_ELEM;
    goto cleanup;
  }

  item_node_p = mxmlGetFirstChild(items_node_p);
  if (!item_node_p) {
    // valid empty response; no child elements at all
    ret_val = 0;
    *dl_pp = dl_p;
    goto cleanup;
  }

  if (mxmlGetType(item_node_p) != MXML_ELEMENT) {
    ret_val = OMEMO_ERR_MALFORMED_DEVICELIST_NO_ITEM_ELEM;
    goto cleanup;
  }

  ret_val = strncmp(mxmlGetElement(item_node_p), ITEM_NODE_NAME, strlen(ITEM_NODE_NAME));
  if (ret_val) {
    // child of <items> element is not an <item> element
    ret_val = OMEMO_ERR_MALFORMED_DEVICELIST_NO_ITEM_ELEM;
    goto cleanup;
  }

  ret_val = expect_next_node(item_node_p, mxmlGetFirstChild, LIST_NODE_NAME, &list_node_p);
  if (ret_val) {
    ret_val = OMEMO_ERR_MALFORMED_DEVICELIST_NO_LIST_ELEM;
    goto cleanup;
  }

  mxmlDelete(dl_p->list_node_p);
  mxmlRemove(list_node_p);
  dl_p->list_node_p = list_node_p;

  ret_val = expect_next_node(list_node_p, mxmlGetFirstChild, DEVICE_NODE_NAME, &device_node_p);
  if (ret_val) {
    // another variant of valid empty list: a <list> element inside an <item> element, buz with no children
    ret_val = 0;
    *dl_pp = dl_p;
    goto cleanup;
  }

  size_t device_count = 0;
  while (device_node_p) {
    device_count++;

    const char * id_string = mxmlElementGetAttr(device_node_p, DEVICE_NODE_ID_ATTR_NAME);
    if (!id_string) {
      log_err("device element #%zu does not have an ID attribute", device_count);
      ret_val = OMEMO_ERR_MALFORMED_DEVICELIST_NO_DEVICE_ID_ATTR;
      goto cleanup;
    }

    uint32_t * id_temp_p = malloc(sizeof(uint32_t));
    if (!id_temp_p) {
      ret_val = OMEMO_ERR_NOMEM;
      goto cleanup;
    }

    *id_temp_p = strtol(id_string, (void *) 0, 0);
    id_list_p = g_list_append(id_list_p, id_temp_p);

    device_node_p = mxmlGetNextSibling(device_node_p);
  }
  dl_p->id_list_p = id_list_p;

  *dl_pp = dl_p;

cleanup:
  if (ret_val) {
    omemo_devicelist_destroy(dl_p);
    g_list_free_full(id_list_p, free);
  }
  mxmlDelete(items_node_p);
  return ret_val;
}

int omemo_devicelist_add(omemo_devicelist * dl_p, uint32_t device_id) {
  if (!dl_p || !dl_p->list_node_p) {
    return OMEMO_ERR_NULL;
  }

  uint32_t * id_p = malloc(sizeof(uint32_t));
  if (!id_p) {
    return OMEMO_ERR_NOMEM;
  }
  *id_p = device_id;

  char * id_string;
  int id_string_len = int_to_string(device_id, &id_string);
  if (id_string_len < 1) {
    free(id_p);
    return OMEMO_ERR;
  }

  mxml_node_t * device_node_p = mxmlNewElement(MXML_NO_PARENT, DEVICE_NODE_NAME);
  mxmlElementSetAttr(device_node_p, DEVICE_NODE_ID_ATTR_NAME, id_string);
  mxmlAdd(dl_p->list_node_p, MXML_ADD_AFTER, MXML_ADD_TO_PARENT, device_node_p);
  dl_p->id_list_p = g_list_append(dl_p->id_list_p, id_p);

  return 0;
}

int omemo_devicelist_contains_id(const omemo_devicelist * dl_p, uint32_t device_id) {
  if (!dl_p || !dl_p->list_node_p) {
    return 0;
  }

  GList * curr = dl_p->id_list_p;
  while(curr) {
    if (*((uint32_t *) curr->data) == device_id) {
      return 1;
    }

    curr = curr->next;
  }

  return 0;
}

int omemo_devicelist_remove(omemo_devicelist * dl_p, uint32_t device_id) {
  if (!dl_p) {
    return OMEMO_ERR_NULL;
  }
  int ret_val = 0;

  char * device_id_str = (void *) 0;
  mxml_node_t * device_node_p = (void *) 0;
  GList * curr_p = (void *) 0;
  uint32_t * remove_id_p = (void *) 0;

  ret_val = int_to_string(device_id, &device_id_str);
  if (ret_val < 1) {
    ret_val = OMEMO_ERR;
    goto cleanup;
  }
  ret_val = 0;

  device_node_p = mxmlFindElement(dl_p->list_node_p, dl_p->list_node_p, DEVICE_NODE_NAME, DEVICE_NODE_ID_ATTR_NAME, device_id_str, MXML_DESCEND);
  if (!device_node_p) {
    goto cleanup;
  }
  mxmlDelete(device_node_p);

  for (curr_p = dl_p->id_list_p; curr_p; curr_p = curr_p->next) {
    if (omemo_devicelist_list_data(curr_p) == device_id) {
      remove_id_p = curr_p->data;
      break;
    }
  }

  dl_p->id_list_p = g_list_remove(dl_p->id_list_p, remove_id_p);

cleanup:
  free(device_id_str);
  return ret_val;
}

int omemo_devicelist_is_empty(omemo_devicelist * dl_p) {
  return dl_p->id_list_p ? 0 : 1;
}

int omemo_devicelist_diff(const omemo_devicelist * dl_a_p, const omemo_devicelist * dl_b_p, GList ** a_minus_b_pp, GList ** b_minus_a_pp) {
  if (!dl_a_p || !dl_b_p || !a_minus_b_pp || !b_minus_a_pp) {
    return OMEMO_ERR_NULL;
  }

  GList * a_l_p = (void *) 0;
  GList * b_l_p = (void *) 0;
  GList * amb_p = (void *) 0;
  GList * bma_p = (void *) 0;
  GList * curr_p = (void *) 0;
  GList * temp_p = (void *) 0;

  a_l_p = omemo_devicelist_get_id_list(dl_a_p);
  b_l_p = omemo_devicelist_get_id_list(dl_b_p);

  curr_p = a_l_p;
  while (curr_p) {
    temp_p = curr_p;
    curr_p = curr_p->next;
    if (!omemo_devicelist_contains_id(dl_b_p, omemo_devicelist_list_data(temp_p))) {
      a_l_p = g_list_remove_link(a_l_p, temp_p);
      amb_p = g_list_prepend(amb_p, temp_p->data);
      g_list_free_1(temp_p);
    }
  }

  curr_p = b_l_p;
  while (curr_p) {
    temp_p = curr_p;
    curr_p = curr_p->next;
    if (!omemo_devicelist_contains_id(dl_a_p, omemo_devicelist_list_data(temp_p))) {
      b_l_p = g_list_remove_link(b_l_p, temp_p);
      bma_p = g_list_prepend(bma_p, temp_p->data);
      g_list_free_1(temp_p);
    }
  }

  *a_minus_b_pp = amb_p;
  *b_minus_a_pp = bma_p;

  g_list_free_full(a_l_p, free);
  g_list_free_full(b_l_p, free);

  return 0;
}

int omemo_devicelist_export(omemo_devicelist * dl_p, char ** xml_p) {
  if (!dl_p || !dl_p->list_node_p || !xml_p) {
    return OMEMO_ERR_NULL;
  }

  mxml_node_t * publish_node_p = mxmlNewElement(MXML_NO_PARENT, PUBLISH_NODE_NAME);
  mxmlElementSetAttr(publish_node_p, PUBLISH_NODE_NODE_ATTR_NAME, OMEMO_DEVICELIST_PEP_NODE);

  mxml_node_t * item_node_p = mxmlNewElement(publish_node_p, ITEM_NODE_NAME);
  mxmlAdd(item_node_p, MXML_ADD_AFTER, MXML_ADD_TO_PARENT, dl_p->list_node_p);

  char * xml = mxmlSaveAllocString(publish_node_p, MXML_NO_CALLBACK);
  if (!xml) {
    return OMEMO_ERR;
  }

  *xml_p = xml;
  return 0;
}

GList * omemo_devicelist_get_id_list(const omemo_devicelist * dl_p) {
  GList * new_l_p = (void *) 0;
  GList * curr_p = (void *) 0;
  uint32_t * cpy_p = (void *) 0;

  for (curr_p = dl_p->id_list_p; curr_p; curr_p = curr_p->next) {
    cpy_p = malloc(sizeof(uint32_t));
    if (!cpy_p) {
      g_list_free_full(new_l_p, free);
      return (void *) 0;
    }
    memcpy(cpy_p, curr_p->data, (sizeof(uint32_t)));

    new_l_p = g_list_append(new_l_p, cpy_p);
  }

  return new_l_p;
}

int omemo_devicelist_has_id_list(const omemo_devicelist * dl_p) {
  return (dl_p->id_list_p) ? 1 : 0;
}

const char * omemo_devicelist_get_owner(const omemo_devicelist * dl_p) {
  return dl_p ? dl_p->from : (void *) 0;
}

int omemo_devicelist_get_pep_node_name(char ** node_name_p) {
  char * format = "%s%s%s";
  size_t len = snprintf((void *) 0, 0, format, OMEMO_NS, OMEMO_NS_SEPARATOR, DEVICELIST_PEP_NAME);
  size_t buf_len = len + 1;

  char * node_name = malloc(buf_len);
  if (!node_name) {
    return -1;
  }

  size_t actual_len = snprintf(node_name, buf_len, format, OMEMO_NS, OMEMO_NS_SEPARATOR, DEVICELIST_PEP_NAME);
  if (actual_len != len) {
    free(node_name);
    return -2;
  }

  *node_name_p = node_name;
  return 0;
}

void omemo_devicelist_destroy(omemo_devicelist * dl_p) {
  if (dl_p) {
    g_list_free_full(dl_p->id_list_p, free);
    mxmlDelete(dl_p->list_node_p);
    free(dl_p->from);
    free(dl_p);
  }
}

int omemo_message_create(uint32_t sender_device_id, const omemo_crypto_provider * crypto_p, omemo_message ** message_pp) {
  if (!crypto_p || !crypto_p->random_bytes_func || !crypto_p->aes_gcm_encrypt_func || !message_pp) {
    return OMEMO_ERR_NULL;
  }

  int ret_val = 0;

  omemo_message * msg_p = (void *) 0;
  uint8_t * iv_p = (void *) 0;
  gchar * iv_b64 = (void *) 0;
  char * device_id_string = (void *) 0;
  mxml_node_t * header_node_p = (void *) 0;
  mxml_node_t * iv_node_p = (void *) 0;
  uint8_t * key_p = (void *) 0;

  msg_p = malloc(sizeof(omemo_message));
  if (!msg_p) {
    ret_val = OMEMO_ERR_NOMEM;
    goto cleanup;
  }
  memset(msg_p, 0, sizeof(omemo_message));

  ret_val = crypto_p->random_bytes_func(&iv_p, OMEMO_AES_GCM_IV_LENGTH, crypto_p->user_data_p);
  if (ret_val) {
    goto cleanup;
  }
  msg_p->iv_p = iv_p;
  msg_p->iv_len = OMEMO_AES_GCM_IV_LENGTH;
  iv_b64 = g_base64_encode(iv_p, OMEMO_AES_GCM_IV_LENGTH);

  if (int_to_string(sender_device_id, &device_id_string) <= 0) {
    ret_val = -1;
    goto cleanup;
  }
  header_node_p = mxmlNewElement(MXML_NO_PARENT, HEADER_NODE_NAME);
  mxmlElementSetAttr(header_node_p, HEADER_NODE_SID_ATTR_NAME, device_id_string);

  iv_node_p = mxmlNewElement(header_node_p, IV_NODE_NAME);
  (void) mxmlNewOpaque(iv_node_p, iv_b64);
  msg_p->header_node_p = header_node_p;

  ret_val = crypto_p->random_bytes_func(&key_p, OMEMO_AES_128_KEY_LENGTH + OMEMO_AES_GCM_TAG_LENGTH, crypto_p->user_data_p);
  if (ret_val) {
    goto cleanup;
  }

  msg_p->key_p = key_p;
  msg_p->key_len = OMEMO_AES_128_KEY_LENGTH;
  msg_p->tag_len = 0;

  *message_pp = msg_p;

cleanup:
  if (ret_val) {
    omemo_message_destroy(msg_p);
  }

  free(device_id_string);
  g_free(iv_b64);

  return ret_val;
}

int omemo_message_strip_possible_plaintext(omemo_message * msg_p) {
  if (!msg_p) {
    return OMEMO_ERR_NULL;
  }

  int ret_val = 0;
  mxml_node_t * bad_node_p = (void *) 0;

  bad_node_p = mxmlFindElement(msg_p->message_node_p, msg_p->message_node_p, "html", NULL, NULL, MXML_DESCEND_FIRST);
  if (bad_node_p) {
    mxmlDelete(bad_node_p);
  }

  for (bad_node_p = mxmlFindElement(msg_p->message_node_p, msg_p->message_node_p, "body", NULL, NULL, MXML_DESCEND_FIRST);
       bad_node_p;
       bad_node_p = mxmlFindElement(msg_p->message_node_p, msg_p->message_node_p, "body", NULL, NULL, MXML_DESCEND_FIRST)) {
    mxmlDelete(bad_node_p);
  }

  return ret_val;
}

int omemo_message_prepare_encryption(char * outgoing_message, uint32_t sender_device_id, const omemo_crypto_provider * crypto_p, int strip, omemo_message ** message_pp) {
  if (!outgoing_message || !crypto_p || !crypto_p->random_bytes_func || !crypto_p->aes_gcm_encrypt_func || !message_pp) {
    return OMEMO_ERR_NULL;
  }

  int ret_val = 0;

  omemo_message * msg_p = (void *) 0;
  mxml_node_t * msg_node_p = (void *) 0;
  mxml_node_t * body_node_p = (void *) 0;
  const char * msg_text = (void *) 0;

  uint8_t * ct_p = (void *) 0;
  size_t ct_len = 0;

  gchar * payload_b64 = (void *) 0;
  mxml_node_t * payload_node_p = (void *) 0;

  uint8_t * tag_p = (void *) 0;

  ret_val = omemo_message_create(sender_device_id, crypto_p, &msg_p);
  if (ret_val) {
    goto cleanup;
  }

  msg_node_p = mxmlLoadString((void *) 0, outgoing_message, MXML_OPAQUE_CALLBACK);
  if (!msg_node_p) {
    log_err("outgoing message is invalid XML: %s", outgoing_message);
    ret_val = OMEMO_ERR_MALFORMED_XML;
    goto cleanup;
  }
  msg_p->message_node_p = msg_node_p;

  body_node_p = mxmlFindPath(msg_node_p, BODY_NODE_NAME);
  if (!body_node_p) {
    ret_val = OMEMO_ERR_MALFORMED_OUTGOING_MESSAGE_NO_BODY_ELEM;
    goto cleanup;
  }

  msg_text = mxmlGetOpaque(body_node_p);
  if (!msg_text) {
    ret_val = OMEMO_ERR_MALFORMED_OUTGOING_MESSAGE_NO_BODY_DATA;
    goto cleanup;
  }

  ret_val = crypto_p->aes_gcm_encrypt_func((uint8_t *) msg_text, strlen(msg_text),
                                           msg_p->iv_p, msg_p->iv_len,
                                           msg_p->key_p, msg_p->key_len,
                                           OMEMO_AES_GCM_TAG_LENGTH,
                                           crypto_p->user_data_p,
                                           &ct_p, &ct_len,
                                           &tag_p);
  if (ret_val) {
    goto cleanup;
  }

  msg_p->tag_len = OMEMO_AES_GCM_TAG_LENGTH;
  memcpy(msg_p->key_p + msg_p->key_len, tag_p, msg_p->tag_len);

  // to delete it, go one level up
  ret_val = expect_next_node(body_node_p, mxmlGetParent, BODY_NODE_NAME, &body_node_p);
  if (ret_val) {
    log_err("failed to navigate to %s node for deletion", BODY_NODE_NAME);
    ret_val = OMEMO_ERR_MALFORMED_OUTGOING_MESSAGE_NO_BODY_ELEM;
    goto cleanup;
  }

  mxmlRemove(body_node_p);

  payload_b64 = g_base64_encode(ct_p, ct_len);
  payload_node_p = mxmlNewElement(MXML_NO_PARENT, PAYLOAD_NODE_NAME);
  (void) mxmlNewOpaque(payload_node_p, payload_b64);
  msg_p->payload_node_p = payload_node_p;

  if (strip == OMEMO_STRIP_ALL) {
    omemo_message_strip_possible_plaintext(msg_p);
  }

  *message_pp = msg_p;

cleanup:
  if (ret_val) {
    omemo_message_destroy(msg_p);
  }

  free(ct_p);
  g_free(payload_b64);
  free(tag_p);

  return ret_val;
}

const uint8_t * omemo_message_get_key(omemo_message * msg_p) {
  return (msg_p) ? msg_p->key_p : (void *) 0;
}

size_t omemo_message_get_key_len(omemo_message * msg_p) {
  return msg_p->key_len + msg_p->tag_len;
}

// adds a "key" element with the given parameters to the header.
static int add_recipient(omemo_message * msg_p, uint32_t device_id, const uint8_t * encrypted_key_p, size_t key_len, bool prekey) {
  if (!msg_p || !msg_p->header_node_p || !encrypted_key_p) {
    return OMEMO_ERR_NULL;
  }

  char * device_id_string = (void *) 0;
  if (int_to_string(device_id, &device_id_string) <= 0) {
    return OMEMO_ERR;
  }

  gchar * key_b64 = g_base64_encode(encrypted_key_p, key_len);
  mxml_node_t * key_node_p =  mxmlNewElement(MXML_NO_PARENT, KEY_NODE_NAME);
  mxmlElementSetAttr(key_node_p, KEY_NODE_RID_ATTR_NAME, device_id_string);
  (void) mxmlNewOpaque(key_node_p, key_b64);

  if (prekey) {
    mxmlElementSetAttr(key_node_p, KEY_NODE_PREKEY_ATTR_NAME, KEY_NODE_PREKEY_ATTR_VAL_TRUE);
  }

  mxmlAdd(msg_p->header_node_p, MXML_ADD_BEFORE, MXML_ADD_TO_PARENT, key_node_p);

  free(device_id_string);
  g_free(key_b64);
  return 0;
}

int omemo_message_add_recipient(omemo_message * msg_p, uint32_t device_id, const uint8_t * encrypted_key_p, size_t key_len) {
  return add_recipient(msg_p, device_id, encrypted_key_p, key_len, false);
}

int omemo_message_add_recipient_w_prekey(omemo_message * msg_p, uint32_t device_id, const uint8_t * encrypted_key_p, size_t key_len) {
  return add_recipient(msg_p, device_id, encrypted_key_p, key_len, true);
}

int omemo_message_export_encrypted(omemo_message * msg_p, int add_msg, char ** msg_xml) {
  if (!msg_p || !msg_p->message_node_p || !msg_p->header_node_p || !msg_p->payload_node_p || !msg_xml) {
    return OMEMO_ERR_NULL;
  }

  int ret_val = 0;

  mxml_node_t * body_node_p = (void *) 0;
  mxml_node_t * encrypted_node_p = (void *) 0;
  mxml_node_t * eme_node_p = (void *) 0;
  mxml_node_t * store_node_p = (void *) 0;
  char * xml_str = (void *) 0;

  if (add_msg == OMEMO_ADD_MSG_BODY || add_msg == OMEMO_ADD_MSG_BOTH) {
    body_node_p = mxmlNewElement(msg_p->message_node_p, BODY_NODE_NAME);
    (void) mxmlNewOpaque(body_node_p, NO_OMEMO_MSG);
  }

  encrypted_node_p = mxmlNewElement(msg_p->message_node_p, ENCRYPTED_NODE_NAME);
  mxmlElementSetAttr(encrypted_node_p, XMLNS_ATTR_NAME, OMEMO_NS);

  mxmlAdd(encrypted_node_p, MXML_ADD_AFTER, MXML_ADD_TO_PARENT, msg_p->header_node_p);
  mxmlAdd(encrypted_node_p, MXML_ADD_AFTER, MXML_ADD_TO_PARENT, msg_p->payload_node_p);

  if (add_msg == OMEMO_ADD_MSG_EME || add_msg == OMEMO_ADD_MSG_BOTH) {
    eme_node_p = mxmlNewElement(msg_p->message_node_p, EME_NODE_NAME);
    mxmlElementSetAttr(eme_node_p, XMLNS_ATTR_NAME, EME_XMLNS);
    mxmlElementSetAttr(eme_node_p, EME_NAMESPACE_ATTR_NAME, OMEMO_NS);
    mxmlElementSetAttr(eme_node_p, EME_NAME_ATTR_NAME, "OMEMO");
  }

  store_node_p = mxmlNewElement(msg_p->message_node_p, STORE_NODE_NAME);
  mxmlElementSetAttr(store_node_p, XMLNS_ATTR_NAME, HINTS_XMLNS);

  xml_str = mxmlSaveAllocString(msg_p->message_node_p, MXML_NO_CALLBACK);
  if (!xml_str) {
    ret_val = OMEMO_ERR;
    goto cleanup;
  }

  *msg_xml = xml_str;

cleanup:
  if (!ret_val) {
    mxmlRemove(msg_p->header_node_p);
    mxmlRemove(msg_p->payload_node_p);
  }
  mxmlDelete(body_node_p);
  mxmlDelete(encrypted_node_p);
  mxmlDelete(store_node_p);
  mxmlDelete(eme_node_p);

  return ret_val;
}

int omemo_message_prepare_decryption(char * incoming_message, omemo_message ** msg_pp) {
  if (!incoming_message || !msg_pp) {
    return OMEMO_ERR_NULL;
  }

  int ret_val = 0;
  mxml_node_t * message_node_p   = (void *) 0;
  mxml_node_t * body_node_p      = (void *) 0;
  mxml_node_t * eme_node_p       = (void *) 0;
  mxml_node_t * store_node_p     = (void *) 0;
  mxml_node_t * encrypted_node_p = (void *) 0;
  mxml_node_t * header_node_p    = (void *) 0;
  mxml_node_t * payload_node_p   = (void *) 0;
  omemo_message * msg_p          = (void *) 0;

  message_node_p = mxmlLoadString((void *) 0, incoming_message, MXML_OPAQUE_CALLBACK);
  if (!message_node_p) {
    log_err("incoming message is invalid XML: %s", incoming_message);
    ret_val = OMEMO_ERR_MALFORMED_XML;
    goto cleanup;
  }

  body_node_p = mxmlFindPath(message_node_p, BODY_NODE_NAME);
  if (body_node_p) {
    ret_val = expect_next_node(body_node_p, mxmlGetParent, BODY_NODE_NAME, &body_node_p);
    if (ret_val) {
      ret_val = OMEMO_ERR_MALFORMED_INCOMING_MESSAGE_NO_BODY_ELEM;
      goto cleanup;
    }
  }

  eme_node_p = mxmlFindPath(message_node_p, EME_NODE_NAME);

  store_node_p = mxmlFindPath(message_node_p, STORE_NODE_NAME);

  encrypted_node_p = mxmlFindPath(message_node_p, ENCRYPTED_NODE_NAME);
  if (!encrypted_node_p) {
    ret_val = OMEMO_ERR_MALFORMED_INCOMING_MESSAGE_NO_ENCRYPTED_ELEM;
    goto cleanup;
  }

  header_node_p = mxmlFindPath(encrypted_node_p, HEADER_NODE_NAME);
  if (!header_node_p) {
    ret_val = OMEMO_ERR_MALFORMED_INCOMING_MESSAGE_NO_HEADER_ELEM;
    goto cleanup;
  }

  payload_node_p = mxmlFindPath(encrypted_node_p, PAYLOAD_NODE_NAME);

  msg_p = malloc(sizeof(omemo_message));
  if (!msg_p) {
    ret_val = OMEMO_ERR_NOMEM;
    goto cleanup;
  }
  memset(msg_p, 0, sizeof(omemo_message));

  if (body_node_p) {
    mxmlDelete(body_node_p);
  }

  if (eme_node_p) {
    mxmlDelete(eme_node_p);
  }

  if (store_node_p) {
    mxmlDelete(store_node_p);
  }

  mxmlRemove(header_node_p);
  msg_p->header_node_p = header_node_p;

  if (payload_node_p) {
    payload_node_p = mxmlGetParent(payload_node_p);
    mxmlRemove(payload_node_p);
    msg_p->payload_node_p = payload_node_p;
  }

  mxmlDelete(encrypted_node_p);
  msg_p->message_node_p = message_node_p;

  *msg_pp = msg_p;

cleanup:
  if (ret_val) {
    mxmlDelete(message_node_p);
    free(msg_p);
  }
  return ret_val;
}

int omemo_message_has_payload(omemo_message * msg_p) {
  return (msg_p->payload_node_p) ? 1 : 0;
}

uint32_t omemo_message_get_sender_id(omemo_message * msg_p) {
  const char * sid_string = mxmlElementGetAttr(msg_p->header_node_p, HEADER_NODE_SID_ATTR_NAME);
  return strtol(sid_string, (void *) 0, 0);
}

static char * jid_strip_resource(const char * full_jid) {
  char ** split = (void *) 0;
  split = g_strsplit(full_jid, "/", -1);
  char * ret = g_strndup(split[0], strlen(split[0]));

  g_strfreev(split);

  return ret;
}

const char * omemo_message_get_sender_name_full(omemo_message * msg_p) {
  return mxmlElementGetAttr(msg_p->message_node_p, MESSAGE_NODE_FROM_ATTR_NAME);
}

char * omemo_message_get_sender_name_bare(omemo_message * msg_p) {
  return jid_strip_resource(omemo_message_get_sender_name_full(msg_p));
}

const char * omemo_message_get_recipient_name_full(omemo_message * msg_p) {
  return mxmlElementGetAttr(msg_p->message_node_p, MESSAGE_NODE_TO_ATTR_NAME);
}

char * omemo_message_get_recipient_name_bare(omemo_message * msg_p) {
  return jid_strip_resource(omemo_message_get_recipient_name_full(msg_p));
}

// Finds the key element for the given recipient device ID.
static int omemo_message_find_key_element(omemo_message * msg_p, uint32_t rid, mxml_node_t ** key_node_pp) {
  if (!msg_p || !key_node_pp) {
    return OMEMO_ERR_NULL;
  }

  int ret_val = 0;
  mxml_node_t * key_node_p = (void *) 0;
  char * rid_string = (void *) 0;

  key_node_p = mxmlFindElement(msg_p->header_node_p, msg_p->header_node_p, KEY_NODE_NAME, NULL, NULL, MXML_DESCEND);
  if (!key_node_p) {
    // if there is not at least one key, skip the rest of the function
    ret_val = 0;
    *key_node_pp = (void *) 0;
    goto cleanup;
  }

  if (int_to_string(rid, &rid_string) <= 0) {
    ret_val = OMEMO_ERR_NOMEM;
    goto cleanup;
  }

  while (key_node_p) {
    if (!strncmp(rid_string, mxmlElementGetAttr(key_node_p, KEY_NODE_RID_ATTR_NAME), strlen(rid_string))) {
      *key_node_pp = key_node_p;
        break;
    }

    ret_val = expect_next_node(key_node_p, mxmlGetNextSibling, KEY_NODE_NAME, &key_node_p);
    if (ret_val) {
      key_node_p = (void *) 0;
      ret_val = 0;
    }
  }

  
cleanup:
  free(rid_string);

  return ret_val;
}

int omemo_message_get_encrypted_key(omemo_message * msg_p, uint32_t own_device_id, uint8_t ** key_pp, size_t * key_len_p ) {
  if (!msg_p || !key_pp) {
    return OMEMO_ERR_NULL;
  }

  int ret_val = 0;

  mxml_node_t * key_node_p = (void *) 0;
  const char * key_b64 = (void *) 0;
  uint8_t * key_p = (void *) 0;
  size_t key_len = 0;

  ret_val = omemo_message_find_key_element(msg_p, own_device_id, &key_node_p);
  if (ret_val || !key_node_p) {
    goto cleanup;
  }

  key_b64 = mxmlGetOpaque(key_node_p);
  if (!key_b64) {
    *key_pp = (void *) 0;
    ret_val = OMEMO_ERR_MALFORMED_INCOMING_MESSAGE_NO_KEY_DATA;
    goto cleanup;
  }

  key_p = g_base64_decode(key_b64, &key_len);

cleanup:
  *key_pp = key_p;
  *key_len_p = key_len;
  
  return ret_val;
}

int omemo_message_is_encrypted_key_prekey(omemo_message * msg_p, uint32_t key_id, bool * is_prekey_p) {
  if (!msg_p) {
    return OMEMO_ERR_NULL;
  }

  int ret_val = 0;
  mxml_node_t * key_node_p = (void *) 0;
  const char * prekey_attr_val = (void *) 0;

  ret_val = omemo_message_find_key_element(msg_p, key_id, &key_node_p);
  if (ret_val || !key_node_p) {
    *is_prekey_p = false;
    goto cleanup;
  }


  prekey_attr_val = mxmlElementGetAttr(key_node_p, KEY_NODE_PREKEY_ATTR_NAME);
  if (prekey_attr_val) {
    // according to https://www.w3.org/TR/xmlschema-2/#boolean "1" can also be a boolean that means true
    if (!strncmp(prekey_attr_val, KEY_NODE_PREKEY_ATTR_VAL_TRUE, strlen(KEY_NODE_PREKEY_ATTR_VAL_TRUE)) || !strncmp(prekey_attr_val, "1", strlen("1"))) {
      *is_prekey_p = true;
      goto cleanup;
    }
  }

  *is_prekey_p = false;

cleanup:
  return ret_val;
}

int omemo_message_export_decrypted(omemo_message * msg_p, uint8_t * key_p, size_t key_len, const omemo_crypto_provider * crypto_p, char ** msg_xml_p) {
  if (!msg_p || !msg_p->header_node_p || !msg_p->payload_node_p || !msg_p->message_node_p || !key_p || !crypto_p || !msg_xml_p) {
    return OMEMO_ERR_NULL;
  }

  int ret_val = 0;

  const char * payload_b64 = (void *) 0;
  uint8_t * payload_p = (void *) 0;
  size_t payload_len = 0;
  mxml_node_t * iv_node_p = (void *) 0;
  const char * iv_b64 = (void *) 0;
  uint8_t * iv_p = (void *) 0;
  size_t iv_len = 0;
  size_t key_len_actual = 0;
  size_t payload_len_actual = 0;
  uint8_t * tag_p = (void *) 0;
  uint8_t * pt_p = (void *) 0;
  size_t pt_len = 0;
  char * pt_str = (void *) 0;
  mxml_node_t * body_node_p = (void *) 0;
  char * xml = (void *) 0;

  payload_b64 = mxmlGetOpaque(msg_p->payload_node_p);
  if (!payload_b64) {
    ret_val = OMEMO_ERR_MALFORMED_INCOMING_MESSAGE_NO_PAYLOAD_DATA;
    goto cleanup;
  }
  payload_p = g_base64_decode(payload_b64, &payload_len);

  iv_node_p = mxmlFindElement(msg_p->header_node_p, msg_p->header_node_p, IV_NODE_NAME, NULL, NULL, MXML_DESCEND);
  if (!iv_node_p) {
    ret_val = OMEMO_ERR_MALFORMED_INCOMING_MESSAGE_NO_IV_ELEM;
    goto cleanup;
  }

  iv_b64 = mxmlGetOpaque(iv_node_p);
  if (!iv_b64) {
    ret_val = OMEMO_ERR_MALFORMED_INCOMING_MESSAGE_NO_IV_DATA;
    goto cleanup;
  }
  iv_p = g_base64_decode(iv_b64, &iv_len);

  if (key_len == OMEMO_AES_128_KEY_LENGTH + OMEMO_AES_GCM_TAG_LENGTH) {
    key_len_actual = OMEMO_AES_128_KEY_LENGTH;
    payload_len_actual = payload_len;
    tag_p = key_p + OMEMO_AES_128_KEY_LENGTH;
  } else if (key_len == OMEMO_AES_128_KEY_LENGTH) {
    key_len_actual = key_len;
    payload_len_actual = payload_len - OMEMO_AES_GCM_TAG_LENGTH;
    tag_p = payload_p + (payload_len - OMEMO_AES_GCM_TAG_LENGTH);
  } else {
    ret_val = OMEMO_ERR_UNSUPPORTED_KEY_LEN;
    goto cleanup;
  }

  ret_val = crypto_p->aes_gcm_decrypt_func(payload_p, payload_len_actual,
                                           iv_p, iv_len,
                                           key_p, key_len_actual,
                                           tag_p, OMEMO_AES_GCM_TAG_LENGTH,
                                           crypto_p->user_data_p,
                                           &pt_p, &pt_len);
  if (ret_val) {
    goto cleanup;
  }

  pt_str = malloc(pt_len + 1);
  if (!pt_str) {
    ret_val = OMEMO_ERR_NOMEM;
    goto cleanup;
  }
  memcpy(pt_str, pt_p, pt_len);
  pt_str[pt_len] = '\0';

  body_node_p = mxmlNewElement(MXML_NO_PARENT, BODY_NODE_NAME);
  (void) mxmlNewText(body_node_p, 0, pt_str);


  mxmlAdd(msg_p->message_node_p, MXML_ADD_AFTER, MXML_ADD_TO_PARENT, body_node_p);

  xml = mxmlSaveAllocString(msg_p->message_node_p, MXML_NO_CALLBACK);
  if (!xml) {
    ret_val = OMEMO_ERR_NOMEM;
    goto cleanup;
  }

  *msg_xml_p = xml;

cleanup:
  g_free(payload_p);
  g_free(iv_p);
  free(pt_p);
  free(pt_str);
  mxmlDelete(body_node_p);

  return ret_val;
}

void omemo_message_destroy(omemo_message * msg_p) {
  if (msg_p) {
    mxmlDelete(msg_p->message_node_p);
    mxmlDelete(msg_p->header_node_p);
    mxmlDelete(msg_p->payload_node_p);
    if (msg_p->key_p) {
      memset(msg_p->key_p, 0, msg_p->key_len);
      free(msg_p->key_p);
    }
    if (msg_p->iv_p) {
      memset(msg_p->iv_p, 0, msg_p->iv_len);
      free(msg_p->iv_p);
    }
  }
}
