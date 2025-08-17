/*!
 * @file Fingerprint.cpp
 *
 * @mainpage Fingerprint Sensor Library
 *
 * @section intro_sec Introduction
 *
 * This is a library for our optical Fingerprint sensor
 *
 * Designed specifically to work with the Adafruit Fingerprint sensor
 * ----> http://www.adafruit.com/products/751
 *
 * These displays use TTL Serial to communicate, 2 pins are required to
 * interface
 * Adafruit invests time and resources providing this open source code,
 * please support Adafruit and open-source hardware by purchasing
 * products from Adafruit!
 *
 * @section author Author
 *
 * Written by Limor Fried/Ladyada for Adafruit Industries.
 *
 * @section license License
 *
 * BSD license, all text above must be included in any redistribution
 *
 */

#include "kw111lib.h"
#include <cstdarg>

#define TAG "FingerprintLib"

#define FINGERPRINT_DEBUG 1

/**
 * @brief Helper function to get command packet with variable arguments
 * @param packet Reference to packet that will be filled
 * @param dataLen Number of data bytes to read
 * @param ... Variable number of uint8_t arguments for packet data
 * @return FINGERPRINT_OK on success, error code otherwise
 */
uint8_t FingerprintSensor::getCmdPacket(Fingerprint_Packet &packet, uint8_t dataLen, ...) {
    va_list args;
    va_start(args, dataLen);
    
    uint8_t data[64];
    
    // Extract the specified number of arguments
    for (int i = 0; i < dataLen && i < 64; i++) {
        data[i] = va_arg(args, int); // va_arg promotes uint8_t to int
    }
    va_end(args);
    
    packet = Fingerprint_Packet(FINGERPRINT_COMMANDPACKET, dataLen, data);
    writeStructuredPacket(packet);
    
    if (getStructuredPacket(&packet) != FINGERPRINT_OK)
        return FINGERPRINT_PACKETRECIEVEERR;
    if (packet.type != FINGERPRINT_ACKPACKET)
        return FINGERPRINT_PACKETRECIEVEERR;
        
    return FINGERPRINT_OK;
}

/**
 * @brief Helper function to send command packet with variable arguments
 * @param dataLen Number of data bytes to read
 * @param ... Variable number of uint8_t arguments for packet data
 * @return Response code from packet.data[0]
 */
uint8_t FingerprintSensor::sendCmdPacket(uint8_t dataLen, ...) {
    va_list args;
    va_start(args, dataLen);
    
    uint8_t data[64];
    
    // Extract the specified number of arguments
    for (int i = 0; i < dataLen && i < 64; i++) {
        data[i] = va_arg(args, int); // va_arg promotes uint8_t to int
    }
    va_end(args);
    
    Fingerprint_Packet packet(FINGERPRINT_COMMANDPACKET, dataLen, data);
    writeStructuredPacket(packet);
    
    if (getStructuredPacket(&packet) != FINGERPRINT_OK)
        return FINGERPRINT_PACKETRECIEVEERR;
    if (packet.type != FINGERPRINT_ACKPACKET)
        return FINGERPRINT_PACKETRECIEVEERR;
        
    return packet.data[0];
}

/***************************************************************************
 PUBLIC FUNCTIONS
 ***************************************************************************/

/**
 * @brief Constructor for R503Lib class.
 *
 * @param serial Pointer to HardwareSerial object.
 * @param rxPin RX pin number.
 * @param txPin TX pin number.
 * @param address Device address.
 */
FingerprintSensor::FingerprintSensor(uart_port_t uartNum, int rxPin, int txPin, uint32_t address, uint32_t password)
    : uart_num(uartNum), fpsRxPin(rxPin), fpsTxPin(txPin), theAddress(address), thePassword(password) {    
}


/**
 * @brief Destructor for the FingerprintSensor class.
 *
 * This function deletes the fpsSerial object created in begin(...)
 */
FingerprintSensor::~FingerprintSensor() {
    uart_driver_delete(uart_num);
}


/**
 * @brief Reads device information from the R503 fingerprint module and stores it in the provided R503DeviceInfo struct.
 * 
 * @param info The FingerprintDeviceInfo struct to store the device information in.
 *
 * @return uint8_t Returns FingerprintSensor if successful, otherwise returns an error code.
 */
uint8_t FingerprintSensor::readDeviceInfo(FingerprintDeviceInfo &info) {
    Fingerprint_Packet packet;
    if (getCmdPacket(packet, 2, 47, 0x3C) != FINGERPRINT_OK) {
        return FINGERPRINT_PACKETRECIEVEERR;
    }
    memcpy(info.moduleType, &packet.data[1], 16);
    memcpy(info.batchNumber, &packet.data[17], 4);
    memcpy(info.serialNumber, &packet.data[21], 8);
    info.hardwareVersion[0] = packet.data[29];
    info.hardwareVersion[1] = packet.data[30];
    memcpy(info.sensorType, &packet.data[31], 8);
    info.sensorWidth = packet.data[39] << 8 | packet.data[40];
    info.sensorHeight = packet.data[41] << 8 | packet.data[42];
    info.templateSize = packet.data[43] << 8 | packet.data[44];
    info.databaseSize = packet.data[45] << 8 | packet.data[46];
    return FINGERPRINT_ACKPACKET;
}


/**
 * @brief Initializes the R503 fingerprint sensor library with the specified baudrate and password.
 *
 * @param baudrate The baudrate to use for serial communication with the sensor.
 * @param passwd The password to use for accessing the sensor.
 *
 * @return uint8_t Returns R503_OK if initialization is successful, otherwise returns an error code.
 */
uint8_t FingerprintSensor::begin(uint16_t baudrate) {

    uart_config_t uart_config = {
        .baud_rate = baudrate,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };

    uart_param_config(uart_num, &uart_config);
    uart_set_pin(uart_num, fpsTxPin, fpsRxPin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(uart_num, 2048, 0, 0, NULL, 0);

    int retVal = verifyPassword();
    if (retVal != FINGERPRINT_OK) {
        ESP_LOGE(TAG, "verifyPassword failed: 0x%02X", retVal);
        return retVal;
    }

    retVal = getParameters() ;
    ESP_LOGD(TAG, "readParameters failed: 0x%02X", retVal);

    retVal = getTemplateCount();
    if (retVal != FINGERPRINT_OK) {
        ESP_LOGE(TAG, "getTemplateCount failed: 0x%02X", retVal);
    }

    return FINGERPRINT_OK;
}


/**************************************************************************/
/*!
    @brief  Verifies the sensors' access password (default password is
   0x0000000). A good way to also check if the sensors is active and responding
    @returns True if password is correct
*/
/**************************************************************************/
bool FingerprintSensor::verifyPassword(void) {
  return checkPassword() == FINGERPRINT_OK;
}

uint8_t FingerprintSensor::checkPassword(void) {
  Fingerprint_Packet packet;
  if (getCmdPacket(packet, 5, FINGERPRINT_VERIFYPASSWORD, (uint8_t)(thePassword >> 24),
                 (uint8_t)(thePassword >> 16), (uint8_t)(thePassword >> 8),
                 (uint8_t)(thePassword & 0xFF)) != FINGERPRINT_OK) {
    return FINGERPRINT_PACKETRECIEVEERR;
  }
  if (packet.data[0] == FINGERPRINT_OK)
    return FINGERPRINT_OK;
  else
    return FINGERPRINT_PACKETRECIEVEERR;
}

/**************************************************************************/
/*!
    @brief  Get the sensors parameters, fills in the member variables
    status_reg, system_id, capacity, security_level, device_addr, packet_len
    and baud_rate
    @returns True if password is correct
*/
/**************************************************************************/
uint8_t FingerprintSensor::getParameters(void) {
  Fingerprint_Packet packet;
  if (getCmdPacket(packet, 1, FINGERPRINT_READSYSPARAM) != FINGERPRINT_OK) {
    return FINGERPRINT_PACKETRECIEVEERR;
  }

  status_reg = ((uint16_t)packet.data[1] << 8) | packet.data[2];
  system_id = ((uint16_t)packet.data[3] << 8) | packet.data[4];
  capacity = ((uint16_t)packet.data[5] << 8) | packet.data[6];
  security_level = ((uint16_t)packet.data[7] << 8) | packet.data[8];
  device_addr = ((uint32_t)packet.data[9] << 24) |
                ((uint32_t)packet.data[10] << 16) |
                ((uint32_t)packet.data[11] << 8) | (uint32_t)packet.data[12];
  packet_len = ((uint16_t)packet.data[13] << 8) | packet.data[14];
  if (packet_len == 0) {
    packet_len = 32;
  } else if (packet_len == 1) {
    packet_len = 64;
  } else if (packet_len == 2) {
    packet_len = 128;
  } else if (packet_len == 3) {
    packet_len = 256;
  }
  baud_rate = (((uint16_t)packet.data[15] << 8) | packet.data[16]) * 9600;

  return packet.data[0];
}

/**************************************************************************/
/*!
    @brief   Ask the sensor to take an image of the finger pressed on surface
    @returns <code>FINGERPRINT_OK</code> on success
    @returns <code>FINGERPRINT_NOFINGER</code> if no finger detected
    @returns <code>FINGERPRINT_PACKETRECIEVEERR</code> on communication error
    @returns <code>FINGERPRINT_IMAGEFAIL</code> on imaging error
*/
/**************************************************************************/
uint8_t FingerprintSensor::getImage(void) {
  return sendCmdPacket(1, FINGERPRINT_GETIMAGE);
}

/**************************************************************************/
/*!
    @brief   Ask the sensor to convert image to feature template
    @param slot Location to place feature template (put one in 1 and another in
   2 for verification to create model)
    @returns <code>FINGERPRINT_OK</code> on success
    @returns <code>FINGERPRINT_IMAGEMESS</code> if image is too messy
    @returns <code>FINGERPRINT_PACKETRECIEVEERR</code> on communication error
    @returns <code>FINGERPRINT_FEATUREFAIL</code> on failure to identify
   fingerprint features
    @returns <code>FINGERPRINT_INVALIDIMAGE</code> on failure to identify
   fingerprint features
*/
uint8_t FingerprintSensor::image2Tz(uint8_t slot) {
  return sendCmdPacket(2, FINGERPRINT_IMAGE2TZ, slot);
}

/**************************************************************************/
/*!
    @brief   Ask the sensor to take two print feature template and create a
   model
    @returns <code>FINGERPRINT_OK</code> on success
    @returns <code>FINGERPRINT_PACKETRECIEVEERR</code> on communication error
    @returns <code>FINGERPRINT_ENROLLMISMATCH</code> on mismatch of fingerprints
*/
uint8_t FingerprintSensor::createModel(void) {
  return sendCmdPacket(1, FINGERPRINT_REGMODEL);
}

/**************************************************************************/
/*!
    @brief   Ask the sensor to store the calculated model for later matching
    @param   location The model location #
    @returns <code>FINGERPRINT_OK</code> on success
    @returns <code>FINGERPRINT_BADLOCATION</code> if the location is invalid
    @returns <code>FINGERPRINT_FLASHERR</code> if the model couldn't be written
   to flash memory
    @returns <code>FINGERPRINT_PACKETRECIEVEERR</code> on communication error
*/
uint8_t FingerprintSensor::storeModel(uint16_t location) {
  return sendCmdPacket(4, FINGERPRINT_STORE, 0x01, (uint8_t)(location >> 8),
                  (uint8_t)(location & 0xFF));
}

/**************************************************************************/
/*!
    @brief   Ask the sensor to load a fingerprint model from flash into buffer 1
    @param   location The model location #
    @returns <code>FINGERPRINT_OK</code> on success
    @returns <code>FINGERPRINT_BADLOCATION</code> if the location is invalid
    @returns <code>FINGERPRINT_PACKETRECIEVEERR</code> on communication error
*/
uint8_t FingerprintSensor::loadModel(uint16_t location) {
  return sendCmdPacket(4, FINGERPRINT_LOAD, 0x01, (uint8_t)(location >> 8),
                  (uint8_t)(location & 0xFF));
}

/**************************************************************************/
/*!
    @brief   Ask the sensor to transfer 256-byte fingerprint template from the
   buffer to the UART
    @returns <code>FINGERPRINT_OK</code> on success
    @returns <code>FINGERPRINT_PACKETRECIEVEERR</code> on communication error
*/
uint8_t FingerprintSensor::getModel(void) {
  return sendCmdPacket(2, FINGERPRINT_UPLOAD, 0x01);
}

/**************************************************************************/
/*!
    @brief   Ask the sensor to delete a model in memory
    @param   location The model location #
    @returns <code>FINGERPRINT_OK</code> on success
    @returns <code>FINGERPRINT_BADLOCATION</code> if the location is invalid
    @returns <code>FINGERPRINT_FLASHERR</code> if the model couldn't be written
   to flash memory
    @returns <code>FINGERPRINT_PACKETRECIEVEERR</code> on communication error
*/
uint8_t FingerprintSensor::deleteModel(uint16_t location) {
  return sendCmdPacket(5, FINGERPRINT_DELETE, (uint8_t)(location >> 8),
                  (uint8_t)(location & 0xFF), 0x00, 0x01);
}

/**************************************************************************/
/*!
    @brief   Ask the sensor to delete ALL models in memory
    @returns <code>FINGERPRINT_OK</code> on success
    @returns <code>FINGERPRINT_BADLOCATION</code> if the location is invalid
    @returns <code>FINGERPRINT_FLASHERR</code> if the model couldn't be written
   to flash memory
    @returns <code>FINGERPRINT_PACKETRECIEVEERR</code> on communication error
*/
uint8_t FingerprintSensor::emptyDatabase(void) {
  return sendCmdPacket(1, FINGERPRINT_EMPTY);
}

/**************************************************************************/
/*!
    @brief   Ask the sensor to search the current slot 1 fingerprint features to
   match saved templates. The matching location is stored in <b>fingerID</b> and
   the matching confidence in <b>confidence</b>
    @returns <code>FINGERPRINT_OK</code> on fingerprint match success
    @returns <code>FINGERPRINT_NOTFOUND</code> no match made
    @returns <code>FINGERPRINT_PACKETRECIEVEERR</code> on communication error
*/
/**************************************************************************/
uint8_t FingerprintSensor::fingerFastSearch(void) {
  // high speed search of slot #1 starting at page 0x0000 and page #0x00A3
  Fingerprint_Packet packet;
  if (getCmdPacket(packet, 6, FINGERPRINT_HISPEEDSEARCH, 0x01, 0x00, 0x00, 0x00, 0xA3) != FINGERPRINT_OK) {
    return FINGERPRINT_PACKETRECIEVEERR;
  }
  fingerID = 0xFFFF;
  confidence = 0xFFFF;

  fingerID = packet.data[1];
  fingerID <<= 8;
  fingerID |= packet.data[2];

  confidence = packet.data[3];
  confidence <<= 8;
  confidence |= packet.data[4];

  return packet.data[0];
}

/**************************************************************************/
/*!
    @brief   Control the built in LED
    @param on True if you want LED on, False to turn LED off
    @returns <code>FINGERPRINT_OK</code> on success
*/
/**************************************************************************/
uint8_t FingerprintSensor::LEDcontrol(bool on) {
  if (on) {
    return sendCmdPacket(1, FINGERPRINT_LEDON);
  } else {
    return sendCmdPacket(1, FINGERPRINT_LEDOFF);
  }
}

/**************************************************************************/
/*!
    @brief   Control the built in Aura LED (if exists). Check datasheet/manual
    for different colors and control codes available
    @param control The control code (e.g. breathing, full on)
    @param speed How fast to go through the breathing/blinking cycles
    @param coloridx What color to light the indicator
    @param count How many repeats of blinks/breathing cycles
    @returns <code>FINGERPRINT_OK</code> on fingerprint match success
    @returns <code>FINGERPRINT_NOTFOUND</code> no match made
    @returns <code>FINGERPRINT_PACKETRECIEVEERR</code> on communication error
*/
/**************************************************************************/
uint8_t FingerprintSensor::LEDcontrol(uint8_t control, uint8_t speed,
                                         uint8_t coloridx, uint8_t count) {
  return sendCmdPacket(5, FINGERPRINT_AURALEDCONFIG, control, speed, coloridx, count);
}

/**************************************************************************/
/*!
    @brief   Ask the sensor to search the current slot fingerprint features to
   match saved templates. The matching location is stored in <b>fingerID</b> and
   the matching confidence in <b>confidence</b>
   @param slot The slot to use for the print search, defaults to 1
    @returns <code>FINGERPRINT_OK</code> on fingerprint match success
    @returns <code>FINGERPRINT_NOTFOUND</code> no match made
    @returns <code>FINGERPRINT_PACKETRECIEVEERR</code> on communication error
*/
/**************************************************************************/
uint8_t FingerprintSensor::fingerSearch(uint8_t slot) {
  // search of slot starting thru the capacity
  Fingerprint_Packet packet;
  if (getCmdPacket(packet, 6, FINGERPRINT_SEARCH, slot, 0x00, 0x00, (uint8_t)(capacity >> 8),
                 (uint8_t)(capacity & 0xFF)) != FINGERPRINT_OK) {
    return FINGERPRINT_PACKETRECIEVEERR;
  }

  fingerID = 0xFFFF;
  confidence = 0xFFFF;

  fingerID = packet.data[1];
  fingerID <<= 8;
  fingerID |= packet.data[2];

  confidence = packet.data[3];
  confidence <<= 8;
  confidence |= packet.data[4];

  return packet.data[0];
}

/**************************************************************************/
/*!
    @brief   Ask the sensor for the number of templates stored in memory. The
   number is stored in <b>templateCount</b> on success.
    @returns <code>FINGERPRINT_OK</code> on success
    @returns <code>FINGERPRINT_PACKETRECIEVEERR</code> on communication error
*/
/**************************************************************************/
uint8_t FingerprintSensor::getTemplateCount(void) {
  Fingerprint_Packet packet;
  if (getCmdPacket(packet, 1, FINGERPRINT_TEMPLATECOUNT) != FINGERPRINT_OK) {
    return FINGERPRINT_PACKETRECIEVEERR;
  }

  templateCount = packet.data[1];
  templateCount <<= 8;
  templateCount |= packet.data[2];

  return packet.data[0];
}

/**************************************************************************/
/*!
    @brief   Set the password on the sensor (future communication will require
   password verification so don't forget it!!!)
    @param   password 32-bit password code
    @returns <code>FINGERPRINT_OK</code> on success
    @returns <code>FINGERPRINT_PACKETRECIEVEERR</code> on communication error
*/
/**************************************************************************/
uint8_t FingerprintSensor::setPassword(uint32_t password) {
  return sendCmdPacket(5, FINGERPRINT_SETPASSWORD, (uint8_t)(password >> 24),
                  (uint8_t)(password >> 16), (uint8_t)(password >> 8),
                  (uint8_t)(password & 0xFF));
}

/**************************************************************************/
/*!
    @brief   Writing module registers
    @param   regAdd 8-bit address of register
    @param   value 8-bit value will write to register
    @returns <code>FINGERPRINT_OK</code> on success
    @returns <code>FINGERPRINT_PACKETRECIEVEERR</code> on communication error
    @returns <code>FINGERPRINT_ADDRESS_ERROR</code> on register address error
*/
/**************************************************************************/
uint8_t FingerprintSensor::writeRegister(uint8_t regAdd, uint8_t value) {

  return sendCmdPacket(3, FINGERPRINT_WRITE_REG, regAdd, value);
}

/**************************************************************************/
/*!
    @brief   Change UART baudrate
    @param   baudrate 8-bit Uart baudrate
    @returns <code>FINGERPRINT_OK</code> on success
    @returns <code>FINGERPRINT_PACKETRECIEVEERR</code> on communication error
*/
/**************************************************************************/
uint8_t FingerprintSensor::setBaudRate(uint8_t baudrate) {

  return (writeRegister(FINGERPRINT_BAUD_REG_ADDR, baudrate));
}

/**************************************************************************/
/*!
    @brief   Change security level
    @param   level 8-bit security level
    @returns <code>FINGERPRINT_OK</code> on success
    @returns <code>FINGERPRINT_PACKETRECIEVEERR</code> on communication error
*/
/**************************************************************************/
uint8_t FingerprintSensor::setSecurityLevel(uint8_t level) {

  return (writeRegister(FINGERPRINT_SECURITY_REG_ADDR, level));
}

/**************************************************************************/
/*!
    @brief   Change packet size
    @param   size 8-bit packet size
    @returns <code>FINGERPRINT_OK</code> on success
    @returns <code>FINGERPRINT_PACKETRECIEVEERR</code> on communication error
*/
/**************************************************************************/
uint8_t FingerprintSensor::setPacketSize(uint8_t size) {

  return (writeRegister(FINGERPRINT_PACKET_REG_ADDR, size));
}
/**************************************************************************/
/*!
    @brief   Helper function to process a packet and send it over UART to the
   sensor
    @param   packet A structure containing the bytes to transmit
*/
/**************************************************************************/

void FingerprintSensor::writeStructuredPacket(const Fingerprint_Packet &packet) {
  uint8_t buf[9 + packet.length + 2]; // header + data + checksum
  int idx = 0;

  // Start code
  buf[idx++] = (uint8_t)(packet.start_code >> 8);
  buf[idx++] = (uint8_t)(packet.start_code & 0xFF);

  // Address
  buf[idx++] = packet.address[0];
  buf[idx++] = packet.address[1];
  buf[idx++] = packet.address[2];
  buf[idx++] = packet.address[3];

  // Type
  buf[idx++] = packet.type;

  // Length (packet.length + 2 for checksum)
  uint16_t wire_length = packet.length + 2;
  buf[idx++] = (uint8_t)(wire_length >> 8);
  buf[idx++] = (uint8_t)(wire_length & 0xFF);

  // Data
  for (uint8_t i = 0; i < packet.length; i++) {
    buf[idx++] = packet.data[i];
  }

  // Checksum calculation
  uint16_t sum = packet.type + (wire_length >> 8) + (wire_length & 0xFF);
  for (uint8_t i = 0; i < packet.length; i++) {
    sum += packet.data[i];
  }
  buf[idx++] = (uint8_t)(sum >> 8);
  buf[idx++] = (uint8_t)(sum & 0xFF);

  #if FINGERPRINT_DEBUG == 1
  ESP_LOGD(TAG, "Sent bytes:");
  for (int i = 0; i < idx; i++) {
    ESP_LOGD(TAG, " 0x%02X", buf[i]);
  }
  #endif

  // Write all bytes to UART using ESP-IDF
  uart_write_bytes(uart_num, (const char*)buf, idx);
}

/**************************************************************************/
/*!
    @brief   Helper function to receive data over UART from the sensor and
   process it into a packet
    @param   packet A structure containing the bytes received
    @param   timeout how many milliseconds we're willing to wait
    @returns <code>FINGERPRINT_OK</code> on success
    @returns <code>FINGERPRINT_TIMEOUT</code> or
   <code>FINGERPRINT_BADPACKET</code> on failure
*/
/**************************************************************************/
uint8_t FingerprintSensor::getStructuredPacket(Fingerprint_Packet *packet, uint32_t timeout) {

  uint8_t byte;
    uint16_t idx = 0;
    uint32_t startTick = xTaskGetTickCount();

    #if FINGERPRINT_DEBUG
    ESP_LOGD(TAG, "\nReceived bytes:");
    #endif

    while (true) {
        int len = uart_read_bytes(uart_num, &byte, 1, 10 / portTICK_PERIOD_MS); 

        #if FINGERPRINT_DEBUG
        ESP_LOGD(TAG, " 0x%02X", byte);
        #endif

        if (len == 1) {
            switch (idx) {
            case 0:
                if (byte != (FINGERPRINT_STARTCODE >> 8))
                    continue;
                packet->start_code = (uint16_t)byte << 8;
                break;
            case 1:
                packet->start_code |= byte;
                if (packet->start_code != FINGERPRINT_STARTCODE)
                    return FINGERPRINT_BADPACKET;
                break;
            case 2: case 3: case 4: case 5:
                packet->address[idx - 2] = byte;
                break;
            case 6:
                packet->type = byte;
                break;
            case 7:
                packet->length = (uint16_t)byte << 8;
                break;
            case 8:
                packet->length |= byte;
                break;
            default:
                packet->data[idx - 9] = byte;
                if ((idx - 8) == packet->length) {
                    return FINGERPRINT_OK;
                }
                break;
            }
            idx++;
            if ((idx + 9) >= sizeof(packet->data)) {
                return FINGERPRINT_BADPACKET;
            }
        }

        // Timeout check
        if ((xTaskGetTickCount() - startTick) * portTICK_PERIOD_MS >= timeout)
            return FINGERPRINT_TIMEOUT;
        vTaskDelay(1); // piccola attesa
    }
    return FINGERPRINT_BADPACKET;
}


uint8_t FingerprintSensor::printDeviceInfo() {
    FingerprintDeviceInfo info;
    int retVal = readDeviceInfo(info);
    if (retVal == FINGERPRINT_OK) {
        ESP_LOGI(TAG, "Module Type: %.*s", 16, info.moduleType);
        ESP_LOGI(TAG, "Module Batch Number: %.*s", 4, info.batchNumber);
        ESP_LOGI(TAG, "Module Serial Number: %.*s", 8, info.serialNumber);
        ESP_LOGI(TAG, "Hardware Version: %d.%d", info.hardwareVersion[0], info.hardwareVersion[1]);
        ESP_LOGI(TAG, "Sensor Type: %.*s", 8, info.sensorType);
        ESP_LOGI(TAG, "Sensor Dimension: %ux%u", info.sensorWidth, info.sensorHeight);
        ESP_LOGI(TAG, "Sensor Template Size: %u", info.templateSize);
        ESP_LOGI(TAG, "Sensor Database Size: %u", info.databaseSize);
    } else {
        ESP_LOGE(TAG, "Error retrieving device info (code: 0x%02X)", retVal);
    }
    return retVal;
}