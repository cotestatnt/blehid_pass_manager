// #pragma once
// #ifndef SHARED_DATA_H
// #define SHARED_DATA_H

// #define NUM_PASSWORDS 3

// extern const char* password_list[NUM_PASSWORDS];
// extern volatile int password_index;

// // char account_name[32] = "Account1";
// // bool show_account_name = false;

// #endif


#pragma once
#ifndef SHARED_DATA_H
#define SHARED_DATA_H

#include <stdint.h>
#include <stddef.h>

#define MAX_LABEL_LEN    32
#define MAX_PASSWORD_LEN 32
#define MAX_USERS        16

typedef struct {
    char label[MAX_LABEL_LEN];         // es: username o descrizione
    uint8_t password_enc[MAX_PASSWORD_LEN]; // password crittografata
    size_t password_len;               // lunghezza reale della password cifrata
    uint32_t usage_count;              // frequenza di utilizzo
} user_entry_t;

extern user_entry_t user_list[MAX_USERS];
extern size_t user_count;
extern volatile int user_index;
extern int display_reset_pending;
extern uint32_t last_interaction_time;

#ifdef __cplusplus
extern "C" {
#endif

// Funzioni di crittografia password
int userdb_encrypt_password(const char* plain, uint8_t* out_encrypted, size_t* out_len) ;
int userdb_decrypt_password(const uint8_t* encrypted, size_t len, char* out_plain);

// Funzioni di gestione
void userdb_load();
void userdb_save();
int userdb_add(const char* label, const uint8_t* password_enc, size_t enc_len);
int userdb_remove(int index);
int userdb_update(int index, const char* new_label, const char* new_password);
void userdb_increment_usage(int index);
void userdb_sort_by_usage();

// Test data initialization
void userdb_init_test_data();
void userdb_dump();
void userdb_clear();

// Funzioni per l'invio della lista utenti al client BLE
void send_next_user_entry();

#ifdef __cplusplus
}
#endif


#endif