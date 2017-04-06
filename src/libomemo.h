/**
 * LIBOMEMO 0.6.0
 */


#pragma once

#include <inttypes.h>

#include <glib.h>

typedef struct omemo_bundle omemo_bundle;
typedef struct omemo_devicelist omemo_devicelist;
typedef struct omemo_message omemo_message;

typedef struct omemo_crypto_provider {
  /**
   * Gets cryptographically strong preudo-random bytes.
   *
   * @param buf_p Will point to a buffer with the output bytes. free() when done.
   * @param buf_len The length of the buffer, or rather how many random bytes are written into the buffer.
   * @param user_data_p Pointer to the user data set in the crypto provider.
   * @return 0 on success, negative on error.
   */
  int (*random_bytes_func)(uint8_t ** buf_pp, size_t buf_len, void * user_data_p);

  /**
   * Encrypts a byte buffer.
   *
   * @param plaintext_p Pointer to the plaintext buffer.
   * @param plaintext_len The length of the buffer.
   * @param iv_p Pointer to the IV buffer.
   * @param iv_len Length of the IV buffer.
   * @param key_p Pointer to the key buffer.
   * @param key_len Length of the key buffer.
   * @param tag_len Length of the tag to generate.
   * @param user_data_p Pointer to the user data set in the crypto provider.
   * @param ciphertext_pp Will point to a pointer to the ciphertext buffer.
   * @param ciphertext_len_p Will point to the length of the ciphertext buffer.
   * @param tag_pp Will point to a pointer to the tag buffer.
   * @return 0 on success, negative on error.
   */
  int (*aes_gcm_encrypt_func)(const uint8_t * plaintext_p, size_t plaintext_len,
                              const uint8_t * iv_p, size_t iv_len,
                              const uint8_t * key_p, size_t key_len,
                              size_t tag_len,
                              void * user_data_p,
                              uint8_t ** ciphertext_pp, size_t * ciphertext_len_p,
                              uint8_t ** tag_pp);

  /**
   * Decrypts a ciphertext byte buffer.
   *
   * @param ciphertext_p Pointer to the ciphertext buffer.
   * @param ciphertext_len Length of the ciphertext buffer.
   * @param iv_p Pointer to the IV buffer.
   * @param iv_len Length of the IV buffer.
   * @param key_p Pointer to the key buffer.
   * @param key_len Length of the key buffer.
   * @param tag_p Pointer to the tag buffer.
   * @param tag_len Length of the tag buffer.
   * @param user_data_p Pointer to the user data set in the crypto provider.
   * @param plaintext_pp Will point to a pointer to the plaintext.
   * @param Will point to the length of the plaintext buffer.
   * @return 0 on success, negative on error.
   */
  int (*aes_gcm_decrypt_func)(const uint8_t * ciphertext_p, size_t ciphertext_len,
                              const uint8_t * iv_p, size_t iv_len,
                              const uint8_t * key_p, size_t key_len,
                              uint8_t * tag_p, size_t tag_len,
                              void * user_data_p,
                              uint8_t ** plaintext_pp, size_t * plaintext_len_p);

  /**
   * Pointer to the user data that will be passed to the functions.
   */
  void * user_data_p;
} omemo_crypto_provider;

#define OMEMO_AES_128_KEY_LENGTH 16
#define OMEMO_AES_GCM_IV_LENGTH  16
#define OMEMO_AES_GCM_TAG_LENGTH 16

#define OMEMO_LOG_OFF    -1
#define OMEMO_LOG_ERROR   0
#define OMEMO_LOG_WARNING 1
#define OMEMO_LOG_NOTICE  2
#define OMEMO_LOG_INFO    3
#define OMEMO_LOG_DEBUG   4

#define OMEMO_SUCCESS                      0
#define OMEMO_ERR                     -10000
#define OMEMO_ERR_NOMEM               -10001
#define OMEMO_ERR_NULL                -10002
#define OMEMO_ERR_CRYPTO              -10010
#define OMEMO_ERR_AUTH_FAIL           -10020
#define OMEMO_ERR_UNSUPPORTED_KEY_LEN -10030
#define OMEMO_ERR_STORAGE             -10100
#define OMEMO_ERR_MALFORMED_BUNDLE    -11000
#define OMEMO_ERR_MALFORMED_XML       -12000

#define OMEMO_ADD_MSG_NONE 0
#define OMEMO_ADD_MSG_BODY 1
#define OMEMO_ADD_MSG_EME  2
#define OMEMO_ADD_MSG_BOTH 3

#define OMEMO_STRIP_ALL  1
#define OMEMO_STRIP_NONE 0

#define omemo_devicelist_list_data(X) (*((uint32_t *) X->data))

/*-------------------- BUNDLE --------------------*/

/**
 * Creates a fresh bundle.
 *
 * @param bundle_pp Will be set to the allocated bundle.
 * @return 0 on success, negative on error.
 */
int omemo_bundle_create(omemo_bundle ** bundle_pp);

/**
 * Sets the device id of a bundle.
 *
 * @param bundle_p Pointer to the bundle.
 * @param device_id The device id to set.
 * @return 0 on success, negative on error.
 */
int omemo_bundle_set_device_id(omemo_bundle * bundle_p, uint32_t device_id);

/**
 * Gets the device id of a bundle.
 *
 * @param bundle_p Pointer to the bundle.
 * @return The device id.
 */
uint32_t omemo_bundle_get_device_id(omemo_bundle * bundle_p);

/**
 * Sets the signed pre key of a bundle.
 *
 * @param bundle_p Pointer to the bundle.
 * @param pre_key_id The ID of the pre key.
 * @param data_p Pointer to the serialized key data.
 * @param data_len Length of the data.
 * @return 0 on success, negative on error.
 */
int omemo_bundle_set_signed_pre_key(omemo_bundle * bundle_p, uint32_t pre_key_id, uint8_t * data_p, size_t data_len);

/**
 * Gets the signed pre key from the specified bundle.
 *
 * @param bundle_p Pointer to the bundle.
 * @param pre_key_id_p Will be set to the pre key id as returned by strtol.
 * @param data_pp Will be set to a pointer to the serialized key data. Has to be free()d when done.
 * @param data_len_p Will be set to the length of the data.
 * @return 0 on success, negative on error.
 */
int omemo_bundle_get_signed_pre_key(omemo_bundle * bundle_p, uint32_t * pre_key_id_p, uint8_t ** data_pp, size_t * data_len_p);

/**
 * Sets the signature data belonging to the signed pre key.
 *
 * @param bundle_p Pointer to the bundle.
 * @param data_p Pointer to the serialized data.
 * @param data_len Length of the data.
 * @return 0 on success, negative on error.
 */
int omemo_bundle_set_signature(omemo_bundle * bundle_p, uint8_t * data_p, size_t data_len);

/**
 * Gets the signature from the specified bundle.
 *
 * @param bundle_p Pointer to the bundle.
 * @param data_pp Will be set to a pointer to the signature data. Has to be free()d when done.
 * @param data_len_p Will be set to the length of the data.
 * @return 0 on succcess, negative on error.
 */
int omemo_bundle_get_signature(omemo_bundle * bundle_p, uint8_t ** data_pp, size_t * data_len_p);

/**
 * Sets the identity key.
 *
 * @param bundle_p Pointer to the bundle.
 * @param data_p Pointer to the serialized key data.
 * @param data_len Length of the data.
 * @return 0 on success, negative on error.
 */
int omemo_bundle_set_identity_key(omemo_bundle * bundle_p, uint8_t * data_p, size_t data_len);

/**
 * Gets the identity key from the specified bundle.
 *
 * @param bundle_p Pointer to the bundle.
 * @param data_pp Will be set to a pointer to the identity key data. Has to be free()d when done.
 * @param data_len_p Will be set to the length of the data.
 * @return 0 on success, negative on error.
 */
int omemo_bundle_get_identity_key(omemo_bundle * bundle_p, uint8_t ** data_pp, size_t * data_len_p);

/**
 * Adds a pre key to the bundle.
 *
 * @param bundle_p Pointer to the bundle.
 * @param pre_key_id The ID of the pre key.
 * @param data_p Pointer to the serialized public key data.
 * @param data_len Length of the data.
 * @return 0 on success, negative on error.
 */
int omemo_bundle_add_pre_key(omemo_bundle * bundle_p, uint32_t pre_key_id, uint8_t * data_p, size_t data_len);

/**
 * Gets a random pre key from the specified bundle.
 *
 * @param bundle_p Pointer to the bundle.
 * @param pre_key_id_p Will be set to the ID of the selected pre key.
 * @param data_pp Will be set to a pointer to the serialized public key data. Has to be free()d when done.
 * @param data_len_p Will be set to the length of the data.
 * @return 0 on success, negative on error.
 */
int omemo_bundle_get_random_pre_key(omemo_bundle * bundle_p, uint32_t * pre_key_id_p, uint8_t ** data_pp, size_t * data_len_p);

/**
 * "Exports" the bundle into XML for publishing via PEP.
 *
 * @param bundle_p Pointer to the complete bundle with at least the minimum amount of prekeys.
 * @param publish Will be set to the XML string starting at the <publish> node.
 * @return 0 on success, negative on error.
 */
int omemo_bundle_export(omemo_bundle * bundle_p, char ** publish);

/**
 * Parses the received XML bundle information and returns a bundle struct to work with.
 * Does some basic validity checks and returns OMEMO_ERR_MALFORMED_XML if something is wrong.
 *
 * @param received_bundle The bundle XML, starting at the <items> node.
 * @param bundle_pp Will be set to the omemo_bundle, has to be freed when done.
 * @return 0 on success, negative on error.
 */
int omemo_bundle_import (const char * received_bundle, omemo_bundle ** bundle_pp);

/**
 * Get the node name of the bundle node.
 *
 * @param device_id The registration ID of the device whose bundle is to be requested.
 * @param node_name_p Will be set to the node name. Has to be free()d afterwards.
 * @return 0 on success, negative on error
 */
int omemo_bundle_get_pep_node_name(uint32_t device_id, char ** node_name_p);

/**
 * Frees the memory of everything contained in a bundle as well as the bundle itself.
 *
 * @param bundle_p Pointer to the bundle to free.
 */
void omemo_bundle_destroy(omemo_bundle * bundle_p);


/*-------------------- DEVICELIST --------------------*/

/**
 * Creates a fresh devicelist. Usually only useful if there are no other OMEMO devices yet.
 *
 * @param from Owner of the devices on the list ("from" attribute in the XML tag).
 * @param dl_pp Will be set to a pointer to the devicelist struct.
 * @return 0 on success, negative on error.
 */
int omemo_devicelist_create(const char * from, omemo_devicelist ** dl_pp);

/**
 * Imports a devicelist from an XML string to the internal representation.
 *
 * @param received_devicelist The devicelist as received by the PEP update, starting at the <items> node.
 * @param from The owner of the devicelist ("from" attribute of the <message> stanza).
 * @param dl_pp Will be set to the pointer to the created devicelist struct.
 * @return 0 on success, negative on error.
 */
int omemo_devicelist_import(char * received_devicelist, const char * from, omemo_devicelist ** dl_pp);

/**
 * Adds a device to a devicelist (e.g. the own device to the own list).
 *
 * @param dl_p Pointer to an initialized devicelist.
 * @param device_id The ID to add to the list.
 * @return 0 on success, negative on error.
 */
int omemo_devicelist_add(omemo_devicelist * dl_p, uint32_t device_id);

/**
 * Looks for the given device ID in the given device list.
 *
 * @param dl_p Pointer to an initialized devicelist.
 * @param device_id The device ID to look for.
 * @return 1 if the list contains the ID, 0 if it does not or is NULL.
 */
int omemo_devicelist_contains_id(const omemo_devicelist * dl_p, uint32_t device_id);

/**
 * Removes a device ID from the list.
 * If it is not contained in the list, nothing happens.
 *
 * @param dl_p Pointer to the devicelist.
 * @param device_id The ID to remove from the list.
 * @return 0 on success, negative on error.
 */
int omemo_devicelist_remove(omemo_devicelist * dl_p, uint32_t device_id);

/**
 * @return 1 if the devicelist containts no IDs, 0 otherwise.
 */
int omemo_devicelist_is_empty(omemo_devicelist * dl_p);

/**
 * Compares two devicelists.
 * If two empty devicelists are compared, the returned pointers will be NULL.
 * This does not mean that any of the pointers may be NULL.
 *
 * @param dl_a_p Pointer to devicelist A.
 * @param dl_b_p Pointer to devicelist B.
 * @param a_minus_b_pp Will be set to a list of IDs that are contained in A, but not in B (i.e. A\B).
 * @param b_minus_a_pp Will be set to a list of IDs that are contained in B, but not in A (i.e. B\A).
 * @return 0 on success, negative on error.
 */
int omemo_devicelist_diff(const omemo_devicelist * dl_a_p, const omemo_devicelist * dl_b_p, GList ** a_minus_b_pp, GList ** b_minus_a_pp);

/**
 * Exports the devicelist to an XML string, as needed to publish it via PEP.
 *
 * @param dl_p Pointer to an initialized devicelist.
 * @param xml_p Will point to the XML string (starting at the <publish> node).
 * @return 0 on success, negative on error.
 */
int omemo_devicelist_export(omemo_devicelist * dl_p, char ** xml_p);

/**
 * Returns a copy of the internally kept list of IDs for easy iterating.
 * Has to be freed using g_list_free_full(list_p, free).
 *
 * "data" is a pointer to the uint32_t, so to acces it either:
 *  - cast the pointer to an uint32_t * and then dereference it
 *  - use the omemo_devicelist_list_data() macro that does the same
 *
 * @param dl_p Pointer to the devicelist.
 * @return Pointer to the head of the list, which may be null.
 */
GList * omemo_devicelist_get_id_list(const omemo_devicelist * dl_p);

/**
 * Checks if the devicelist struct contains any IDs.
 *
 * @param dl_p Pointer to the devicelist.
 * @return 1 if it contains at least 1 ID, 0 if it does not (i.e. the id_list is NULL).
 */
int omemo_devicelist_has_id_list(const omemo_devicelist * dl_p);

/**
 * Returns the name of the devicelist owner.
 *
 * @param Pointer to the devicelist.
 * @return The saved name of the owner.
 */
const char * omemo_devicelist_get_owner(const omemo_devicelist * dl_p);

/**
 * Assembles the name of the devicelist PEP node.
 * As it is static, it can also be done statically from the constants.
 * This function only exists to have the same interface as for the bundle node name.
 *
 * @param node_name_p Will be set to a string that contains the necessary name.
 *                    Has to be free()d when done.
 * @return 0 on success, negative on error.
 */
int omemo_devicelist_get_pep_node_name(char ** node_name_p);

/**
 * Frees the memory used by a devicelist struct, and of all it contains.
 *
 * @param dl_p Pointer to the devicelist to destroy.
 */
void omemo_devicelist_destroy(omemo_devicelist * dl_p);


/*-------------------- MESSAGE --------------------*/

/**
 * Creates a message without a payload for use as a KeyTransportElement.
 *
 * @param sender_device_id The own device ID.
 * @param crypto_p Pointer to a crypro provider.
 * @param message_pp Will point to the created message struct.
 * @return 0 on success, negative on error.
 */
int omemo_message_create(uint32_t sender_device_id, const omemo_crypto_provider * crypto_p, omemo_message ** message_pp);

/**
 * Strips the message of XEP-0071: XHTML-IM <html> nodes, and additional <body> nodes which are valid
 * through different values for the xml:lang attribute.
 * Leaks plaintext if this is not done one way or the other and the clients supports these!
 *
 * @param msg_p Pointer to the omemo_message to strip of possible additional plaintext.
 * @return 0 on success, negative on error.
 */
int omemo_message_strip_possible_plaintext(omemo_message * msg_p);

/**
 * Prepares an intercepted <message> stanza for encryption.
 * This means it removes the <body> and encrypts the contained text, leaving everything else as it is.
 * Recipient devices have to be added to the resulting struct before it is exported back to xml.
 *
 * @param outgoing_message The intercepted <message> stanza.
 * @param sender_device_id The own device ID.
 * @param crypto_p Pointer to a crypto provider.
 * @param strip Either OMEMO_STRIP_ALL or OMEMO_STRIP_NONE. If the former, applies omemo_message_strip_possible_plaintext().
 * @param message_pp Will be set to a pointer to the message struct.
 * @return 0 on success, negative on error.
 */
int omemo_message_prepare_encryption(char * outgoing_message, uint32_t sender_device_id, const omemo_crypto_provider * crypto_p, int strip, omemo_message ** message_pp);

/**
 * Gets the symmetric encryption key and appended authentication tag from the message struct
 * so that both can be encrypted with the Signal session.
 *
 * Naturally only exists in an outgoing message.
 *
 * @param msg_p Pointer to the message struct.
 * @return Pointer to the key data, or null if it does not exist.
 */
const uint8_t * omemo_message_get_key(omemo_message * msg_p);

/**
 * Gets the length of the symmetric key and appended tag.
 * Again, this only makes sense on an outgoing message.
 *
 * At the moment, the length is fixed to 16 (128 bits) for the key, and another 16 for the tag.
 * In a KeyTransportElement, no tag exists.
 *
 * @param msg_p Pointer to the message struct.
 * @return Length of the key + tag in bytes.
 */
size_t omemo_message_get_key_len(omemo_message * msg_p);

/**
 * Add the encrypted symmetric key for a specific device id to the message.
 * Only makes sense on outgoing messages.
 *
 * @param msg_p Pointer to the message to add to.
 * @param device_id The recipient device ID.
 * @param encrypted_key_p The encrypted key data.
 * @param key_len Length of the encrypted key data.
 * @return 0 on success, negative on error.
 */
int omemo_message_add_recipient(omemo_message * msg_p, uint32_t device_id, const uint8_t * encrypted_key_p, size_t key_len);

/**
 * After all recipients have been added, this function can be used to export the resulting <message> stanza.
 * Also adds a <store> hint.
 *
 * @param msg_p Pointer to the message.
 * @param add_msg One of ADD_MSG_* constants. For optionally adding a body, EME, or both.
 * @param msg_xml Will be set to the resulting xml string. free() when done with it.
 * @return 0 on success, negative on error.
 */
int omemo_message_export_encrypted(omemo_message * msg_p, int add_msg, char ** msg_xml);

/**
 * Prepares an intercepted <message> stanza for decryption by parsing it.
 * Afterwards, the encrypted symmetric key can be retrieved, and the decrypted key used to decrypt the payload.
 *
 * @param incoming_message The incoming <message> stanza xml as a string.
 * @param msg_pp Will be set to the created message.
 * @return 0 on success, negative on error.
 */
int omemo_message_prepare_decryption(char * incoming_message, omemo_message ** msg_pp);

/**
 * Checks if the message has a payload, i.e. whether it is a MessageElement or KeyTransportElement.
 *
 * @param msg_p Pointer to the message.
 * @return 1 if has a payload, 0 if it does not.
 */
int omemo_message_has_payload(omemo_message * msg_p);

/**
 * Gets the sender's device id from an OMEMO message.
 *
 * @param msg_p Pointer to the message.
 * @return The sid.
 */
uint32_t omemo_message_get_sender_id(omemo_message * msg_p);

/**
 * Gets the sender's full JID.
 * Note that there is no "from" attribute in outgoing messages.
 *
 * @param msg_p Pointer to the message.
 * @return The full JID.
 */
const char * omemo_message_get_sender_name_full(omemo_message * msg_p);

/**
 * Gets the sender's bare JID.
 * Note that there is no "from" attribute in outgoing messages.
 *
 * @param msg_p Pointer to the message.
 * @return The bare JID. Has to be free()d.
 */
char * omemo_message_get_sender_name_bare(omemo_message * msg_p);

/**
 * Gets the recipient's full JID.
 *
 * @param msg_p Pointer to the message.
 * @return The full JID.
 */
const char * omemo_message_get_recipient_name_full(omemo_message * msg_p);

/**
 * Gets the recipient's bare JID.
 *
 * @param msg_p Pointer to the message.
 * @return The bare JID.
 */
char * omemo_message_get_recipient_name_bare(omemo_message * msg_p);

/**
 * Gets the encrypted key data for a specific device ID (usually your own so you can decrypt it).
 *
 * @param msg_p Pointer to the message.
 * @param own_device_id The device ID to get the encrypted key for.
 * @param key_pp Will be set to the encrypted key data, or NULL if there is no data for the specified ID.
 *               Has to be free()d.
 * @param key_len_p Will be set to length of encrypted key data.
 * @return 0 on success, negative on error. Note that success does not mean a key was found.
 */
int omemo_message_get_encrypted_key(omemo_message * msg_p, uint32_t own_device_id, uint8_t ** key_pp, size_t * key_len_p );

/**
 * Using the decrypted symmetric key, this method decrypts the payload and exports the original <message> stanza.
 *
 * @param msg_p Pointer to the message.
 * @param key_p Pointer to the decrypted symmetric key.
 * @param key_len Length of the key data.
 * @param crypto_p Pointer to the crypto provider.
 * @param msg_xml_p Will be set to the xml string.
 * @return 0 on success, negative on error.
 */
int omemo_message_export_decrypted(omemo_message * msg_p, uint8_t * key_p, size_t key_len, const omemo_crypto_provider * crypto_p, char ** msg_xml_p);

/**
 * Frees the memory of everything contained in the message struct as well as the struct itself.
 *
 * @param msg_p Pointer to the message.
 */
void omemo_message_destroy(omemo_message * msg_p);
