/*********************************************************************
 This is an example for our nRF52 based Bluefruit LE modules

 Pick one up today in the adafruit shop!

 Adafruit invests time and resources providing this open source code,
 please support Adafruit and open-source hardware by purchasing
 products from Adafruit!

 MIT license, check LICENSE for more information
 All text above, and the splash screen below must be included in
 any redistribution
*********************************************************************/
#include <bluefruit.h>
#include "keycodes.h"

BLEBas  blebas;   // BLE Battery service
BLEDis bledis;
BLEHidAdafruit blehid;

bool hasKeyPressed = false;
#define BAT_CHARGE_STATE 23
#define VBAT_RATIO 1510 / 533        // Decrease ratio if Vbat measured with multimeter > Vbat reported
#define VBAT_SCALING 3.6 / 1024      // 3.6 reference and 10 bit resolution

#define MIN_BAT_VOLTAGE 3.0
#define MAX_BAT_VOLTAGE 4.2
#define BATTERY_FULL_SCALE (MAX_BAT_VOLTAGE - MIN_BAT_VOLTAGE)

// Stringhe personalizzabili per il test
String stringa_a = "-|-£=?;'";
String stringa_b = "àèéìòù";
String stringa_c = "£='?#^@";
// String stringa_c = "Test numeri e lettere: 1234567890 ABCDEFGHIJKLMNOPQRSTUVWXYZ.";


/**
 * Callback invoked when central connects
 * @param conn_handle connection where this event happens
 */
void connect_callback(uint16_t conn_handle) {
  // Get the reference to current connection
  BLEConnection* connection = Bluefruit.Connection(conn_handle);
  char central_name[32] = { 0 };
  connection->getPeerName(central_name, sizeof(central_name));
  Serial.printf("Connected to central: %s\n", central_name);  
}

/**
 * Callback invoked when a connection is dropped
 * @param conn_handle connection where this event happens
 * @param reason is a BLE_HCI_STATUS_CODE which can be found in ble_hci.h
 */
void disconnect_callback(uint16_t conn_handle, uint8_t reason) {
  (void)conn_handle;
  (void)reason;  
  Serial.printf("\nDisconnected, reason = 0x%02X\n", reason);  
}

uint8_t getBatteryStats() {
  digitalWrite(VBAT_ENABLE, LOW);
  float batVoltage = analogRead(PIN_VBAT) * VBAT_SCALING * VBAT_RATIO;
  uint8_t battery = constrain((batVoltage - MIN_BAT_VOLTAGE) / BATTERY_FULL_SCALE * 100, 0, 100);
  digitalWrite(VBAT_ENABLE, HIGH);
  // bool charging = (digitalRead(BAT_CHARGE_STATE) == LOW);
  return battery;
}


/**
 * Callback invoked when received Set LED from central.
 * Must be set previously with setKeyboardLedCallback()
 *
 * The LED bit map is as follows: (also defined by KEYBOARD_LED_* )
 *    Kana (4) | Compose (3) | ScrollLock (2) | CapsLock (1) | Numlock (0)
 */
void set_keyboard_led(uint16_t conn_handle, uint8_t led_bitmap) {
  (void) conn_handle;

  // light up Red Led if any bits is set
  led_bitmap ? ledOn( LED_RED ) : ledOff( LED_RED );  
}


void startAdv(void) {  
  // Advertising packet
  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addTxPower();
  Bluefruit.Advertising.addAppearance(BLE_APPEARANCE_HID_KEYBOARD);
  
  // Include BLE HID service
  Bluefruit.Advertising.addService(blehid);
  // Include battery service
  Bluefruit.Advertising.addService(blebas);

  // There is enough room for the dev name in the advertising packet
  Bluefruit.Advertising.addName();
  
  /* Start Advertising
   * - Enable auto advertising if disconnected
   * - Interval:  fast mode = 20 ms, slow mode = 152.5 ms
   * - Timeout for fast mode is 30 seconds
   * - Start(timeout) with timeout = 0 will advertise forever (until connected)
   * 
   * For recommended advertising interval
   * https://developer.apple.com/library/content/qa/qa1931/_index.html   
   */
  Bluefruit.Advertising.restartOnDisconnect(true);
  Bluefruit.Advertising.setInterval(32, 244);    // in unit of 0.625 ms
  Bluefruit.Advertising.setFastTimeout(30);      // number of seconds in fast mode
  Bluefruit.Advertising.start(0);                // 0 = Don't stop advertising after n seconds
}


void setup() {
  // Set GPIO low to enable battery voltage read
  pinMode(VBAT_ENABLE, OUTPUT);
  pinMode(BAT_CHARGE_STATE, INPUT_PULLUP);

  Serial.begin(115200);
  while ( !Serial ) delay(10);   // for nrf52840 with native usb

  Serial.println("Bluefruit52 HID Keyboard Example");
  Serial.println("--------------------------------\n");

  Serial.println();
  Serial.println("Go to your phone's Bluetooth settings to pair your device");
  Serial.println("then open an application that accepts keyboard input");

  Serial.println();
  Serial.println("Enter the character(s) to send:");
  Serial.println();  

  Bluefruit.setName("XIAO-nRF52");
  // Bluefruit.configPrphBandwidth(BANDWIDTH_MAX);
  // Bluefruit.configUuid128Count(15);
  Bluefruit.Periph.setConnectCallback(connect_callback);
  Bluefruit.Periph.setDisconnectCallback(disconnect_callback);

  Bluefruit.begin();
  Bluefruit.setTxPower(4);    // Check bluefruit.h for supported values

  // Configure and Start Device Information Service
  bledis.setManufacturer("Seeed Studio");
  bledis.setModel("XIAO nRF52840");  
  bledis.begin();

  /* Start BLE HID
   * Note: Apple requires BLE device must have min connection interval >= 20m
   * ( The smaller the connection interval the faster we could send data).
   * However for HID and MIDI device, Apple could accept min connection interval 
   * up to 11.25 ms. Therefore BLEHidAdafruit::begin() will try to set the min and max
   * connection interval to 11.25  ms and 15 ms respectively for best performance.
   */
  blehid.begin();

  // Set callback for set LED from central
  blehid.setKeyboardLedCallback(set_keyboard_led);

  uint8_t battery = getBatteryStats();
  blebas.begin();
  blebas.write(battery);

  /* Set connection interval (min, max) to your perferred value.
   * Note: It is already set by BLEHidAdafruit::begin() to 11.25ms - 15ms
   * min = 9*1.25=11.25 ms, max = 12*1.25= 15 ms 
   */
  /* Bluefruit.Periph.setConnInterval(9, 12); */

  // Set up and start advertising
  startAdv();
}


void loop() {
  // Check BLE connection status and restart advertising if needed  
  if (!Bluefruit.connected() && !Bluefruit.Advertising.isRunning()) {
    Serial.println("Restart advertising");    
    Bluefruit.Advertising.start(0);
    delay(100);
  }

  handleSerial();
  

  // Check battery level each 30 seconds
  static uint32_t batteryUpdateTime;
  if (millis() - batteryUpdateTime >= 30000) {
    batteryUpdateTime = millis();
    uint8_t battery = getBatteryStats();    
    Serial.printf("Battery level: %d%%\n", battery);

    // Notify BLE if connected
    if (Bluefruit.connected()) {
      blebas.notify(battery);
    }
  }
}


void handleSerial() {
  if (Serial.available()) {
    char ch = (char)Serial.read();

    switch(ch) {
      case '1':
        sendString(stringa_a.c_str());
        break;
        
      case '2':
        sendString(stringa_b.c_str());
        break;
        
      case '3':
        sendString(stringa_c.c_str());
        break;      
        
      case 's':
        Serial.print("Stato connessione: ");
        if (Bluefruit.connected()) {
          Serial.println("CONNESSO");
        } else {
          Serial.println("DISCONNESSO");
        }
        break;
        
      default:        
        String ss = String(ch);
        sendString(ss);        
        break;
    }
  }
  
  // Piccola pausa per evitare sovraccarico del loop
  delay(2);
}



