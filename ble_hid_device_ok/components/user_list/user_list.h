#pragma once
#ifndef SHARED_DATA_H
#define SHARED_DATA_H

#ifdef __cplusplus
extern "C" {
#endif


#include <stdint.h>
#include <stddef.h>

#define DEBUG_PASSWD 0

#define MAX_LABEL_LEN    32
#define MAX_PASSWORD_LEN 32
#define MAX_USERS        16

typedef struct {
    char label[MAX_LABEL_LEN];         // es: username o descrizione
    uint8_t password_enc[MAX_PASSWORD_LEN]; // password crittografata
    size_t password_len;               // lunghezza reale della password cifrata
    uint32_t usage_count;              // frequenza di utilizzo
    bool winlogin;                     // true se è un login Windows
} user_entry_t;

extern user_entry_t user_list[MAX_USERS];
extern size_t user_count;
extern volatile int user_index;
extern int display_reset_pending;
extern uint32_t last_interaction_time;



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

void userdb_set_username(int index, const char* username, size_t len);
void userdb_set_password(int index, const char* password, size_t len);
void userdb_set_winlogin(int index, bool winlogin);

// Test data initialization
void userdb_init_test_data();
void userdb_dump();
void userdb_clear();

// Funzioni per l'invio della lista utenti al client BLE
void send_winlogin(uint8_t index);
void send_user(uint8_t index);
void send_password(uint8_t index);    
void send_db_cleared();
void send_authenticated(bool auth);

void send_ble_message(const char* message, uint8_t type);
#ifdef __cplusplus
}
#endif


#endif