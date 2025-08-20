/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#include "string.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "hid_device_le_prf.h"

#include "user_list.h"
#include "ble_userlist_auth.h"

/* HID Report type */
#define HID_REPORT_TYPE_INPUT       1
#define HID_REPORT_TYPE_OUTPUT      2
#define HID_REPORT_TYPE_FEATURE     3

// Forward declarations for fingerprint functions (implemented in C++)
extern bool enrollFinger();
extern bool clearFingerprintDB();

static const char *TAG = "BLE_CUSTOM";

/// characteristic presentation information
struct prf_char_pres_fmt
{
    /// Unit (The Unit is a UUID)
    uint16_t unit;
    /// Description
    uint16_t description;
    /// Format
    uint8_t format;
    /// Exponent
    uint8_t exponent;
    /// Name space
    uint8_t name_space;
};

// HID report mapping table
static hid_report_map_t hid_rpt_map[HID_NUM_REPORTS];

// HID Report Map characteristic value
// Keyboard report descriptor (using format for Boot interface descriptor)
static const uint8_t hidReportMap[] = {
    // --- INIZIO SEZIONE TASTIERA ---
    0x05, 0x01, // Usage Pg (Generic Desktop)
    0x09, 0x06, // Usage (Keyboard)
    0xA1, 0x01, // Collection: (Application)
    0x85, 0x02, // Report Id (2)
    //
    0x05, 0x07, //   Usage Pg (Key Codes)
    0x19, 0xE0, //   Usage Min (224)
    0x29, 0xE7, //   Usage Max (231)
    0x15, 0x00, //   Log Min (0)
    0x25, 0x01, //   Log Max (1)
    //
    //   Modifier byte
    0x75, 0x01, //   Report Size (1)
    0x95, 0x08, //   Report Count (8)
    0x81, 0x02, //   Input: (Data, Variable, Absolute)
    //
    //   Reserved byte
    0x95, 0x01, //   Report Count (1)
    0x75, 0x08, //   Report Size (8)
    0x81, 0x01, //   Input: (Constant)
    //
    //   LED report
    0x05, 0x08, //   Usage Pg (LEDs)
    0x19, 0x01, //   Usage Min (1)
    0x29, 0x05, //   Usage Max (5)
    0x95, 0x05, //   Report Count (5)
    0x75, 0x01, //   Report Size (1)
    0x91, 0x02, //   Output: (Data, Variable, Absolute)
    //
    //   LED report padding
    0x95, 0x01, //   Report Count (1)
    0x75, 0x03, //   Report Size (3)
    0x91, 0x01, //   Output: (Constant)
    //
    //   Key arrays (6 bytes)
    0x95, 0x06, //   Report Count (6)
    0x75, 0x08, //   Report Size (8)
    0x15, 0x00, //   Log Min (0)
    0x25, 0x65, //   Log Max (101)
    0x05, 0x07, //   Usage Pg (Key Codes)
    0x19, 0x00, //   Usage Min (0)
    0x29, 0x65, //   Usage Max (101)
    0x81, 0x00, //   Input: (Data, Array)
    //
    0xC0, // End Collection
// --- FINE SEZIONE TASTIERA ---

#if (SUPPORT_REPORT_VENDOR == true)
    0x06, 0xFF, 0xFF, // Usage Page(Vendor defined)
    0x09, 0xA5,       // Usage(Vendor Defined)
    0xA1, 0x01,       // Collection(Application)
    0x85, 0x04,       // Report Id (4)
    0x09, 0xA6,       // Usage(Vendor defined)
    0x09, 0xA9,       // Usage(Vendor defined)
    0x75, 0x08,       // Report Size
    0x95, 0x7F,       // Report Count = 127 Btyes
    0x91, 0x02,       // Output(Data, Variable, Absolute)
    0xC0,             // End Collection
#endif

};

/// Battery Service Attributes Indexes
enum
{
    BAS_IDX_SVC,
    BAS_IDX_BATT_LVL_CHAR,
    BAS_IDX_BATT_LVL_VAL,
    BAS_IDX_BATT_LVL_NTF_CFG,
    BAS_IDX_BATT_LVL_PRES_FMT,
    BAS_IDX_NB,
};



// User Management Service UUID and Characteristic UUID
#define USER_MGMT_SERVICE_UUID      0xFFF0
#define USER_MGMT_CHAR_UUID         0xFFF1

static const uint16_t user_mgmt_svc = USER_MGMT_SERVICE_UUID;   
static const uint16_t user_mgmt_char = USER_MGMT_CHAR_UUID;

uint16_t user_mgmt_handle[USER_MGMT_IDX_NB];
uint16_t user_mgmt_conn_id = 0;
uint8_t user_mgmt_value[USER_MGMT_PAYLOAD_LEN] = {0};
int user_list_index = 0;

////////////////////////////////////////////////////////////////////

#define HI_UINT16(a) (((a) >> 8) & 0xFF)
#define LO_UINT16(a) ((a) & 0xFF)
#define PROFILE_NUM 1
#define PROFILE_APP_IDX 0

struct gatts_profile_inst
{
    esp_gatts_cb_t gatts_cb;
    uint16_t gatts_if;
    uint16_t app_id;
    uint16_t conn_id;
};

hidd_le_env_t hidd_le_env;

// HID report map length
uint8_t hidReportMapLen = sizeof(hidReportMap);
uint8_t hidProtocolMode = HID_PROTOCOL_MODE_REPORT;


// HID Information characteristic value
static const uint8_t hidInfo[HID_INFORMATION_LEN] = {
    LO_UINT16(0x0111), HI_UINT16(0x0111), // bcdHID (USB HID version)
    0x00,                                 // bCountryCode
    HID_KBD_FLAGS                         // Flags
};

// HID External Report Reference Descriptor
static uint16_t hidExtReportRefDesc = ESP_GATT_UUID_BATTERY_LEVEL;

// HID Report Reference characteristic descriptor, key input
static uint8_t hidReportRefKeyIn[HID_REPORT_REF_LEN] =
    {HID_RPT_ID_KEY_IN, HID_REPORT_TYPE_INPUT};

// HID Report Reference characteristic descriptor, LED output
static uint8_t hidReportRefLedOut[HID_REPORT_REF_LEN] =
    {HID_RPT_ID_LED_OUT, HID_REPORT_TYPE_OUTPUT};

#if (SUPPORT_REPORT_VENDOR == true)

static uint8_t hidReportRefVendorOut[HID_REPORT_REF_LEN] =
    {HID_RPT_ID_VENDOR_OUT, HID_REPORT_TYPE_OUTPUT};
#endif


/// hid Service uuid
static uint16_t hid_le_svc = ATT_SVC_HID;
uint16_t hid_count = 0;
esp_gatts_incl_svc_desc_t incl_svc = {0};

#define CHAR_DECLARATION_SIZE (sizeof(uint8_t))
/// the uuid definition
static const uint16_t primary_service_uuid = ESP_GATT_UUID_PRI_SERVICE;
static const uint16_t include_service_uuid = ESP_GATT_UUID_INCLUDE_SERVICE;
static const uint16_t character_declaration_uuid = ESP_GATT_UUID_CHAR_DECLARE;
static const uint16_t character_client_config_uuid = ESP_GATT_UUID_CHAR_CLIENT_CONFIG;
static const uint16_t hid_info_char_uuid = ESP_GATT_UUID_HID_INFORMATION;
static const uint16_t hid_report_map_uuid = ESP_GATT_UUID_HID_REPORT_MAP;
static const uint16_t hid_control_point_uuid = ESP_GATT_UUID_HID_CONTROL_POINT;
static const uint16_t hid_report_uuid = ESP_GATT_UUID_HID_REPORT;
static const uint16_t hid_proto_mode_uuid = ESP_GATT_UUID_HID_PROTO_MODE;
static const uint16_t hid_kb_input_uuid = ESP_GATT_UUID_HID_BT_KB_INPUT;
static const uint16_t hid_kb_output_uuid = ESP_GATT_UUID_HID_BT_KB_OUTPUT;
static const uint16_t hid_repot_map_ext_desc_uuid = ESP_GATT_UUID_EXT_RPT_REF_DESCR;
static const uint16_t hid_report_ref_descr_uuid = ESP_GATT_UUID_RPT_REF_DESCR;
/// the propoty definition
static const uint8_t char_prop_notify = ESP_GATT_CHAR_PROP_BIT_NOTIFY;
static const uint8_t char_prop_read = ESP_GATT_CHAR_PROP_BIT_READ;
static const uint8_t char_prop_write_nr = ESP_GATT_CHAR_PROP_BIT_WRITE_NR;
static const uint8_t char_prop_read_write = ESP_GATT_CHAR_PROP_BIT_WRITE | ESP_GATT_CHAR_PROP_BIT_READ;
static const uint8_t char_prop_read_notify = ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_NOTIFY;
static const uint8_t char_prop_read_write_notify = ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_WRITE | ESP_GATT_CHAR_PROP_BIT_NOTIFY;
static const uint8_t char_prop_read_write_write_nr = ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_WRITE | ESP_GATT_CHAR_PROP_BIT_WRITE_NR;

/// battary Service
static const uint16_t battary_svc = ESP_GATT_UUID_BATTERY_SERVICE_SVC;
static const uint16_t bat_lev_uuid = ESP_GATT_UUID_BATTERY_LEVEL;
static const uint8_t bat_lev_ccc[2] = {0x00, 0x00};
static const uint16_t char_format_uuid = ESP_GATT_UUID_CHAR_PRESENT_FORMAT;
static uint8_t battary_lev = 50;



/// Full HRS Database Description - Used to add attributes into the database
static const esp_gatts_attr_db_t user_mgmt_att_db[USER_MGMT_IDX_NB] = {
    // User management Service Declaration
    [USER_MGMT_IDX_SVC] = {
        {ESP_GATT_AUTO_RSP},
        {ESP_UUID_LEN_16, (uint8_t *)&primary_service_uuid,
        ESP_GATT_PERM_READ, sizeof(uint16_t), sizeof(user_mgmt_svc), (uint8_t *)&user_mgmt_svc}
    },
    // Characteristic Declaration
    [USER_MGMT_IDX_CHAR] = {
        {ESP_GATT_AUTO_RSP},
        {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid,
         ESP_GATT_PERM_READ, CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read_write_notify}
    },
    // Characteristic Value
    [USER_MGMT_IDX_VAL] = {
        {ESP_GATT_AUTO_RSP},
        {ESP_UUID_LEN_16, (uint8_t *)&user_mgmt_char,
        ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE, USER_MGMT_PAYLOAD_LEN, USER_MGMT_PAYLOAD_LEN, user_mgmt_value}
    },
    [USER_MGMT_IDX_CCC] = {
        {ESP_GATT_AUTO_RSP},
        {ESP_UUID_LEN_16, (uint8_t *)&character_client_config_uuid,
        ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE, sizeof(uint16_t), sizeof(uint16_t), (uint8_t[]){0x00, 0x00}}
    },
};
/// Full BAS Database Description - Used to add attributes into the database
static const esp_gatts_attr_db_t bas_att_db[BAS_IDX_NB] =
    {
        // Battary Service Declaration
        [BAS_IDX_SVC] = {
            {ESP_GATT_AUTO_RSP}, 
            {ESP_UUID_LEN_16, (uint8_t *)&primary_service_uuid, 
             ESP_GATT_PERM_READ, sizeof(uint16_t), sizeof(battary_svc), (uint8_t *)&battary_svc}
        },

        // Battary level Characteristic Declaration
        [BAS_IDX_BATT_LVL_CHAR] = {
            {ESP_GATT_AUTO_RSP}, 
            {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, 
             ESP_GATT_PERM_READ, CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read_notify}
        },

        // Battary level Characteristic Value
        [BAS_IDX_BATT_LVL_VAL] = {
            {ESP_GATT_AUTO_RSP}, 
            {ESP_UUID_LEN_16, (uint8_t *)&bat_lev_uuid, 
             ESP_GATT_PERM_READ, sizeof(uint8_t), sizeof(uint8_t), &battary_lev}
        },

        // Battary level Characteristic - Client Characteristic Configuration Descriptor
        [BAS_IDX_BATT_LVL_NTF_CFG] = {
            {ESP_GATT_AUTO_RSP}, 
            {ESP_UUID_LEN_16, (uint8_t *)&character_client_config_uuid, 
             ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE, sizeof(uint16_t), sizeof(bat_lev_ccc), (uint8_t *)bat_lev_ccc}
        },

        // Battary level report Characteristic Declaration
        [BAS_IDX_BATT_LVL_PRES_FMT] = {
            {ESP_GATT_AUTO_RSP}, 
            {ESP_UUID_LEN_16, (uint8_t *)&char_format_uuid, 
             ESP_GATT_PERM_READ, sizeof(struct prf_char_pres_fmt), 0, NULL}
        },
};

// HID Device GATT Database
// This database is used to define the attributes of the HID Device profile.
static esp_gatts_attr_db_t hidd_le_gatt_db[HIDD_LE_IDX_NB] = {
    // HID Service Declaration
    [HIDD_LE_IDX_SVC] = {
        {ESP_GATT_AUTO_RSP},
        {ESP_UUID_LEN_16, (uint8_t *)&primary_service_uuid,
         ESP_GATT_PERM_READ_ENCRYPTED, sizeof(uint16_t), sizeof(hid_le_svc), (uint8_t *)&hid_le_svc}
    },

    // Include Service
    [HIDD_LE_IDX_INCL_SVC] = {
        {ESP_GATT_AUTO_RSP},
        {ESP_UUID_LEN_16, (uint8_t *)&include_service_uuid,
         ESP_GATT_PERM_READ, sizeof(esp_gatts_incl_svc_desc_t),
         sizeof(esp_gatts_incl_svc_desc_t), (uint8_t *)&incl_svc}
    },

    // HID Information
    [HIDD_LE_IDX_HID_INFO_CHAR] = {
        {ESP_GATT_AUTO_RSP},
        {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid,
         ESP_GATT_PERM_READ, CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read}
    },
    [HIDD_LE_IDX_HID_INFO_VAL] = {
        {ESP_GATT_AUTO_RSP},
        {ESP_UUID_LEN_16, (uint8_t *)&hid_info_char_uuid,
         ESP_GATT_PERM_READ, sizeof(hids_hid_info_t), sizeof(hidInfo), (uint8_t *)&hidInfo}
    },

    // HID Control Point
    [HIDD_LE_IDX_HID_CTNL_PT_CHAR] = {
        {ESP_GATT_AUTO_RSP},
        {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid,
         ESP_GATT_PERM_READ, CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_write_nr}
    },
    [HIDD_LE_IDX_HID_CTNL_PT_VAL] = {
        {ESP_GATT_AUTO_RSP},
        {ESP_UUID_LEN_16, (uint8_t *)&hid_control_point_uuid,
         ESP_GATT_PERM_WRITE, sizeof(uint8_t), 0, NULL}
    },

    // Report Map
    [HIDD_LE_IDX_REPORT_MAP_CHAR] = {
        {ESP_GATT_AUTO_RSP},
        {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid,
         ESP_GATT_PERM_READ, CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read}
    },
    [HIDD_LE_IDX_REPORT_MAP_VAL] = {
        {ESP_GATT_AUTO_RSP},
        {ESP_UUID_LEN_16, (uint8_t *)&hid_report_map_uuid,
         ESP_GATT_PERM_READ, HIDD_LE_REPORT_MAP_MAX_LEN, sizeof(hidReportMap), (uint8_t *)&hidReportMap}
    },
    [HIDD_LE_IDX_REPORT_MAP_EXT_REP_REF] = {
        {ESP_GATT_AUTO_RSP},
        {ESP_UUID_LEN_16, (uint8_t *)&hid_repot_map_ext_desc_uuid,
         ESP_GATT_PERM_READ, sizeof(uint16_t), sizeof(uint16_t), (uint8_t *)&hidExtReportRefDesc}
    },

    // Protocol Mode
    [HIDD_LE_IDX_PROTO_MODE_CHAR] = {
        {ESP_GATT_AUTO_RSP},
        {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid,
         ESP_GATT_PERM_READ, CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read_write}
    },
    [HIDD_LE_IDX_PROTO_MODE_VAL] = {
        {ESP_GATT_AUTO_RSP},
        {ESP_UUID_LEN_16, (uint8_t *)&hid_proto_mode_uuid,
         (ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE), sizeof(uint8_t), sizeof(hidProtocolMode), (uint8_t *)&hidProtocolMode}
    },

    // --- Keyboard Input Report ---
    [HIDD_LE_IDX_REPORT_KEY_IN_CHAR] = {
        {ESP_GATT_AUTO_RSP},
        {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid,
         ESP_GATT_PERM_READ, CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read_notify}
    },
    [HIDD_LE_IDX_REPORT_KEY_IN_VAL] = {
        {ESP_GATT_AUTO_RSP},
        {ESP_UUID_LEN_16, (uint8_t *)&hid_report_uuid,
         ESP_GATT_PERM_READ, HIDD_LE_REPORT_MAX_LEN, 0, NULL}
    },
    [HIDD_LE_IDX_REPORT_KEY_IN_CCC] = {
        {ESP_GATT_AUTO_RSP},
        {ESP_UUID_LEN_16, (uint8_t *)&character_client_config_uuid,
         (ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE), sizeof(uint16_t), 0, NULL}
    },
    [HIDD_LE_IDX_REPORT_KEY_IN_REP_REF] = {
        {ESP_GATT_AUTO_RSP},
        {ESP_UUID_LEN_16, (uint8_t *)&hid_report_ref_descr_uuid,
         ESP_GATT_PERM_READ, sizeof(hidReportRefKeyIn), sizeof(hidReportRefKeyIn), hidReportRefKeyIn}
    },

    // --- LED Output Report ---
    [HIDD_LE_IDX_REPORT_LED_OUT_CHAR] = {
        {ESP_GATT_AUTO_RSP},
        {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid,
         ESP_GATT_PERM_READ, CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read_write_write_nr}
    },
    [HIDD_LE_IDX_REPORT_LED_OUT_VAL] = {
        {ESP_GATT_AUTO_RSP},
        {ESP_UUID_LEN_16, (uint8_t *)&hid_report_uuid,
         ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE, HIDD_LE_REPORT_MAX_LEN, 0, NULL}
    },
    [HIDD_LE_IDX_REPORT_LED_OUT_REP_REF] = {
        {ESP_GATT_AUTO_RSP},
        {ESP_UUID_LEN_16, (uint8_t *)&hid_report_ref_descr_uuid,
         ESP_GATT_PERM_READ, sizeof(hidReportRefLedOut), sizeof(hidReportRefLedOut), hidReportRefLedOut}
    },

#if (SUPPORT_REPORT_VENDOR == true)
    // --- Vendor Report ---
    [HIDD_LE_IDX_REPORT_VENDOR_OUT_CHAR] = {
        {ESP_GATT_AUTO_RSP},
        {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid,
         ESP_GATT_PERM_READ, CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read_write_notify}
    },
    [HIDD_LE_IDX_REPORT_VENDOR_OUT_VAL] = {
        {ESP_GATT_AUTO_RSP},
        {ESP_UUID_LEN_16, (uint8_t *)&hid_report_uuid,
         ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE, HIDD_LE_REPORT_MAX_LEN, 0, NULL}
    },
    [HIDD_LE_IDX_REPORT_VENDOR_OUT_REP_REF] = {
        {ESP_GATT_AUTO_RSP},
        {ESP_UUID_LEN_16, (uint8_t *)&hid_report_ref_descr_uuid,
         ESP_GATT_PERM_READ, sizeof(hidReportRefVendorOut), sizeof(hidReportRefVendorOut), hidReportRefVendorOut}
    },
#endif
};


static void hid_add_id_tbl(void);

void esp_hidd_prf_cb_hdl(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if,esp_ble_gatts_cb_param_t *param)
{
    last_interaction_time = xTaskGetTickCount();

    switch (event) {
    case ESP_GATTS_REG_EVT: {
        esp_ble_gap_config_local_icon(ESP_BLE_APPEARANCE_GENERIC_HID);
        esp_hidd_cb_param_t hidd_param;
        hidd_param.init_finish.state = param->reg.status;
        if (param->reg.app_id == HIDD_APP_ID)
        {
            hidd_le_env.gatt_if = gatts_if;
            if (hidd_le_env.hidd_cb != NULL)
            {
                (hidd_le_env.hidd_cb)(ESP_HIDD_EVENT_REG_FINISH, &hidd_param);
                hidd_le_create_service(hidd_le_env.gatt_if);
            }
        }
        if (param->reg.app_id == BATTRAY_APP_ID)
        {
            hidd_param.init_finish.gatts_if = gatts_if;
            if (hidd_le_env.hidd_cb != NULL)
            {
                (hidd_le_env.hidd_cb)(ESP_BAT_EVENT_REG, &hidd_param);
            }
        }

        break;
    }
    case ESP_GATTS_CONF_EVT:
    {
        break;
    }
    case ESP_GATTS_CREATE_EVT:
        break;
    case ESP_GATTS_CONNECT_EVT:
    {
        esp_hidd_cb_param_t cb_param = {0};
        ESP_LOGI(HID_LE_PRF_TAG, "HID connection establish, conn_id = %x", param->connect.conn_id);
        memcpy(cb_param.connect.remote_bda, param->connect.remote_bda, sizeof(esp_bd_addr_t));
        cb_param.connect.conn_id = param->connect.conn_id;
        hidd_clcb_alloc(param->connect.conn_id, param->connect.remote_bda);
        esp_ble_set_encryption(param->connect.remote_bda, ESP_BLE_SEC_ENCRYPT_NO_MITM);
        if (hidd_le_env.hidd_cb != NULL)
        {
            (hidd_le_env.hidd_cb)(ESP_HIDD_EVENT_BLE_CONNECT, &cb_param);
        }
        break;
    }
    case ESP_GATTS_DISCONNECT_EVT:
    {
        if (hidd_le_env.hidd_cb != NULL)
        {
            (hidd_le_env.hidd_cb)(ESP_HIDD_EVENT_BLE_DISCONNECT, NULL);
        }
        hidd_clcb_dealloc(param->disconnect.conn_id);
        break;
    }
    case ESP_GATTS_CLOSE_EVT:
        break;
    case ESP_GATTS_WRITE_EVT:
    {
        esp_hidd_cb_param_t cb_param = {0};
        if (param->write.handle == hidd_le_env.hidd_inst.att_tbl[HIDD_LE_IDX_REPORT_LED_OUT_VAL])
        {
            cb_param.led_write.conn_id = param->write.conn_id;
            cb_param.led_write.report_id = HID_RPT_ID_LED_OUT;
            cb_param.led_write.length = param->write.len;
            cb_param.led_write.data = param->write.value;
            (hidd_le_env.hidd_cb)(ESP_HIDD_EVENT_BLE_LED_REPORT_WRITE_EVT, &cb_param);
        }

        if (param->write.handle == user_mgmt_handle[USER_MGMT_IDX_VAL]) {
            // Gestisci la scrittura sulla caratteristica custom            
            #if DEBUG_PASSWD
            printf("Received write on custom characteristic: %.*s\n",  param->write.len, param->write.value);
            #endif

            if (param->write.len < 1) break;
            uint8_t cmd = param->write.value[0];
            uint8_t idx = param->write.value[1];
            
            if (!ble_userlist_is_authenticated()) {
                printf("[BLE] Accesso lista utenti negato: non autenticato!\n");                
                send_authenticated(false);                
                break;
            }

            // Aggiorna l'ID di connessione per le operazioni di gestione utenti
            user_mgmt_conn_id = param->write.conn_id;
            switch (cmd) {                  
                case RESET_USER_LIST: {
                    // Comando di reset della lista utenti                
                    userdb_clear();     
                    send_db_cleared();                    
                    break;
                }

                case ADD_NEW_USER:
                case EDIT_USER: {
                    // Comando di inserimento/modifica utente o password
                    if (param->write.len < 69) {
                        ESP_LOGE(TAG, "Inserimento: dati insufficienti\n");
                        break;
                    }

                    if (idx >= MAX_USERS) {
                        ESP_LOGE(TAG, "Indice utente non valido: %d", idx);
                        return;
                    }

                    size_t offset = 2; // Inizio dopo il comando e l'indice 
                    user_entry_t user;  
                    
                    memset(&user, 0, sizeof(user));
                    memcpy(user.label, (uint8_t*)&param->write.value[offset], MAX_LABEL_LEN);
                    offset += MAX_LABEL_LEN;

                    char plainPsw[MAX_PASSWORD_LEN] = { 0 };
                    memcpy(plainPsw, (const char *)&param->write.value[offset], MAX_PASSWORD_LEN);                    
                    offset += MAX_PASSWORD_LEN;
                    
                    // Encrypt the password before storing it
                    user.password_len = userdb_encrypt_password(plainPsw, user.password_enc);                
                    user.winlogin = (bool)param->write.value[offset++];
                    user.magicfinger = (bool)param->write.value[offset++];
                    user.fingerprint_id = (uint8_t)param->write.value[offset++];
                    user.login_type = (uint8_t)param->write.value[offset++];

                    if (idx < user_count) {
                        // Edit user
                        userdb_edit(idx, &user);
                    } else {
                        // Add new user
                        userdb_add(&user);
                    }                                        
                    break;
                }

                case REMOVE_USER: {
                    // Comando di cancellazione utente                            
                    userdb_remove(idx);                    
                    break;
                }

                case CLEAR_USER_DB: {
                    // Comando di cancellazione del database utenti
                    userdb_clear();                    
                    break;
                }                

                case ENROLL_FINGER: {    
                    // Comando di enroll fingerprint                    
                    printf("Enrolling new fingerprint...\n");                    
                    enrollFinger();                                        
                    break;
                }

                case CLEAR_LIBRARY: {    
                    // Comando di clear fingerprint DB                    
                    printf("Clearing fingerprint DB...\n");              
                    clearFingerprintDB();
                    break;
                }

                case GET_USERS_LIST: {                   
                    if (send_user_entry(idx) != -1) {
                        printf("Sending user %d\n", idx);
                    }
                    break;
                }
                
                default: {
                    printf("[BLE] Comando non riconosciuto: %02X\n", cmd);                    
                    break;
                }
            }
        }

#if (SUPPORT_REPORT_VENDOR == true)
        if (param->write.handle == hidd_le_env.hidd_inst.att_tbl[HIDD_LE_IDX_REPORT_VENDOR_OUT_VAL] &&
            hidd_le_env.hidd_cb != NULL)
        {
            cb_param.vendor_write.conn_id = param->write.conn_id;
            cb_param.vendor_write.report_id = HID_RPT_ID_VENDOR_OUT;
            cb_param.vendor_write.length = param->write.len;
            cb_param.vendor_write.data = param->write.value;
            (hidd_le_env.hidd_cb)(ESP_HIDD_EVENT_BLE_VENDOR_REPORT_WRITE_EVT, &cb_param);
        }
#endif
        break;
    }
    case ESP_GATTS_CREAT_ATTR_TAB_EVT:
    {
        if (param->add_attr_tab.num_handle == BAS_IDX_NB &&
            param->add_attr_tab.svc_uuid.uuid.uuid16 == ESP_GATT_UUID_BATTERY_SERVICE_SVC &&
            param->add_attr_tab.status == ESP_GATT_OK)
        {
            incl_svc.start_hdl = param->add_attr_tab.handles[BAS_IDX_SVC];
            incl_svc.end_hdl = incl_svc.start_hdl + BAS_IDX_NB - 1;
            ESP_LOGI(HID_LE_PRF_TAG, "%s(), start added the hid service to the stack database. incl_handle = %d",
                     __func__, incl_svc.start_hdl);
            esp_ble_gatts_create_attr_tab(hidd_le_gatt_db, gatts_if, HIDD_LE_IDX_NB, 0);
        }
        if (param->add_attr_tab.num_handle == HIDD_LE_IDX_NB &&
            param->add_attr_tab.status == ESP_GATT_OK)
        {
            memcpy(hidd_le_env.hidd_inst.att_tbl, param->add_attr_tab.handles,
                   HIDD_LE_IDX_NB * sizeof(uint16_t));
            ESP_LOGI(HID_LE_PRF_TAG, "hid svc handle = %x", hidd_le_env.hidd_inst.att_tbl[HIDD_LE_IDX_SVC]);
            hid_add_id_tbl();
            esp_ble_gatts_start_service(hidd_le_env.hidd_inst.att_tbl[HIDD_LE_IDX_SVC]);
        }
        // --- SERVIZIO CUSTOM USER MANAGEMENT  ---
        if (param->add_attr_tab.num_handle == USER_MGMT_IDX_NB &&
            param->add_attr_tab.svc_uuid.uuid.uuid16 == USER_MGMT_SERVICE_UUID &&
            param->add_attr_tab.status == ESP_GATT_OK)
        {
            memcpy(user_mgmt_handle, param->add_attr_tab.handles, USER_MGMT_IDX_NB * sizeof(uint16_t));
            ESP_LOGI(HID_LE_PRF_TAG, "UserMgmt svc handle = %x", user_mgmt_handle[USER_MGMT_IDX_SVC]);
            esp_ble_gatts_start_service(user_mgmt_handle[USER_MGMT_IDX_SVC]);
        }
        // else
        // {
        //     esp_ble_gatts_start_service(param->add_attr_tab.handles[0]);
        // }
        break;
    }

    default:
        break;
    }
}

void hidd_le_create_service(esp_gatt_if_t gatts_if)
{
    /* Here should added the battery service first, because the hid service should include the battery service.
       After finish to added the battery service then can added the hid service. */
    esp_ble_gatts_create_attr_tab(bas_att_db, gatts_if, BAS_IDX_NB, 0);
    esp_ble_gatts_create_attr_tab(user_mgmt_att_db, gatts_if, USER_MGMT_IDX_NB, 0);
}

void hidd_le_init(void)
{

    // Reset the hid device target environment
    memset(&hidd_le_env, 0, sizeof(hidd_le_env_t));
}

void hidd_clcb_alloc(uint16_t conn_id, esp_bd_addr_t bda)
{
    uint8_t i_clcb = 0;
    hidd_clcb_t *p_clcb = NULL;

    for (i_clcb = 0, p_clcb = hidd_le_env.hidd_clcb; i_clcb < HID_MAX_APPS; i_clcb++, p_clcb++)
    {
        if (!p_clcb->in_use)
        {
            p_clcb->in_use = true;
            p_clcb->conn_id = conn_id;
            p_clcb->connected = true;
            memcpy(p_clcb->remote_bda, bda, ESP_BD_ADDR_LEN);
            break;
        }
    }
    return;
}

bool hidd_clcb_dealloc(uint16_t conn_id)
{
    uint8_t i_clcb = 0;
    hidd_clcb_t *p_clcb = NULL;

    for (i_clcb = 0, p_clcb = hidd_le_env.hidd_clcb; i_clcb < HID_MAX_APPS; i_clcb++, p_clcb++)
    {
        memset(p_clcb, 0, sizeof(hidd_clcb_t));
        return true;
    }

    return false;
}

static struct gatts_profile_inst hid_profile_tab [PROFILE_NUM] = {
    [PROFILE_APP_IDX] = {
        .gatts_cb = esp_hidd_prf_cb_hdl,
        .gatts_if = ESP_GATT_IF_NONE, /* Not get the gatt_if, so initial is ESP_GATT_IF_NONE */
    },

};

static void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{
    if (event == ESP_GATTS_MTU_EVT) {
        ESP_LOGI(HID_LE_PRF_TAG, "ESP_GATTS_MTU_EVT, MTU %d", param->mtu.mtu);        
    }

    /* If event is register event, store the gatts_if for each profile */
    if (event == ESP_GATTS_REG_EVT) {
        if (param->reg.status == ESP_GATT_OK) {
            hid_profile_tab [PROFILE_APP_IDX].gatts_if = gatts_if;
            ESP_LOGI(HID_LE_PRF_TAG, "Reg app success, app_id %04x, gatts_if %d", param->reg.app_id, gatts_if);
        }
        else
        {
            ESP_LOGI(HID_LE_PRF_TAG, "Reg app failed, app_id %04x, status %d", param->reg.app_id, param->reg.status);
            return;
        }
    }
    
    for (int idx = 0; idx < PROFILE_NUM; idx++) {
        /* ESP_GATT_IF_NONE, not specify a certain gatt_if, need to call every profile cb function */
        if (gatts_if == ESP_GATT_IF_NONE || gatts_if == hid_profile_tab[idx].gatts_if)   {
            if (hid_profile_tab[idx].gatts_cb) {
                hid_profile_tab[idx].gatts_cb(event, gatts_if, param);
            }
        }
    }
    
}

esp_err_t hidd_register_cb(void)
{
    esp_err_t status;
    status = esp_ble_gatts_register_callback(gatts_event_handler);
    return status;
}

void hidd_set_attr_value(uint16_t handle, uint16_t val_len, const uint8_t *value)
{
    hidd_inst_t *hidd_inst = &hidd_le_env.hidd_inst;
    if (hidd_inst->att_tbl[HIDD_LE_IDX_HID_INFO_VAL] <= handle &&
        hidd_inst->att_tbl[HIDD_LE_IDX_REPORT_REP_REF] >= handle)
    {
        esp_ble_gatts_set_attr_value(handle, val_len, value);
    }
    else
    {
        ESP_LOGE(HID_LE_PRF_TAG, "%s error:Invalid handle value.", __func__);
    }
    return;
}

void hidd_get_attr_value(uint16_t handle, uint16_t *length, uint8_t **value)
{
    hidd_inst_t *hidd_inst = &hidd_le_env.hidd_inst;
    if (hidd_inst->att_tbl[HIDD_LE_IDX_HID_INFO_VAL] <= handle &&
        hidd_inst->att_tbl[HIDD_LE_IDX_REPORT_REP_REF] >= handle)
    {
        esp_ble_gatts_get_attr_value(handle, length, (const uint8_t **)value);
    }
    else
    {
        ESP_LOGE(HID_LE_PRF_TAG, "%s error:Invalid handle value.", __func__);
    }

    return;
}

static void hid_add_id_tbl(void)
{
    int idx = 0;

    // Key input report
    hid_rpt_map[idx].id = hidReportRefKeyIn[0];
    hid_rpt_map[idx].type = hidReportRefKeyIn[1];
    hid_rpt_map[idx].handle = hidd_le_env.hidd_inst.att_tbl[HIDD_LE_IDX_REPORT_KEY_IN_VAL];
    hid_rpt_map[idx].cccdHandle = hidd_le_env.hidd_inst.att_tbl[HIDD_LE_IDX_REPORT_KEY_IN_CCC];
    hid_rpt_map[idx].mode = HID_PROTOCOL_MODE_REPORT;
    idx++;

    // LED output report
    hid_rpt_map[idx].id = hidReportRefLedOut[0];
    hid_rpt_map[idx].type = hidReportRefLedOut[1];
    hid_rpt_map[idx].handle = hidd_le_env.hidd_inst.att_tbl[HIDD_LE_IDX_REPORT_LED_OUT_VAL];
    hid_rpt_map[idx].cccdHandle = 0;
    hid_rpt_map[idx].mode = HID_PROTOCOL_MODE_REPORT;
    idx++;

    hid_dev_register_reports(idx, hid_rpt_map);
}

