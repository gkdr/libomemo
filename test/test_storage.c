#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include <stdio.h>

#include "../src/libomemo.h"
#include "../src/libomemo.c"
#include "../src/libomemo_storage.c"

#define TEST_DB_PATH "test.sqlite"

int db_cleanup(void ** state) {
  (void) state;

  remove(TEST_DB_PATH);

  return 0;
}

void test_test(void ** state) {
  (void) state;

  int a = 1;

  printf("%s\n", STMT_PROLOG);
  printf("a: %i, -a: %i\n", a, -a);
}

void test_devicelist_save_id(void ** state) {
  (void) state;

  int ret_val = omemo_storage_user_device_id_save("alice", 1337, TEST_DB_PATH);
  assert_int_equal(ret_val, 0);

  sqlite3 * db_p;
  assert_int_equal(sqlite3_open(TEST_DB_PATH, &db_p), SQLITE_OK);

  char * stmt = "SELECT * FROM devicelists WHERE name IS 'alice';";
  sqlite3_stmt * pstmt_p;
  assert_int_equal(sqlite3_prepare_v2(db_p, stmt, -1, &pstmt_p, (void *) 0), SQLITE_OK);

  assert_int_equal(sqlite3_step(pstmt_p), SQLITE_ROW);
  assert_int_equal(sqlite3_column_int(pstmt_p, 1), 1337);
  assert_int_equal(sqlite3_step(pstmt_p), SQLITE_DONE);

  sqlite3_finalize(pstmt_p);
  sqlite3_close(db_p);
}

void test_devicelist_delete_id(void ** state){
  (void) state;

  assert_int_equal(omemo_storage_user_device_id_save("alice", 1111, TEST_DB_PATH), 0);
  assert_int_equal(omemo_storage_global_device_id_exists(1111, TEST_DB_PATH), 1);
  assert_int_equal(omemo_storage_user_device_id_delete("alice", 2222, TEST_DB_PATH), 0);
  assert_int_equal(omemo_storage_global_device_id_exists(1111, TEST_DB_PATH), 1);
  assert_int_equal(omemo_storage_user_device_id_delete("alice", 1111, TEST_DB_PATH), 0);
  assert_int_equal(omemo_storage_global_device_id_exists(1111, TEST_DB_PATH), 0);
}

void test_devicelist_retrieve(void ** state) {
  (void) state;

  omemo_devicelist * dl_p;
  GList * id_list_p;

  assert_int_equal(omemo_storage_user_devicelist_retrieve("alice", TEST_DB_PATH, &dl_p), 0);
  assert_ptr_not_equal(dl_p, (void *) 0);
  assert_ptr_equal(omemo_devicelist_get_id_list(dl_p), (void *) 0);
  omemo_devicelist_destroy(dl_p);

  assert_int_equal(omemo_storage_user_device_id_save("alice", 1337, TEST_DB_PATH), 0);
  assert_int_equal(omemo_storage_user_devicelist_retrieve("alice", TEST_DB_PATH, &dl_p), 0);

  assert_ptr_not_equal(dl_p, (void *) 0);
  id_list_p = omemo_devicelist_get_id_list(dl_p);
  assert_int_equal(omemo_devicelist_list_data(id_list_p), 1337);
  assert_ptr_equal(id_list_p->next, (void *) 0);
  g_list_free_full(id_list_p, free);
  omemo_devicelist_destroy(dl_p);

  assert_int_equal(omemo_storage_user_device_id_save("alice", 1338, TEST_DB_PATH), 0);
  assert_int_equal(omemo_storage_user_devicelist_retrieve("alice", TEST_DB_PATH, &dl_p), 0);
  assert_ptr_not_equal(dl_p, (void *) 0);
  id_list_p = omemo_devicelist_get_id_list(dl_p);
  assert_int_equal(omemo_devicelist_list_data(id_list_p), 1337);
  assert_int_equal(omemo_devicelist_list_data(id_list_p), 1337);
  assert_int_equal(omemo_devicelist_list_data(id_list_p->next), 1338);
  g_list_free_full(id_list_p, free);

  omemo_devicelist_destroy(dl_p);
}

void test_chatlist_save(void ** state) {
  (void) state;

  int ret_val = omemo_storage_chatlist_save("test", TEST_DB_PATH);
  assert_int_equal(ret_val, 0);

  sqlite3 * db_p;
  assert_int_equal(sqlite3_open(TEST_DB_PATH, &db_p), SQLITE_OK);

  char * stmt = "SELECT * FROM cl WHERE chat_name IS 'test';";
  sqlite3_stmt * pstmt_p;
  assert_int_equal(sqlite3_prepare_v2(db_p, stmt, -1, &pstmt_p, (void *) 0), SQLITE_OK);

  assert_int_equal(sqlite3_step(pstmt_p), SQLITE_ROW);
  assert_string_equal(sqlite3_column_text(pstmt_p, 0), "test");
  assert_int_equal(sqlite3_step(pstmt_p), SQLITE_DONE);

  sqlite3_finalize(pstmt_p);
  sqlite3_close(db_p);
}

void test_chatlist_exists(void ** state) {
  (void) state;

  assert_int_equal(omemo_storage_chatlist_exists("test", TEST_DB_PATH), 0);
  assert_int_equal(omemo_storage_chatlist_save("test", TEST_DB_PATH), 0);
  assert_int_equal(omemo_storage_chatlist_exists("test", TEST_DB_PATH), 1);
}

void test_chatlist_delete(void ** state) {
  (void) state;

  assert_int_equal(omemo_storage_chatlist_exists("test", TEST_DB_PATH), 0);
  assert_int_equal(omemo_storage_chatlist_save("test", TEST_DB_PATH), 0);
  assert_int_equal(omemo_storage_chatlist_exists("test", TEST_DB_PATH), 1);
  assert_int_equal(omemo_storage_chatlist_delete("test", TEST_DB_PATH), 0);
  assert_int_equal(omemo_storage_chatlist_exists("test", TEST_DB_PATH), 0);
}

void test_global_device_id_exists(void ** state) {
  (void) state;

  assert_int_equal(omemo_storage_global_device_id_exists(55555, TEST_DB_PATH), 0);
  assert_int_equal(omemo_storage_user_device_id_save("alice", 11111, TEST_DB_PATH), 0);
  assert_int_equal(omemo_storage_global_device_id_exists(55555, TEST_DB_PATH), 0);
  assert_int_equal(omemo_storage_user_device_id_save("alice", 11112, TEST_DB_PATH), 0);
  assert_int_equal(omemo_storage_global_device_id_exists(55555, TEST_DB_PATH), 0);
  assert_int_equal(omemo_storage_user_device_id_save("bob", 21111, TEST_DB_PATH), 0);
  assert_int_equal(omemo_storage_global_device_id_exists(55555, TEST_DB_PATH), 0);
  assert_int_equal(omemo_storage_user_device_id_save("bob", 55555, TEST_DB_PATH), 0);
  assert_int_equal(omemo_storage_global_device_id_exists(55555, TEST_DB_PATH), 1);
}

int main(void) {
  const struct CMUnitTest tests[] = {
      //cmocka_unit_test(test_test),
      cmocka_unit_test_teardown(test_devicelist_save_id, db_cleanup),
      cmocka_unit_test_teardown(test_devicelist_delete_id, db_cleanup),
      cmocka_unit_test_teardown(test_devicelist_retrieve, db_cleanup),
      cmocka_unit_test_teardown(test_chatlist_save, db_cleanup),
      cmocka_unit_test_teardown(test_chatlist_exists, db_cleanup),
      cmocka_unit_test_teardown(test_chatlist_delete, db_cleanup),
      cmocka_unit_test_teardown(test_global_device_id_exists, db_cleanup)
  };

  return cmocka_run_group_tests(tests, (void *) 0, (void *) 0);
 }
