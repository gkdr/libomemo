#pragma once

#include <inttypes.h>

#include "libomemo.h"

  /*
   * Saves a device ID for a username.
   *
   * @param user Owner of the devicelist.
   * @param device_id The device ID.
   * @param db_fn Path to the DB.
   * @return 0 on success, negative on error (e.g. negated SQLite3 error codes).
   */
int omemo_storage_user_device_id_save(const char * user, uint32_t device_id, const char * db_fn);

/**
 * Deletes a device ID for a username.
 *
 * @param user Owner of the devicelist.
 * @param device_id The device ID.
 * @param db_fn Path to the DB.
 * @return 0 on success, negative on error.
 */
int omemo_storage_user_device_id_delete(const char * user, uint32_t device_id, const char * db_fn);

/**
 * Retrieves the list of devices for a user.
 * If the function returns success but the ID list inside the devicelist is empty, there are no entries.
 *
 * @param user User to look for.
 * @param db_fn Path to the DB.
 * @param dl_pp Will be set to the devicelist.
 * @return 0 on success, negative on error (e.g. negated SQLite3 error codes).
 */
int omemo_storage_user_devicelist_retrieve(const char * user, const char * db_fn, omemo_devicelist ** dl_pp);

/**
 * Saves a chat to "the list" (used as whitelist for groupchats, and blacklist for normal chats).
 *
 * @param chat The name of the chat.
 * @param db_fn Path to the DB.
 * @return 0 on success, negative on error
 */
int omemo_storage_chatlist_save(const char * chat, const char * db_fn);

/**
 * Looks up a chat in "the list".
 *
 * @param chat The name of the chat.
 * @param db_fn Path to the DB.
 * @return 1 if it exists, 0 if it does not, negative on error.
 */
int omemo_storage_chatlist_exists(const char * chat, const char * db_fn);

/**
 * Deletes a chat from "the list".
 *
 * @param chat The name of the chat.
 * @param db_fn Path to the DB.
 * @return 0 on success, negative on error.
 */
int omemo_storage_chatlist_delete(const char * chat, const char * db_fn);

/**
 * Checks if the device ID is contained in the database.
 *
 * @param device_id The device ID to look for.
 * @param db_fn Path to the omemo DB.
 * @return 1 if true, 0 if false, negative on error
 */
int omemo_storage_global_device_id_exists(uint32_t device_id, const char * db_fn);
