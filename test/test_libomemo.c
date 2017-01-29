#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include "../src/libomemo.c"
#include "../src/libomemo_crypto.c"

char * devicelist = "<items node='urn:xmpp:omemo:0:devicelist'>"
                              "<item>"
                                "<list xmlns='urn:xmpp:omemo:0'>"
                                   "<device id='4223' />"
                                   "<device id='1337' />"
                                "</list>"
                              "</item>"
                            "</items>";

char * bundle = "<items node='urn:xmpp:omemo:0:bundles:31415'>"
                  "<item>"
                    "<bundle xmlns='urn:xmpp:omemo:0'>"
                      "<signedPreKeyPublic signedPreKeyId='1'>sWsAtQ==</signedPreKeyPublic>"
                      "<signedPreKeySignature>sWsAtQ==</signedPreKeySignature>"
                      "<identityKey>sWsAtQ==</identityKey>"
                      "<prekeys>"
                        "<preKeyPublic preKeyId='10'>sWsAtQ==</preKeyPublic>"
                        "<preKeyPublic preKeyId='20'>sWsAtQ==</preKeyPublic>"
                        "<preKeyPublic preKeyId='30'>sWsAtQ==</preKeyPublic>"
                        "<preKeyPublic preKeyId='40'>sWsAtQ==</preKeyPublic>"
                      "</prekeys>"
                     "</bundle>"
                    "</item>"
                  "</items>";

char * msg_out = "<message xmlns='jabber:client' type='chat' to='bob@example.com'>"
                    "<body>hello</body>"
                 "</message>";

char * msg_out_extra = "<message id='purplef89d7b27' type='chat' to='bob@example.com'>"
                          "<active xmlns='http://jabber.org/protocol/chatstates'/>"
                          "<body>hello</body>"
                       "</message>";

uint8_t data[] = {0xB1, 0x6B, 0x00, 0xB5};
char * data_b64 = "sWsAtQ==";

omemo_crypto_provider crypto = {
    .random_bytes_func = omemo_default_crypto_random_bytes,
    .aes_gcm_encrypt_func = omemo_default_crypto_aes_gcm_encrypt,
    .aes_gcm_decrypt_func = omemo_default_crypto_aes_gcm_decrypt,
    (void *) 0
};

void test_devicelist_create(void ** state) {
  (void) state;

  assert_int_equal(omemo_devicelist_create((void *) 0, (void *) 0), OMEMO_ERR_NULL);
  assert_int_equal(omemo_devicelist_create("test", (void *) 0), OMEMO_ERR_NULL);

  omemo_devicelist * dl_p;
  assert_int_equal(omemo_devicelist_create("alice", &dl_p), 0);
  assert_string_equal(dl_p->from, "alice");
  assert_string_equal(mxmlGetElement(dl_p->list_node_p), "list");
  assert_string_equal(mxmlElementGetAttr(dl_p->list_node_p, "xmlns"), "urn:xmpp:omemo:0");
  assert_ptr_equal(mxmlGetFirstChild(dl_p->list_node_p), (void *) 0);

  omemo_devicelist_destroy(dl_p);
}

void test_devicelist_import(void ** state) {
  (void) state;

  omemo_devicelist * dl_p;
  assert_int_equal(omemo_devicelist_import(devicelist, "bob", &dl_p), 0);
  assert_string_equal(dl_p->from, "bob");
  assert_string_equal(mxmlGetElement(dl_p->list_node_p), "list");
  assert_string_equal(mxmlElementGetAttr(dl_p->list_node_p, "xmlns"), "urn:xmpp:omemo:0");

  mxml_node_t * device_node_p;
  assert_int_equal(expect_next_node(dl_p->list_node_p, mxmlGetFirstChild, "device", &device_node_p), 0);
  assert_string_equal(mxmlElementGetAttr(device_node_p, "id"), "4223");

  assert_int_equal(expect_next_node(device_node_p, mxmlGetNextSibling, "device", &device_node_p), 0);
  assert_string_equal(mxmlElementGetAttr(device_node_p, "id"), "1337");

  assert_ptr_equal(mxmlGetNextSibling(device_node_p), (void *) 0);
  assert_ptr_equal(mxmlGetNextSibling(dl_p->list_node_p), (void *) 0);

  uint32_t * id_p = dl_p->id_list_p->data;
  assert_int_equal(*id_p, 4223);

  id_p = dl_p->id_list_p->next->data;
  assert_int_equal(*id_p, 1337);

  assert_ptr_equal(dl_p->id_list_p->next->next, (void *) 0);

  omemo_devicelist_destroy(dl_p);
}

void test_devicelist_import_empty(void ** state) {
  (void) state;

  char * devicelist_empty = "<items node='urn:xmpp:omemo:0:devicelist'>"
                                "<item>"
                                  "<list xmlns='urn:xmpp:omemo:0'>"
                                  "</list>"
                                "</item>"
                              "</items>";

  omemo_devicelist * dl_p;
  assert_int_equal(omemo_devicelist_import(devicelist_empty, "alice", &dl_p), 0);

  assert_int_equal(omemo_devicelist_is_empty(dl_p), 1);

  omemo_devicelist_destroy(dl_p);
}

void test_devicelist_add(void ** state) {
  (void) state;

  omemo_devicelist * dl_p;
  assert_int_equal(omemo_devicelist_create("alice", &dl_p), 0);
  assert_ptr_equal(omemo_devicelist_get_id_list(dl_p), (void *) 0);
  assert_int_equal(omemo_devicelist_add(dl_p, 123456), 0);

  mxml_node_t * device_node_p = mxmlGetFirstChild(dl_p->list_node_p);
  assert_string_equal(mxmlGetElement(device_node_p), "device");
  assert_string_equal(mxmlElementGetAttr(device_node_p, "id"), "123456");
  assert_ptr_equal(mxmlGetNextSibling(device_node_p), (void *) 0);

  uint32_t * id_p = dl_p->id_list_p->data;
  assert_int_equal(*id_p, 123456);
  assert_ptr_equal(dl_p->id_list_p->next, (void *) 0);

  GList * dl_l_p = omemo_devicelist_get_id_list(dl_p);
  assert_ptr_not_equal(dl_l_p, (void *) 0);
  assert_int_equal(omemo_devicelist_list_data(dl_l_p), 123456);
  assert_ptr_equal(dl_l_p->next, (void *) 0);

  g_list_free_full(dl_l_p, free);
  omemo_devicelist_destroy(dl_p);
}

void test_devicelist_contains_id(void ** state) {
  (void) state;

  omemo_devicelist * dl_p;
  assert_int_equal(omemo_devicelist_create("alice", &dl_p), 0);
  assert_int_equal(omemo_devicelist_contains_id(dl_p, 123456), 0);
  assert_int_equal(omemo_devicelist_add(dl_p, 123456), 0);
  assert_int_equal(omemo_devicelist_contains_id(dl_p, 123456), 1);
  omemo_devicelist_destroy(dl_p);

  assert_int_equal(omemo_devicelist_import(devicelist, "bob", &dl_p), 0);
  assert_int_equal(omemo_devicelist_contains_id(dl_p, 123456), 0);
  assert_int_equal(omemo_devicelist_contains_id(dl_p, 1337), 1);
  assert_int_equal(omemo_devicelist_contains_id(dl_p, 4223), 1);
  assert_int_equal(omemo_devicelist_add(dl_p, 123456), 0);
  assert_int_equal(omemo_devicelist_contains_id(dl_p, 123456), 1);
  omemo_devicelist_destroy(dl_p);
}

void test_devicelist_remove(void ** state) {
  (void) state;

  omemo_devicelist * dl_p;
  assert_int_equal(omemo_devicelist_import(devicelist, "alice", &dl_p), 0);
  assert_int_equal(omemo_devicelist_contains_id(dl_p, 1337), 1);
  assert_int_equal(omemo_devicelist_contains_id(dl_p, 4223), 1);

  assert_int_equal(omemo_devicelist_remove(dl_p, 1337), 0);
  assert_int_equal(omemo_devicelist_contains_id(dl_p, 1337), 0);
  assert_int_equal(omemo_devicelist_contains_id(dl_p, 4223), 1);

  assert_int_equal(omemo_devicelist_contains_id(dl_p, 1338), 0);
  assert_int_equal(omemo_devicelist_remove(dl_p, 1338), 0);
  assert_int_equal(omemo_devicelist_contains_id(dl_p, 1338), 0);

  assert_int_equal(omemo_devicelist_remove(dl_p, 4223), 0);
  assert_int_equal(omemo_devicelist_contains_id(dl_p, 1337), 0);
  assert_int_equal(omemo_devicelist_contains_id(dl_p, 4223), 0);

  assert_int_equal(omemo_devicelist_contains_id(dl_p, 1338), 0);
  assert_int_equal(omemo_devicelist_remove(dl_p, 1338), 0);
  assert_int_equal(omemo_devicelist_contains_id(dl_p, 1338), 0);

  omemo_devicelist_destroy(dl_p);
}

void test_devicelist_diff(void ** state) {
  (void) state;

  omemo_devicelist * dl_a_p, * dl_b_p;
  assert_int_equal(omemo_devicelist_create("alice", &dl_a_p), 0);
  assert_int_equal(omemo_devicelist_create("alice", &dl_b_p), 0);

  GList * amb_p, * bma_p;
  assert_int_equal(omemo_devicelist_diff((void *) 0, (void *) 0, (void *) 0, (void *) 0), OMEMO_ERR_NULL);
  assert_int_equal(omemo_devicelist_diff(dl_a_p, (void *) 0, (void *) 0, (void *) 0), OMEMO_ERR_NULL);
  assert_int_equal(omemo_devicelist_diff(dl_a_p, dl_b_p, (void *) 0, (void *) 0), OMEMO_ERR_NULL);
  assert_int_equal(omemo_devicelist_diff(dl_a_p, dl_b_p, &amb_p, (void *) 0), OMEMO_ERR_NULL);

  assert_int_equal(omemo_devicelist_diff(dl_a_p, dl_b_p, &amb_p, &bma_p), 0);
  assert_ptr_equal(amb_p, (void *) 0);
  assert_ptr_equal(bma_p, (void *) 0);

  assert_int_equal(omemo_devicelist_add(dl_a_p, 1111), 0);
  assert_int_equal(omemo_devicelist_diff(dl_a_p, dl_b_p, &amb_p, &bma_p), 0);
  assert_ptr_equal(bma_p, (void *) 0);
  assert_int_equal(g_list_length(amb_p), 1);
  assert_int_equal(omemo_devicelist_list_data(amb_p), 1111);
  g_list_free_full(amb_p, free);
  g_list_free_full(bma_p, free);

  assert_int_equal(omemo_devicelist_add(dl_b_p, 1111), 0);
  assert_int_equal(omemo_devicelist_add(dl_b_p, 2222), 0);
  assert_int_equal(omemo_devicelist_diff(dl_a_p, dl_b_p, &amb_p, &bma_p), 0);
  assert_ptr_equal(amb_p, (void *) 0);
  assert_int_equal(g_list_length(bma_p), 1);
  assert_int_equal(omemo_devicelist_list_data(bma_p), 2222);
  g_list_free_full(amb_p, free);
  g_list_free_full(bma_p, free);

  omemo_devicelist_destroy(dl_a_p);
  omemo_devicelist_destroy(dl_b_p);
}

void test_devicelist_export(void ** state) {
  (void) state;

  omemo_devicelist * dl_p;
  assert_int_equal(omemo_devicelist_create("alice", &dl_p), 0);
  assert_int_equal(omemo_devicelist_add(dl_p, 54321), 0);
  assert_int_equal(omemo_devicelist_add(dl_p, 56789), 0);

  char * xml;
  assert_int_equal(omemo_devicelist_export(dl_p, &xml), 0);

  mxml_node_t * publish_node_p = mxmlLoadString((void *) 0, xml, MXML_NO_CALLBACK);
  assert_ptr_not_equal(publish_node_p, (void *) 0);
  assert_string_equal(mxmlGetElement(publish_node_p), "publish");
  assert_string_equal(mxmlElementGetAttr(publish_node_p, "node"), "urn:xmpp:omemo:0:devicelist");

  mxml_node_t * item_node_p = mxmlGetFirstChild(publish_node_p);
  assert_string_equal(mxmlGetElement(item_node_p), "item");

  mxml_node_t * list_node_p = mxmlGetFirstChild(item_node_p);
  assert_string_equal(mxmlGetElement(list_node_p), "list");
  assert_string_equal(mxmlElementGetAttr(list_node_p, "xmlns"), "urn:xmpp:omemo:0");

  mxml_node_t * device_node_p = mxmlGetFirstChild(list_node_p);
  assert_string_equal(mxmlGetElement(device_node_p), "device");
  assert_string_equal(mxmlElementGetAttr(device_node_p, "id"), "54321");

  device_node_p = mxmlGetNextSibling(device_node_p);
  assert_string_equal(mxmlGetElement(device_node_p), "device");
  assert_string_equal(mxmlElementGetAttr(device_node_p, "id"), "56789");

  mxmlDelete(publish_node_p);
  free(xml);
  omemo_devicelist_destroy(dl_p);
}

void test_devicelist_get_pep_node_name(void ** state) {
  (void) state;

  char * node_name = (void *) 0;
  assert_int_equal(omemo_devicelist_get_pep_node_name(&node_name), 0);
  assert_string_equal(node_name, "urn:xmpp:omemo:0:devicelist");

  free(node_name);
}

void test_devicelist_get_id_list(void ** state) {
  (void) state;

  omemo_devicelist * dl_p;
  assert_int_equal(omemo_devicelist_import(devicelist, "alice", &dl_p), 0);

  GList * dl_l_p = omemo_devicelist_get_id_list(dl_p);
  assert_ptr_not_equal(dl_l_p, (void *) 0);
  assert_int_equal(4223, omemo_devicelist_list_data(dl_l_p));

  dl_l_p = dl_l_p->next;
  assert_int_equal(1337, omemo_devicelist_list_data(dl_l_p));
  assert_ptr_equal(dl_l_p->next, (void *) 0);

  g_list_free_full(dl_l_p->prev, free);
  omemo_devicelist_destroy(dl_p);
}

void test_bundle_create(void ** state) {
  (void) state;

  omemo_bundle * bundle_p = (void *) 0;
  assert_int_equal(omemo_bundle_create(&bundle_p), 0);

  assert_ptr_not_equal(bundle_p, (void *) 0);

  assert_int_equal(bundle_p->pre_keys_amount, 0);

  omemo_bundle_destroy(bundle_p);
}

void test_bundle_set_device_id(void ** state) {
  (void) state;

  omemo_bundle * bundle_p = (void *) 0;
  assert_int_equal(omemo_bundle_create(&bundle_p), 0);

  assert_int_equal(omemo_bundle_set_device_id(bundle_p, 1337), 0);
  assert_string_equal(bundle_p->device_id, "1337");
}

void test_bundle_set_signed_pre_key(void ** state) {
  (void) state;

  omemo_bundle * bundle_p = (void *) 0;
  assert_int_equal(omemo_bundle_create(&bundle_p), 0);

  uint8_t data[] = {0xB1, 0x6B, 0x00, 0xB5};
  assert_int_equal(omemo_bundle_set_signed_pre_key(bundle_p, 1, &data[0], 4), 0);

  mxml_node_t * spkn_p = bundle_p->signed_pk_node_p;
  assert_ptr_not_equal(spkn_p, (void *) 0);
  assert_string_equal(mxmlElementGetAttr(spkn_p, SIGNED_PRE_KEY_NODE_ID_ATTR_NAME), "1");
  assert_string_equal(mxmlGetOpaque(spkn_p), g_base64_encode(&data[0], 4));

  omemo_bundle_destroy(bundle_p);
}

void test_bundle_set_signature(void ** state) {
  (void) state;

  omemo_bundle * bundle_p = (void *) 0;
  assert_int_equal(omemo_bundle_create(&bundle_p), 0);

  uint8_t data[] = {0xB1, 0x6B, 0x00, 0xB5};
  assert_int_equal(omemo_bundle_set_signature(bundle_p, &data[0], 4), 0);

  mxml_node_t * signature_p = bundle_p->signature_node_p;
  assert_ptr_not_equal(signature_p, (void *) 0);
  assert_string_equal(mxmlGetOpaque(signature_p), g_base64_encode(&data[0], 4));

  omemo_bundle_destroy(bundle_p);
}

void test_bundle_set_identity_key(void ** state) {
  (void) state;

  omemo_bundle * bundle_p = (void *) 0;
  assert_int_equal(omemo_bundle_create(&bundle_p), 0);

  uint8_t data[] = {0xB1, 0x6B, 0x00, 0xB5};
  assert_int_equal(omemo_bundle_set_identity_key(bundle_p, &data[0], 4), 0);

  assert_ptr_not_equal(bundle_p->identity_key_node_p, (void *) 0);
  assert_string_equal(mxmlGetOpaque(bundle_p->identity_key_node_p), g_base64_encode(&data[0], 4));

  omemo_bundle_destroy(bundle_p);
}

void test_bundle_add_pre_key(void ** state) {
  (void) state;

  omemo_bundle * bundle_p = (void *) 0;
  assert_int_equal(omemo_bundle_create(&bundle_p), 0);

  uint8_t data[] = {0xB1, 0x6B, 0x00, 0xB5};
  assert_int_equal(omemo_bundle_add_pre_key(bundle_p, 1, &data[0], 4), 0);
  assert_ptr_not_equal(bundle_p->pre_keys_node_p, (void *) 0);
  assert_int_equal(bundle_p->pre_keys_amount, 1);
  assert_string_equal(mxmlGetElement(bundle_p->pre_keys_node_p), PREKEYS_NODE_NAME);

  mxml_node_t * pre_key_node_p = mxmlFindPath(bundle_p->pre_keys_node_p, PRE_KEY_NODE_NAME);
  pre_key_node_p = mxmlGetParent(pre_key_node_p);
  assert_ptr_not_equal(pre_key_node_p, (void *) 0);
  assert_string_equal(mxmlElementGetAttr(pre_key_node_p, PRE_KEY_NODE_ID_ATTR_NAME), "1");
  assert_string_equal(mxmlGetOpaque(pre_key_node_p), g_base64_encode(&data[0], 4));

  mxml_node_t * pk_node_before = bundle_p->pre_keys_node_p;
  uint8_t data2[] = {0xBA, 0xDF, 0xEE, 0x75};
  assert_int_equal(omemo_bundle_add_pre_key(bundle_p, 2, &data2[0], 4), 0);
  assert_ptr_equal(pk_node_before, bundle_p->pre_keys_node_p);
  assert_int_equal(bundle_p->pre_keys_amount, 2);

  pre_key_node_p = mxmlGetNextSibling(pre_key_node_p);
  assert_ptr_not_equal(pre_key_node_p, (void *) 0);
  assert_string_equal(mxmlElementGetAttr(pre_key_node_p, PRE_KEY_NODE_ID_ATTR_NAME), "2");
  assert_string_equal(mxmlGetOpaque(pre_key_node_p), g_base64_encode(&data2[0], 4));

  omemo_bundle_destroy(bundle_p);
}

void test_bundle_export(void ** state) {
  (void) state;

  uint8_t * data_p = &data[0];
  size_t len = 4;
  uint32_t reg_id = 1337;

  char * publish = (void *) 0;

  omemo_bundle * bundle_p = (void *) 0;
  assert_int_equal(omemo_bundle_create(&bundle_p), 0);
  assert_int_not_equal(omemo_bundle_export(bundle_p, &publish), 0);

  assert_int_equal(omemo_bundle_add_pre_key(bundle_p, 14444488, data_p, len), 0);
  assert_int_not_equal(omemo_bundle_export(bundle_p, &publish), 0);

  assert_int_equal(omemo_bundle_set_signature(bundle_p, data_p, len), 0);
  assert_int_not_equal(omemo_bundle_export(bundle_p, &publish), 0);

  assert_int_equal(omemo_bundle_set_signed_pre_key(bundle_p, 1, data_p, len), 0);
  assert_int_not_equal(omemo_bundle_export(bundle_p, &publish), 0);

  assert_int_equal(omemo_bundle_set_identity_key(bundle_p, data_p, len), 0);
  assert_int_not_equal(omemo_bundle_export(bundle_p, &publish), 0);

  assert_int_equal(omemo_bundle_set_device_id(bundle_p, reg_id), 0);

  for (int i = 0; i <= 20; i++) {
    assert_int_equal(omemo_bundle_add_pre_key(bundle_p, i, data_p, len), 0);
  }

  assert_int_equal(bundle_p->pre_keys_amount, 22);

  assert_int_equal(omemo_bundle_export(bundle_p, &publish), 0);
  assert_ptr_not_equal(publish, (void *) 0);

  mxml_node_t * publish_node_p = mxmlLoadString((void *) 0, publish, MXML_OPAQUE_CALLBACK);
  assert_ptr_not_equal(publish_node_p, (void *) 0);

  mxml_node_t * item_node_p = mxmlGetFirstChild(publish_node_p);
  assert_ptr_not_equal(item_node_p, (void *) 0);
  assert_string_equal(mxmlGetElement(item_node_p), "item");

  mxml_node_t * bundle_node_p = mxmlGetFirstChild(item_node_p);
  assert_ptr_not_equal(bundle_node_p, (void *) 0);
  assert_string_equal(mxmlGetElement(bundle_node_p), "bundle");
  assert_string_equal(mxmlElementGetAttr(bundle_node_p, "xmlns"), "urn:xmpp:omemo:0");

  mxml_node_t * signed_pk_node_p = mxmlGetFirstChild(bundle_node_p);
  assert_ptr_not_equal(signed_pk_node_p, (void *) 0);
  assert_string_equal(mxmlGetElement(signed_pk_node_p), "signedPreKeyPublic");
  assert_string_equal(mxmlElementGetAttr(signed_pk_node_p, "signedPreKeyId"), "1");

  mxml_node_t * signature_node_p = mxmlGetNextSibling(signed_pk_node_p);
  assert_ptr_not_equal(signature_node_p, (void *) 0);
  assert_string_equal(mxmlGetElement(signature_node_p), "signedPreKeySignature");

  mxml_node_t * identity_key_node_p = mxmlGetNextSibling(signature_node_p);
  assert_ptr_not_equal(identity_key_node_p, (void *) 0);
  assert_string_equal(mxmlGetElement(identity_key_node_p), "identityKey");

  mxml_node_t * prekeys_node_p = mxmlGetNextSibling(identity_key_node_p);
  assert_ptr_not_equal(prekeys_node_p, (void *) 0);
  assert_string_equal(mxmlGetElement(prekeys_node_p), "prekeys");

  mxml_node_t * pre_key_node_p = mxmlGetFirstChild(prekeys_node_p);
  assert_ptr_not_equal(pre_key_node_p, (void *) 0);
  assert_string_equal(mxmlGetElement(pre_key_node_p), "preKeyPublic");
  assert_string_equal(mxmlElementGetAttr(pre_key_node_p, "preKeyId"), "14444488");

  pre_key_node_p = mxmlGetLastChild(prekeys_node_p);
  assert_ptr_not_equal(pre_key_node_p, (void *) 0);
  assert_string_equal(mxmlGetElement(pre_key_node_p), "preKeyPublic");
  assert_string_equal(mxmlElementGetAttr(pre_key_node_p, "preKeyId"), "20");

   mxmlDelete(publish_node_p);
}

void test_bundle_get_pep_node_name(void ** state) {
  (void) state;

  char * node_name = (void *) 0;
  assert_int_equal(omemo_bundle_get_pep_node_name(1337, &node_name), 0);
  assert_string_equal(node_name, "urn:xmpp:omemo:0:bundles:1337");

  free(node_name);
}

void test_bundle_import_malformed(void ** state) {
  (void) state;

  char * bundle_malformed_1 = "<items node='urn:xmpp:omemo:0:bundles:31415'>"
                                "<bundle xmlns='urn:xmpp:omemo:0'>"
                                  "<signedPreKeyPublic signedPreKeyId='1'>sWsAtQ==</signedPreKeyPublic>"
                                  "<signedPreKeySignature>sWsAtQ==</signedPreKeySignature>"
                                  "<identityKey>sWsAtQ==</identityKey>"
                                  "<prekeys>"
                                    "<preKeyPublic preKeyId='1'>sWsAtQ==</preKeyPublic>"
                                    "<preKeyPublic preKeyId='1'>sWsAtQ==</preKeyPublic>"
                                    "<preKeyPublic preKeyId='1'>sWsAtQ==</preKeyPublic>"
                                    "<preKeyPublic preKeyId='1'>sWsAtQ==</preKeyPublic>"
                                  "</prekeys>"
                                 "</bundle>"
                                "</item>"
                              "</items>";

  char * bundle_malformed_2 = "<items node='urn:xmpp:omemo:0:bundles:31415'>"
                                "<item>"
                                  "<bundle xmlns='urn:xmpp:omemo:0'>"
                                    "<signedPreKeySignature>sWsAtQ==</signedPreKeySignature>"
                                    "<identityKey>sWsAtQ==</identityKey>"
                                    "<prekeys>"
                                      "<preKeyPublic preKeyId='1'>sWsAtQ==</preKeyPublic>"
                                      "<preKeyPublic preKeyId='1'>sWsAtQ==</preKeyPublic>"
                                      "<preKeyPublic preKeyId='1'>sWsAtQ==</preKeyPublic>"
                                      "<preKeyPublic preKeyId='1'>sWsAtQ==</preKeyPublic>"
                                    "</prekeys>"
                                   "</bundle>"
                                  "</item>"
                                "</items>";

  char * bundle_malformed_3 = "<items node='urn:xmpp:omemo:0:bundles:31415'>"
                                "<item>"
                                  "<bundle xmlns='urn:xmpp:omemo:0'>"
                                    "<signedPreKeyPublic signedPreKeyId='1'>sWsAtQ==</signedPreKeyPublic>"
                                    "<identityKey>sWsAtQ==</identityKey>"
                                    "<prekeys>"
                                      "<preKeyPublic preKeyId='1'>sWsAtQ==</preKeyPublic>"
                                      "<preKeyPublic preKeyId='1'>sWsAtQ==</preKeyPublic>"
                                      "<preKeyPublic preKeyId='1'>sWsAtQ==</preKeyPublic>"
                                      "<preKeyPublic preKeyId='1'>sWsAtQ==</preKeyPublic>"
                                    "</prekeys>"
                                   "</bundle>"
                                  "</item>"
                                "</items>";

  char * bundle_malformed_4 = "<items node='urn:xmpp:omemo:0:bundles:31415'>"
                                "<item>"
                                  "<bundle xmlns='urn:xmpp:omemo:0'>"
                                    "<signedPreKeyPublic signedPreKeyId='1'>sWsAtQ==</signedPreKeyPublic>"
                                    "<signedPreKeySignature>sWsAtQ==</signedPreKeySignature>"
                                    "<prekeys>"
                                      "<preKeyPublic preKeyId='1'>sWsAtQ==</preKeyPublic>"
                                      "<preKeyPublic preKeyId='1'>sWsAtQ==</preKeyPublic>"
                                      "<preKeyPublic preKeyId='1'>sWsAtQ==</preKeyPublic>"
                                      "<preKeyPublic preKeyId='1'>sWsAtQ==</preKeyPublic>"
                                    "</prekeys>"
                                   "</bundle>"
                                  "</item>"
                                "</items>";

  char * bundle_malformed_5 = "<items node='urn:xmpp:omemo:0:bundles:31415'>"
                                "<item>"
                                  "<bundle xmlns='urn:xmpp:omemo:0'>"
                                    "<signedPreKeyPublic signedPreKeyId='1'>sWsAtQ==</signedPreKeyPublic>"
                                    "<signedPreKeySignature>sWsAtQ==</signedPreKeySignature>"
                                    "<identityKey>sWsAtQ==</identityKey>"
                                   "</bundle>"
                                  "</item>"
                                "</items>";

  omemo_bundle * bundle_p = (void *) 0;
  assert_int_not_equal(omemo_bundle_import(bundle_malformed_1, &bundle_p), 0);
  assert_int_not_equal(omemo_bundle_import(bundle_malformed_2, &bundle_p), 0);
  assert_int_not_equal(omemo_bundle_import(bundle_malformed_3, &bundle_p), 0);
  assert_int_not_equal(omemo_bundle_import(bundle_malformed_4, &bundle_p), 0);
  assert_int_not_equal(omemo_bundle_import(bundle_malformed_5, &bundle_p), 0);

}

void test_bundle_import(void ** state) {
  (void) state;

  omemo_bundle * bundle_p = (void *) 0;
  assert_int_equal(omemo_bundle_import(bundle, &bundle_p), 0);
  assert_string_equal(bundle_p->device_id, "31415");

  assert_ptr_not_equal(bundle_p->signed_pk_node_p, (void *) 0);
  assert_string_equal(mxmlGetElement(bundle_p->signed_pk_node_p), "signedPreKeyPublic");

  assert_ptr_not_equal(bundle_p->signature_node_p, (void *) 0);
  assert_string_equal(mxmlGetElement(bundle_p->signature_node_p), "signedPreKeySignature");

  assert_ptr_not_equal(bundle_p->identity_key_node_p, (void *) 0);
  assert_string_equal(mxmlGetElement(bundle_p->identity_key_node_p), "identityKey");

  assert_ptr_not_equal(bundle_p->pre_keys_node_p, (void *) 0);
  assert_string_equal(mxmlGetElement(bundle_p->pre_keys_node_p), "prekeys");

  assert_int_equal(bundle_p->pre_keys_amount, 4);

  omemo_bundle_destroy(bundle_p);
}

void test_bundle_get_device_id(void ** state) {
  (void) state;

  omemo_bundle * bundle_p = (void *) 0;
  assert_int_equal(omemo_bundle_import(bundle, &bundle_p), 0);
  assert_int_equal(omemo_bundle_get_device_id(bundle_p), 31415);

  omemo_bundle_destroy(bundle_p);
}

void test_bundle_get_signed_pre_key(void ** state) {
  (void) state;

  omemo_bundle * bundle_p = (void *) 0;
  assert_int_equal(omemo_bundle_import(bundle, &bundle_p), 0);

  uint32_t pre_key_id = 0;
  uint8_t * data_p = (void *) 0;
  size_t data_len;

  assert_int_equal(omemo_bundle_get_signed_pre_key(bundle_p, &pre_key_id, &data_p, &data_len), 0);
  assert_int_equal(pre_key_id, 1);
  assert_int_equal(data_len, 4);
  assert_memory_equal(&data[0], data_p, data_len);

  free(data_p);
  omemo_bundle_destroy(bundle_p);
}

void test_bundle_get_signature(void ** state) {
  (void) state;

  omemo_bundle * bundle_p = (void *) 0;
  assert_int_equal(omemo_bundle_import(bundle, &bundle_p), 0);

  uint8_t * data_p = (void *) 0;
  size_t data_len;

  assert_int_equal(omemo_bundle_get_signature(bundle_p, &data_p, &data_len), 0);
  assert_int_equal(data_len, 4);
  assert_memory_equal(&data[0], data_p, data_len);

  free(data_p);
  omemo_bundle_destroy(bundle_p);
}

void test_bundle_get_identity_key(void ** state) {
  (void) state;

  omemo_bundle * bundle_p = (void *) 0;
  assert_int_equal(omemo_bundle_import(bundle, &bundle_p), 0);

  uint8_t * data_p = (void *) 0;
  size_t data_len;

  assert_int_equal(omemo_bundle_get_identity_key(bundle_p, &data_p, &data_len), 0);
  assert_int_equal(data_len, 4);
  assert_memory_equal(&data[0], data_p, data_len);

  free(data_p);
  omemo_bundle_destroy(bundle_p);
}

void test_bundle_get_random_pre_key(void ** state) {
  (void) state;

  omemo_bundle * bundle_p = (void *) 0;
  assert_int_equal(omemo_bundle_import(bundle, &bundle_p), 0);

  uint32_t pre_key_id = 0;
  uint8_t * data_p = (void *) 0;
  size_t data_len;

  assert_int_equal(omemo_bundle_get_random_pre_key(bundle_p, &pre_key_id, &data_p, &data_len), 0);
  assert_int_equal(data_len, 4);
  assert_memory_equal(&data[0], data_p, data_len);

  free(data_p);
  omemo_bundle_destroy(bundle_p);
}

void test_message_prepare_encryption(void ** state) {
  (void) state;

  uint32_t sid = 1337;

  omemo_message * msg_p;
  assert_int_equal(omemo_message_prepare_encryption((void *) 0, sid, (void *) 0, (void *) 0), OMEMO_ERR_NULL);
  assert_int_equal(omemo_message_prepare_encryption(msg_out, sid, (void *) 0, (void *) 0), OMEMO_ERR_NULL);
  assert_int_equal(omemo_message_prepare_encryption(msg_out, sid, &crypto, (void *) 0), OMEMO_ERR_NULL);
  assert_int_equal(omemo_message_prepare_encryption("asdf", sid, &crypto, &msg_p), OMEMO_ERR_MALFORMED_XML);

  assert_int_equal(omemo_message_prepare_encryption(msg_out, sid, &crypto, &msg_p), 0);

  assert_ptr_not_equal(msg_p, (void *) 0);
  assert_ptr_not_equal(msg_p->message_node_p, (void *) 0);
  assert_ptr_not_equal(msg_p->header_node_p, (void *) 0);
  assert_ptr_not_equal(msg_p->payload_node_p, (void *) 0);
  assert_ptr_not_equal(msg_p->key_p, (void *) 0);
  assert_int_equal(msg_p->key_len, OMEMO_AES_128_KEY_LENGTH);

  assert_string_equal(mxmlGetElement(msg_p->message_node_p), "message");
  assert_string_equal(mxmlElementGetAttr(msg_p->message_node_p, "xmlns"), "jabber:client");
  assert_string_equal(mxmlElementGetAttr(msg_p->message_node_p, "type"), "chat");
  assert_string_equal(mxmlElementGetAttr(msg_p->message_node_p, "to"), "bob@example.com");
  assert_ptr_equal(mxmlGetFirstChild(msg_p->message_node_p), (void *) 0);

  assert_string_equal(mxmlGetElement(msg_p->header_node_p), "header");
  assert_string_equal(mxmlElementGetAttr(msg_p->header_node_p, "sid"), "1337");
  mxml_node_t * iv_node_p = mxmlGetFirstChild(msg_p->header_node_p);
  assert_ptr_not_equal(iv_node_p, (void *) 0);
  assert_string_not_equal(mxmlGetOpaque(iv_node_p), "");

  assert_string_equal(mxmlGetElement(msg_p->payload_node_p), "payload");
  assert_string_not_equal(mxmlGetOpaque(msg_p->payload_node_p), "");

  omemo_message_destroy(msg_p);
}

void test_message_prepare_encryption_with_extra_data(void ** state) {
  (void) state;

  uint32_t sid = 1337;

  omemo_message * msg_p;
  assert_int_equal(omemo_message_prepare_encryption(msg_out_extra, sid, &crypto, &msg_p), 0);

  assert_ptr_not_equal(msg_p, (void *) 0);
  assert_ptr_not_equal(msg_p->message_node_p, (void *) 0);
  assert_ptr_not_equal(msg_p->header_node_p, (void *) 0);
  assert_ptr_not_equal(msg_p->payload_node_p, (void *) 0);
  assert_ptr_not_equal(msg_p->key_p, (void *) 0);
  assert_int_equal(msg_p->key_len, OMEMO_AES_128_KEY_LENGTH);

  mxml_node_t * active_node_p = mxmlGetFirstChild(msg_p->message_node_p);
  assert_ptr_not_equal(active_node_p, (void *) 0);
  assert_string_equal(mxmlGetElement(active_node_p), "active");

  assert_string_equal(mxmlGetElement(msg_p->header_node_p), "header");
  assert_string_equal(mxmlElementGetAttr(msg_p->header_node_p, "sid"), "1337");
  mxml_node_t * iv_node_p = mxmlGetFirstChild(msg_p->header_node_p);
  assert_ptr_not_equal(iv_node_p, (void *) 0);
  assert_string_not_equal(mxmlGetOpaque(iv_node_p), "");

  assert_string_equal(mxmlGetElement(msg_p->payload_node_p), "payload");
  assert_string_not_equal(mxmlGetOpaque(msg_p->payload_node_p), "");

  omemo_message_destroy(msg_p);
}

void test_message_get_key(void ** state) {
  (void) state;

  uint32_t sid = 1234;

  omemo_message * msg_p;
  assert_int_equal(omemo_message_prepare_encryption(msg_out, sid, &crypto, &msg_p), 0);
  assert_ptr_not_equal(omemo_message_get_key(msg_p), (void *) 0);

  assert_int_equal(omemo_message_get_key_len(msg_p), OMEMO_AES_128_KEY_LENGTH);

  omemo_message_destroy(msg_p);
}

void test_message_get_encrypted_key(void ** state) {
  (void) state;

  char * msg = "<message to='bob@example.com' from='alice@example.com'>"
                 "<encrypted xmlns='urn:xmpp:omemo:0'>"
                   "<header sid='1111'>"
                     "<key rid='2222'>sWsAtQ==</key>"
                     "<iv>BASE64ENCODED</iv>"
                   "</header>"
                   "<payload>BASE64ENCODED</payload>"
                  "</encrypted>"
                  "<store xmlns='urn:xmpp:hints'/>"
                "</message>";

  omemo_message * msg_p;
  assert_int_equal(omemo_message_prepare_decryption(msg, &msg_p), 0);

  uint8_t * key_p;
  size_t key_len;
  assert_int_equal(omemo_message_get_encrypted_key(msg_p, 1111, &key_p, &key_len), 0);
  assert_ptr_equal(key_p, (void *) 0);

  assert_int_equal(omemo_message_get_encrypted_key(msg_p, 2222, &key_p, &key_len), 0);
  assert_int_equal(key_len, 4);
  assert_memory_equal(key_p, data, key_len);

  omemo_message_destroy(msg_p);
}

void test_message_get_encrypted_key_no_keys(void ** state) {
  (void) state;

  char * msg = "<message to='bob@example.com' from='alice@example.com'>"
                 "<encrypted xmlns='urn:xmpp:omemo:0'>"
                   "<header sid='1111'>"
                     "<iv>BASE64ENCODED</iv>"
                   "</header>"
                   "<payload>BASE64ENCODED</payload>"
                  "</encrypted>"
                  "<store xmlns='urn:xmpp:hints'/>"
                "</message>";

  omemo_message * msg_p;
  assert_int_equal(omemo_message_prepare_decryption(msg, &msg_p), 0);

  uint8_t * key_p;
  size_t key_len;
  assert_int_equal(omemo_message_get_encrypted_key(msg_p, 1111, &key_p, &key_len), 0);
  assert_ptr_equal(key_p, (void *) 0);

  omemo_message_destroy(msg_p);
}

void test_message_add_recipient(void ** state) {
  (void) state;

  uint32_t sid = 4321;
  uint32_t rid = 1234;

  omemo_message * msg_p;
  assert_int_equal(omemo_message_prepare_encryption(msg_out, sid, &crypto, &msg_p), 0);

  assert_int_equal(omemo_message_add_recipient((void *) 0, rid, (void *) 0, 0), OMEMO_ERR_NULL);
  assert_int_equal(omemo_message_add_recipient(msg_p, rid, (void *) 0, 0), OMEMO_ERR_NULL);

  assert_int_equal(omemo_message_add_recipient(msg_p, rid, &data[0], 4), 0);

  mxml_node_t * key_node_p = mxmlGetFirstChild(msg_p->header_node_p);
  assert_ptr_not_equal(key_node_p, (void *) 0);
  assert_string_equal(mxmlGetElement(key_node_p), "key");
  assert_string_equal(mxmlElementGetAttr(key_node_p, "rid"), "1234");
  assert_string_equal(mxmlGetOpaque(key_node_p), data_b64);

  assert_int_equal(omemo_message_add_recipient(msg_p, sid, &data[0], 4), 0);
  key_node_p = mxmlGetNextSibling(key_node_p);
  assert_ptr_not_equal(key_node_p, (void *) 0);

  mxml_node_t * iv_node_p = mxmlGetLastChild(msg_p->header_node_p);
  assert_ptr_not_equal(iv_node_p, (void *) 0);
  assert_string_equal(mxmlGetElement(iv_node_p), "iv");

  omemo_message_destroy(msg_p);
}

void test_message_export_encrypted(void ** state) {
  (void) state;

  uint32_t sid = 4321;
  uint32_t rid = 1234;

  omemo_message * msg_p;
  assert_int_equal(omemo_message_prepare_encryption(msg_out, sid, &crypto, &msg_p), 0);
  assert_int_equal(omemo_message_add_recipient(msg_p, rid, &data[0], 4), 0);

  char * xml;
  assert_int_equal(omemo_message_export_encrypted(msg_p, &xml), 0);

  mxml_node_t * message_node_p = mxmlLoadString((void *) 0, xml, MXML_OPAQUE_CALLBACK);
  assert_ptr_not_equal(message_node_p, (void *) 0);
  assert_string_equal(mxmlGetElement(message_node_p), "message");

  mxml_node_t * encrypted_node_p;
  assert_int_equal(expect_next_node(message_node_p, mxmlGetFirstChild, "encrypted", &encrypted_node_p), 0);
  assert_string_equal(mxmlElementGetAttr(encrypted_node_p, "xmlns"), "urn:xmpp:omemo:0");

  mxml_node_t * header_node_p;
  assert_int_equal(expect_next_node(encrypted_node_p, mxmlGetFirstChild, "header", &header_node_p), 0);
  assert_string_equal(mxmlElementGetAttr(header_node_p, "sid"), "4321");

  mxml_node_t * payload_node_p;
  assert_int_equal(expect_next_node(header_node_p, mxmlGetNextSibling, "payload", &payload_node_p), 0);

  mxml_node_t * store_node_p;
  assert_int_equal(expect_next_node(encrypted_node_p, mxmlGetNextSibling, "store", &store_node_p), 0);
  assert_string_equal(mxmlElementGetAttr(store_node_p, "xmlns"), "urn:xmpp:hints");

  mxmlDelete(message_node_p);
  free(xml);
  omemo_message_destroy(msg_p);
}

void test_message_export_encrypted_extra(void ** state) {
  (void) state;

  uint32_t sid = 4321;
  uint32_t rid = 1234;

  omemo_message * msg_p;
  assert_int_equal(omemo_message_prepare_encryption(msg_out_extra, sid, &crypto, &msg_p), 0);
  assert_int_equal(omemo_message_add_recipient(msg_p, rid, &data[0], 4), 0);

  char * xml;
  assert_int_equal(omemo_message_export_encrypted(msg_p, &xml), 0);

  mxml_node_t * message_node_p = mxmlLoadString((void *) 0, xml, MXML_OPAQUE_CALLBACK);
  assert_ptr_not_equal(message_node_p, (void *) 0);
  assert_string_equal(mxmlGetElement(message_node_p), "message");

  mxml_node_t * active_node_p;
  assert_int_equal(expect_next_node(message_node_p, mxmlGetFirstChild, "active", &active_node_p), 0);

  mxml_node_t * encrypted_node_p;
  assert_int_equal(expect_next_node(active_node_p, mxmlGetNextSibling, "encrypted", &encrypted_node_p), 0);
  assert_string_equal(mxmlElementGetAttr(encrypted_node_p, "xmlns"), "urn:xmpp:omemo:0");

  mxml_node_t * header_node_p;
  assert_int_equal(expect_next_node(encrypted_node_p, mxmlGetFirstChild, "header", &header_node_p), 0);
  assert_string_equal(mxmlElementGetAttr(header_node_p, "sid"), "4321");

  mxml_node_t * payload_node_p;
  assert_int_equal(expect_next_node(header_node_p, mxmlGetNextSibling, "payload", &payload_node_p), 0);

  mxml_node_t * store_node_p
  ;
  assert_int_equal(expect_next_node(encrypted_node_p, mxmlGetNextSibling, "store", &store_node_p), 0);
  assert_string_equal(mxmlElementGetAttr(store_node_p, "xmlns"), "urn:xmpp:hints");

  mxmlDelete(message_node_p);
  free(xml);
  omemo_message_destroy(msg_p);
}

void test_message_encrypt_decrypt(void ** state) {
  (void) state;

  uint32_t sid = 4321;
  uint32_t rid = 1234;

  omemo_message * msg_out_p;
  assert_int_equal(omemo_message_prepare_encryption(msg_out, sid, &crypto, &msg_out_p), 0);

  const uint8_t * key_p = omemo_message_get_key(msg_out_p);

  assert_int_equal(omemo_message_add_recipient(msg_out_p, rid, key_p, omemo_message_get_key_len(msg_out_p)), 0);

  char * xml_out;
  assert_int_equal(omemo_message_export_encrypted(msg_out_p, &xml_out), 0);

  omemo_message * msg_in_p;
  assert_int_equal(omemo_message_prepare_decryption(xml_out, &msg_in_p), 0);

  assert_ptr_not_equal(msg_in_p->header_node_p, (void *) 0);
  assert_string_equal(mxmlGetElement(msg_in_p->message_node_p), "message");
  assert_ptr_equal(mxmlGetFirstChild(msg_in_p->message_node_p), (void *) 0);

  assert_ptr_not_equal(msg_in_p->header_node_p, (void *) 0);
  assert_string_equal(mxmlGetElement(msg_in_p->header_node_p), "header");

  assert_ptr_not_equal(msg_in_p->payload_node_p, (void *) 0);
  assert_string_equal(mxmlGetElement(msg_in_p->payload_node_p), "payload");

  assert_ptr_equal(msg_in_p->key_p, (void *) 0);

  assert_int_equal(omemo_message_get_sender_id(msg_in_p), sid);

  uint8_t * key_retrieved_p;
  size_t key_retrieved_len;
  assert_int_equal(omemo_message_get_encrypted_key(msg_in_p, rid, &key_retrieved_p, &key_retrieved_len), 0);
  assert_int_equal(key_retrieved_len, OMEMO_AES_128_KEY_LENGTH);
  assert_memory_equal(key_p, key_retrieved_p, key_retrieved_len);

  char * xml_in;
  assert_int_equal(omemo_message_export_decrypted(msg_in_p, key_retrieved_p, key_retrieved_len, &crypto, &xml_in), 0);
  mxml_node_t * message_node_decrypted_p = mxmlLoadString((void *) 0, xml_in, MXML_NO_CALLBACK);
  assert_ptr_not_equal(message_node_decrypted_p, (void *) 0);
  mxml_node_t * body_text_node_p = mxmlFindPath(message_node_decrypted_p, "body");
  assert_string_equal(mxmlGetText(body_text_node_p, 0), "hello");

  omemo_message_destroy(msg_out_p);
  omemo_message_destroy(msg_in_p);
  free(xml_out);
  free(xml_in);
  free(key_retrieved_p);
}

void test_message_encrypt_decrypt_extra(void ** state) {
  (void) state;

  uint32_t sid = 4321;
  uint32_t rid = 1234;

  omemo_message * msg_out_p;
  assert_int_equal(omemo_message_prepare_encryption(msg_out_extra, sid, &crypto, &msg_out_p), 0);

  const uint8_t * key_p = omemo_message_get_key(msg_out_p);

  assert_int_equal(omemo_message_add_recipient(msg_out_p, rid, key_p, omemo_message_get_key_len(msg_out_p)), 0);

  char * xml_out;
  assert_int_equal(omemo_message_export_encrypted(msg_out_p, &xml_out), 0);

  omemo_message * msg_in_p;
  assert_int_equal(omemo_message_prepare_decryption(xml_out, &msg_in_p), 0);

  assert_ptr_not_equal(msg_in_p->message_node_p, (void *) 0);
  assert_string_equal(mxmlGetElement(msg_in_p->message_node_p), "message");

  mxml_node_t * active_node_p;
  assert_int_equal(expect_next_node(msg_in_p->message_node_p, mxmlGetFirstChild, "active", &active_node_p), 0);

  assert_ptr_not_equal(msg_in_p->header_node_p, (void *) 0);
  assert_string_equal(mxmlGetElement(msg_in_p->header_node_p), "header");

  assert_ptr_not_equal(msg_in_p->payload_node_p, (void *) 0);
  assert_string_equal(mxmlGetElement(msg_in_p->payload_node_p), "payload");

  assert_ptr_equal(msg_in_p->key_p, (void *) 0);

  assert_int_equal(omemo_message_get_sender_id(msg_in_p), sid);

  uint8_t * key_retrieved_p;
  size_t key_retrieved_len;
  assert_int_equal(omemo_message_get_encrypted_key(msg_in_p, rid, &key_retrieved_p, &key_retrieved_len), 0);
  assert_int_equal(key_retrieved_len, OMEMO_AES_128_KEY_LENGTH);
  assert_memory_equal(key_p, key_retrieved_p, key_retrieved_len);

  char * xml_in;
  assert_int_equal(omemo_message_export_decrypted(msg_in_p, key_retrieved_p, key_retrieved_len, &crypto, &xml_in), 0);
  mxml_node_t * message_node_decrypted_p = mxmlLoadString((void *) 0, xml_in, MXML_NO_CALLBACK);
  assert_ptr_not_equal(message_node_decrypted_p, (void *) 0);
  mxml_node_t * body_text_node_p = mxmlFindPath(message_node_decrypted_p, "body");
  assert_string_equal(mxmlGetText(body_text_node_p, 0), "hello");

  omemo_message_destroy(msg_out_p);
  omemo_message_destroy(msg_in_p);
  free(xml_out);
  free(xml_in);
  free(key_retrieved_p);
}

void test_message_get_names(void ** state) {
  (void) state;

  char * msg = "<message xmlns='jabber:client' type='chat' from='alice@example.com/hurr' to='bob@example.com'>"
                 "<body>hello</body>"
               "</message>";

  omemo_message * msg_p;
  assert_int_equal(omemo_message_prepare_encryption(msg, 1337, &crypto, &msg_p), 0);
  assert_string_equal(omemo_message_get_sender_name_full(msg_p), "alice@example.com/hurr");
  assert_string_equal(omemo_message_get_sender_name_bare(msg_p), "alice@example.com");
  assert_string_equal(omemo_message_get_recipient_name_full(msg_p), "bob@example.com");
  assert_string_equal(omemo_message_get_recipient_name_bare(msg_p), "bob@example.com");

  omemo_message_destroy(msg_p);
}

int main(void) {
  const struct CMUnitTest tests[] = {
      cmocka_unit_test(test_devicelist_create),
      cmocka_unit_test(test_devicelist_import),
      cmocka_unit_test(test_devicelist_import_empty),
      cmocka_unit_test(test_devicelist_add),
      cmocka_unit_test(test_devicelist_contains_id),
      cmocka_unit_test(test_devicelist_remove),
      cmocka_unit_test(test_devicelist_export),
      cmocka_unit_test(test_devicelist_get_pep_node_name),
      cmocka_unit_test(test_devicelist_get_id_list),
      cmocka_unit_test(test_devicelist_diff),

      cmocka_unit_test(test_bundle_create),
      cmocka_unit_test(test_bundle_set_device_id),
      cmocka_unit_test(test_bundle_set_signed_pre_key),
      cmocka_unit_test(test_bundle_set_signature),
      cmocka_unit_test(test_bundle_set_identity_key),
      cmocka_unit_test(test_bundle_add_pre_key),
      cmocka_unit_test(test_bundle_export),
      cmocka_unit_test(test_bundle_get_pep_node_name),
      cmocka_unit_test(test_bundle_import_malformed),
      cmocka_unit_test(test_bundle_import),
      cmocka_unit_test(test_bundle_get_device_id),
      cmocka_unit_test(test_bundle_get_signed_pre_key),
      cmocka_unit_test(test_bundle_get_signature),
      cmocka_unit_test(test_bundle_get_identity_key),
      cmocka_unit_test(test_bundle_get_random_pre_key),

      cmocka_unit_test(test_message_prepare_encryption),
      cmocka_unit_test(test_message_prepare_encryption_with_extra_data),
      cmocka_unit_test(test_message_get_key),
      cmocka_unit_test(test_message_get_encrypted_key),
      cmocka_unit_test(test_message_get_encrypted_key_no_keys),
      cmocka_unit_test(test_message_add_recipient),
      cmocka_unit_test(test_message_export_encrypted),
      cmocka_unit_test(test_message_export_encrypted_extra),
      cmocka_unit_test(test_message_encrypt_decrypt),
      cmocka_unit_test(test_message_encrypt_decrypt_extra),
      cmocka_unit_test(test_message_get_names)
  };

  return cmocka_run_group_tests(tests, NULL, NULL);
 }
