/**
 * LIBOMEMO 0.5.0
 * Copyright (C) 2017
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <inttypes.h>
#include <stddef.h>
#include <stdio.h>

#include <sqlite3.h>

#include "libomemo.h"

#define xstr(s) str(s)
#define str(s) #s

#define DEVICELIST_TABLE_NAME "devicelists"
#define DEVICELIST_NAME_NAME "name"
#define DEVICELIST_ID_NAME "id"
#define DEVICELIST_ADDED_NAME "date_added"
#define DEVICELIST_LASTUSE_NAME "date_lastuse"
#define DEVICELIST_TRUST_STATUS_NAME "trust_status"

#define CHATLIST_TABLE_NAME "cl"
#define CHATLIST_CHAT_NAME_NAME "chat_name"

#define LURCH_TRUST_NONE 0

#define STMT_PROLOG "BEGIN TRANSACTION;"\
                    "CREATE TABLE IF NOT EXISTS " DEVICELIST_TABLE_NAME "("\
                    DEVICELIST_NAME_NAME " TEXT NOT NULL, "\
                    DEVICELIST_ID_NAME " INTEGER NOT NULL, "\
                    DEVICELIST_ADDED_NAME " TEXT NOT NULL, "\
                    DEVICELIST_LASTUSE_NAME " TEXT NOT NULL, "\
                    DEVICELIST_TRUST_STATUS_NAME " INTEGER NOT NULL, "\
                    "PRIMARY KEY("DEVICELIST_NAME_NAME", "DEVICELIST_ID_NAME"));"\
                    "CREATE TABLE IF NOT EXISTS " CHATLIST_TABLE_NAME " ("\
                    CHATLIST_CHAT_NAME_NAME " TEXT PRIMARY KEY);"

#define STMT_EPILOG "COMMIT TRANSACTION;"


static int db_conn_open_and_prepare(sqlite3 ** db_pp, sqlite3_stmt ** pstmt_pp, const char * stmt, const char * db_fn) {
  int ret_val = 0;

  sqlite3 * db_p = (void *) 0;
  sqlite3_stmt * pstmt_p = (void *) 0;
  char * err_msg = (void *) 0;


  ret_val = sqlite3_open(db_fn, &db_p);
  if (ret_val) {
    ret_val = -ret_val;
    goto cleanup;
  }

  (void) sqlite3_exec(db_p, STMT_PROLOG, (void *) 0, (void *) 0, &err_msg);
  if (err_msg) {
    ret_val = OMEMO_ERR_STORAGE;
    goto cleanup;
  }

  ret_val = sqlite3_prepare_v2(db_p, stmt, -1, &pstmt_p, (void *) 0);
  if (ret_val) {
    ret_val = -ret_val;
    goto cleanup;
  }

  *db_pp = db_p;
  *pstmt_pp = pstmt_p;

cleanup:
  if (ret_val) {
    sqlite3_finalize(pstmt_p);
    sqlite3_close(db_p);
    sqlite3_free(err_msg);
  }

  return ret_val;
}

static int db_conn_commit(sqlite3 * db_p) {
  if (!db_p) {
    return OMEMO_ERR_NULL;
  }

  char * err_msg = (void *) 0;
  (void) sqlite3_exec(db_p, STMT_EPILOG, (void *) 0, (void *) 0, &err_msg);
  if (err_msg) {
    sqlite3_free(err_msg);
    return OMEMO_ERR_STORAGE;
  }

  return 0;
}

int omemo_storage_user_device_id_save(const char * user, uint32_t device_id, const char * db_fn) {
  const char * stmt = "INSERT INTO " DEVICELIST_TABLE_NAME " VALUES("
                    "?1, "
                    "?2, "
                    "datetime('now'), "
                    "datetime('now'), "
                    xstr(LURCH_TRUST_NONE)
                 ");";

  int ret_val = 0;

  sqlite3 * db_p = (void *) 0;
  sqlite3_stmt * pstmt_p = (void *) 0;

  ret_val = db_conn_open_and_prepare(&db_p, &pstmt_p, stmt, db_fn);
  if (ret_val) {
    goto cleanup;
  }

  ret_val = sqlite3_bind_text(pstmt_p, 1, user, -1, SQLITE_STATIC);
  if (ret_val) {
    ret_val = -ret_val;
    goto cleanup;
  }

  ret_val = sqlite3_bind_int(pstmt_p, 2, device_id);
  if (ret_val) {
    ret_val = -ret_val;
    goto cleanup;
  }

  ret_val = sqlite3_step(pstmt_p);
  if (ret_val != SQLITE_DONE) {
    ret_val = -ret_val;
    goto cleanup;
  }

  ret_val = db_conn_commit(db_p);
  if (ret_val) {
    goto cleanup;
  }


cleanup:
  sqlite3_finalize(pstmt_p);
  sqlite3_close(db_p);

  return ret_val;
}

int omemo_storage_user_device_id_delete(const char * user, uint32_t device_id, const char * db_fn) {
  const char * stmt = "DELETE FROM " DEVICELIST_TABLE_NAME
                      " WHERE " DEVICELIST_NAME_NAME " IS ?1"
                      " AND " DEVICELIST_ID_NAME " IS ?2;";

  int ret_val = 0;

  sqlite3 * db_p = (void *) 0;
  sqlite3_stmt * pstmt_p = (void *) 0;

  ret_val = db_conn_open_and_prepare(&db_p, &pstmt_p, stmt, db_fn);
  if (ret_val) {
    goto cleanup;
  }

  ret_val = sqlite3_bind_text(pstmt_p, 1, user, -1, SQLITE_STATIC);
  if (ret_val) {
    ret_val = -ret_val;
    goto cleanup;
  }

  ret_val = sqlite3_bind_int(pstmt_p, 2, device_id);
  if (ret_val) {
    ret_val = -ret_val;
    goto cleanup;
  }

  ret_val = sqlite3_step(pstmt_p);
  if (ret_val != SQLITE_DONE) {
    ret_val = -ret_val;
    goto cleanup;
  }

  ret_val = db_conn_commit(db_p);
  if (ret_val) {
    goto cleanup;
  }

cleanup:
  sqlite3_finalize(pstmt_p);
  sqlite3_close(db_p);

  return ret_val;
}

int omemo_storage_user_devicelist_retrieve(const char * user, const char * db_fn, omemo_devicelist ** dl_pp) {
  const char * stmt = "SELECT * FROM " DEVICELIST_TABLE_NAME " WHERE name IS ?1;";

  int ret_val = 0;

  omemo_devicelist * dl_p = (void *) 0;
  sqlite3 * db_p = (void *) 0;
  sqlite3_stmt * pstmt_p = (void *) 0;

  ret_val = omemo_devicelist_create(user, &dl_p);
  if (ret_val) {
    goto cleanup;
  }

  ret_val = db_conn_open_and_prepare(&db_p, &pstmt_p, stmt, db_fn);
  if (ret_val) {
    goto cleanup;
  }

  ret_val = sqlite3_bind_text(pstmt_p, 1, user, -1, SQLITE_STATIC);
  if (ret_val) {
    ret_val = -ret_val;
    goto cleanup;
  }

  ret_val = sqlite3_step(pstmt_p);
  while (ret_val == SQLITE_ROW) {
    ret_val = omemo_devicelist_add(dl_p, sqlite3_column_int(pstmt_p, 1));
    if (ret_val) {
      goto cleanup;
    }

    ret_val = sqlite3_step(pstmt_p);
  }

  ret_val = db_conn_commit(db_p);
  if (ret_val) {
    goto cleanup;
  }

  *dl_pp = dl_p;

cleanup:
  if (ret_val) {
    omemo_devicelist_destroy(dl_p);
  }
  sqlite3_finalize(pstmt_p);
  sqlite3_close(db_p);

  return ret_val;
}

int omemo_storage_chatlist_save(const char * chat, const char * db_fn) {
  const char * stmt = "INSERT OR REPLACE INTO " CHATLIST_TABLE_NAME " VALUES(?1);";

  int ret_val = 0;

  sqlite3 * db_p = (void *) 0;
  sqlite3_stmt * pstmt_p = (void *) 0;

  ret_val = db_conn_open_and_prepare(&db_p, &pstmt_p, stmt, db_fn);
  if (ret_val) {
    goto cleanup;
  }

  ret_val = sqlite3_bind_text(pstmt_p, 1, chat, -1, SQLITE_STATIC);
  if (ret_val) {
    ret_val = -ret_val;
    goto cleanup;
  }

  ret_val = sqlite3_step(pstmt_p);
  if (ret_val != SQLITE_DONE) {
    ret_val = -ret_val;
    goto cleanup;
  }

  ret_val = db_conn_commit(db_p);
  if (ret_val) {
    goto cleanup;
  }


cleanup:
  sqlite3_finalize(pstmt_p);
  sqlite3_close(db_p);

  return ret_val;
}

int omemo_storage_chatlist_exists(const char * chat, const char * db_fn) {
  const char * stmt = "SELECT " CHATLIST_CHAT_NAME_NAME " FROM " CHATLIST_TABLE_NAME
                      " WHERE " CHATLIST_CHAT_NAME_NAME " IS ?1;";

  int ret_val = 0;

  sqlite3 * db_p = (void *) 0;
  sqlite3_stmt * pstmt_p = (void *) 0;

  ret_val = db_conn_open_and_prepare(&db_p, &pstmt_p, stmt, db_fn);
  if (ret_val) {
    goto cleanup;
  }

  ret_val = sqlite3_bind_text(pstmt_p, 1, chat, -1, SQLITE_STATIC);
  if (ret_val) {
    ret_val = -ret_val;
    goto cleanup;
  }

  ret_val = sqlite3_step(pstmt_p);
  if (ret_val != SQLITE_ROW) {
    ret_val = (ret_val == SQLITE_DONE) ? 0 : -ret_val;
  } else {
    ret_val = 1;
  }

cleanup:
  sqlite3_finalize(pstmt_p);
  sqlite3_close(db_p);

  return ret_val;
}

int omemo_storage_chatlist_delete(const char * chat, const char * db_fn) {
  const char * stmt = "DELETE FROM " CHATLIST_TABLE_NAME " WHERE " CHATLIST_CHAT_NAME_NAME " IS ?1;";

  int ret_val = 0;

  sqlite3 * db_p = (void *) 0;
  sqlite3_stmt * pstmt_p = (void *) 0;

  ret_val = db_conn_open_and_prepare(&db_p, &pstmt_p, stmt, db_fn);
  if (ret_val) {
    goto cleanup;
  }

  ret_val = sqlite3_bind_text(pstmt_p, 1, chat, -1, SQLITE_STATIC);
  if (ret_val) {
    ret_val = -ret_val;
    goto cleanup;
  }

  ret_val = sqlite3_step(pstmt_p);
  if (ret_val != SQLITE_DONE) {
    ret_val = -ret_val;
    goto cleanup;
  }

  ret_val = db_conn_commit(db_p);
  if (ret_val) {
    goto cleanup;
  }

cleanup:
  sqlite3_finalize(pstmt_p);
  sqlite3_close(db_p);

  return ret_val;
}


int omemo_storage_global_device_id_exists(uint32_t device_id, const char * db_fn) {
  const char * stmt = "SELECT " DEVICELIST_ID_NAME " FROM " DEVICELIST_TABLE_NAME
                      " WHERE " DEVICELIST_ID_NAME " IS ?1;";

  int ret_val = 0;

  sqlite3 * db_p = (void *) 0;
  sqlite3_stmt * pstmt_p = (void *) 0;

  ret_val = db_conn_open_and_prepare(&db_p, &pstmt_p, stmt, db_fn);
  if (ret_val) {
    goto cleanup;
  }

  ret_val = sqlite3_bind_int(pstmt_p, 1, device_id);
  if (ret_val) {
    ret_val = -ret_val;
    goto cleanup;
  }

  ret_val = sqlite3_step(pstmt_p);
  if (ret_val != SQLITE_ROW) {
    ret_val = (ret_val == SQLITE_DONE) ? 0 : -ret_val;
  } else {
    ret_val = 1;
  }

cleanup:
  sqlite3_finalize(pstmt_p);
  sqlite3_close(db_p);

  return ret_val;
}
