// This file will hold the authentication flag for BLE user list access
#include <stdbool.h>
#include "ble_userlist_auth.h"
#include "user_list.h"

static bool ble_userlist_authenticated = false;

void ble_userlist_set_authenticated(bool value) {
    ble_userlist_authenticated = value;
    send_authenticated(ble_userlist_authenticated);
}

bool ble_userlist_is_authenticated() {
    return ble_userlist_authenticated;
}