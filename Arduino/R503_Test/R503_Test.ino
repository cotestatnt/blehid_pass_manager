
#include "src/R503Lib/R503Lib.h"

#define PIN_INTERRUPT 4  // D5 sul XIAO nRF52840

// Set this to the serial port you are using
#define fpsSerial Serial1
R503Lib fps(&fpsSerial, 8, 9, 0xFFFFFFFF);

// Template buffer
uint8_t templateData[1792] = { 0 };
uint16_t sizeTemplateData;

void enrollFinger();
void searchFinger();
void matchFinger();
void deleteFinger();
void clearLibrary();
void printIndexTable();
void saveTemplateToBuffer();
void restoreTemplateFromBuffer();

bool searching = false;


void printMenu() {
  // ask for the action
  Serial.printf("\n== MENU =================================\n\n");

  const char* menuContent =
    "[e] Enroll a New Finger\n"
    "[s] Search a Finger (30s)\n"
    "[m] Match a Finger\n"
    "[d] Delete a Finger\n"
    "[c] Clear Library\n"
    "[p] Print Index Table\n"
    "[t] Transfer (download) Template to MCU\n"
    "[r] Restore Template (upload) to Sensor\n\n";

  Serial.printf(menuContent);
  Serial.printf("== ENTER ACTION =========================\n");
}

void setup() {
  pinMode(PIN_INTERRUPT, INPUT);

  Serial.begin(115200);
  while (!Serial) ;
  Serial.printf("\n\n=========================================\n");
  Serial.printf("== Fingerprint Sensor R503 Example ======\n");
  Serial.printf("=========================================\n\n");

  // set the data rate for the sensor serial port
  if (fps.begin(57600, 0x0) != R503_OK) {
    Serial.println("[X] Sensor not found!");
    while (1) {
      delay(1);
    }
  } else {
    fps.setAuraLED(aLEDBreathing, aLEDBlue, 255, 1);
    // fps.setPacketSize(256); // default is 128, 32 <> 256
    // fps.setBaudrate(57600); // default is 57600, 9600 <> 115200
    Serial.println(" >> Sensor 1 found!");
  }

  
  
  fps.printDeviceInfo();
  fps.printParameters();

  printMenu();
}

void loop()  // run over and over again
{

  if (!digitalRead(PIN_INTERRUPT) && !searching) {  
    Serial.println("D5 triggered!");
    searching = true;
    searchFinger();
  }

  while (Serial.available()) {
    String str = Serial.readStringUntil('\n');
    Serial.printf(" > %s\n", str.c_str());
    Serial.printf("----------------------------------------\n");

    char action = str[0];

    switch (action) {
      case 'e':
        searching = true;
        enrollFinger();        
        break;
      case 's':
        searchFinger();
        break;
      case 'm':
        matchFinger();
        break;
      case 'd':
        deleteFinger();
        break;
      case 'c':
        clearLibrary();
        break;
      case 'p':
        printIndexTable();
        break;
      case 't':
        saveTemplateToBuffer();
        break;
      case 'r':
        restoreTemplateFromBuffer();
        break;
      default:
        Serial.printf(" [X] '%c' is not a valid action!\n", action);
    }
  }

}

void enrollFinger() {
  R503Lib* fp = &fps;
  int ret;
  String str;

  Serial.flush();  // flush the buffer
  Serial.println("How many features do you want to extract? (1-6)");

  do {
    str = Serial.readStringUntil('\n');
  } while (str.length() < 1);

  uint8_t featureCount = str.toInt();
  Serial.printf(" << %d\n\n", featureCount);
  Serial.flush();  // flush the buffer
  Serial.println("Where do you want to store the fingerprint (ID)?");

  do {
    str = Serial.readStringUntil('\n');
  } while (str.length() < 1);

  uint16_t location = str.toInt();
  Serial.printf(" << %d\n\n", location);
  Serial.print("We are all set, follow the steps below to enroll a new finger");

  for (int i = 1; i <= featureCount; i++) {
    fp->setAuraLED(aLEDBreathing, aLEDBlue, 50, 255);
    Serial.println("\n\n >> Place finger on sensor...");
    while (true) {
      ret = fp->takeImage();
      if (ret == R503_NO_FINGER) {
        delay(100);
        continue;  // try again
      } else if (ret == R503_OK) {
        Serial.printf(" >> Image %d of %d taken \n", i, featureCount);
        fp->setAuraLED(aLEDBreathing, aLEDYellow, 255, 255);
        // Go for feature extraction
      } else {
        Serial.printf("[X] Could not take image (code: 0x%02X)\n", ret);
        fp->setAuraLED(aLEDFlash, aLEDRed, 50, 3);
        delay(1000);
        continue;  // try again
      }

      ret = fp->extractFeatures(i);
      if (ret != R503_OK) {
        Serial.printf("[X] Failed to extract features, trying again (code: 0x%02X)\n", ret);
        fp->setAuraLED(aLEDFlash, aLEDRed, 50, 3);
        delay(1000);
        continue;
      }

      fp->setAuraLED(aLEDBreathing, aLEDGreen, 255, 255);
      Serial.printf(" >> Features %d of %d extracted\n", i, featureCount);
      delay(250);

      break;
    }

    Serial.println("\n\n >> Lift your finger from the sensor!");

    while (fp->takeImage() != R503_NO_FINGER) {
      delay(100);
    }
  }

  Serial.println(" >> Creating template...");
  fp->setAuraLED(aLEDBreathing, aLEDPurple, 100, 255);
  delay(100);
  ret = fp->createTemplate();

  if (ret != R503_OK) {
    Serial.printf("[X] Failed to create a template (code: 0x%02X)\n", ret);
    fp->setAuraLED(aLEDFlash, aLEDRed, 50, 3);
    return;
  }

  Serial.println(" >> Template created");
  ret = fp->storeTemplate(1, location);
  if (ret != R503_OK) {
    Serial.printf("[X] Failed to store the template (code: 0x%02X)\n", ret);
    fp->setAuraLED(aLEDFlash, aLEDRed, 50, 3);
    return;
  }

  delay(250);

  fp->setAuraLED(aLEDBreathing, aLEDGreen, 255, 1);
  Serial.printf(" >> Template stored at location: %d\n", location);
}

void searchFinger() {
  unsigned long start = millis();
  R503Lib* fp = &fps;
  int ret;
  String str;

  Serial.flush();  // flush the buffer
  Serial.printf(" >> Place your finger on the sensor...\n\n");
  fp->setAuraLED(aLEDBreathing, aLEDBlue, 50, 255);
  while (millis() - start < 30000) {
    ret = fp->takeImage();

    if (ret == R503_NO_FINGER) {
      delay(250);
      continue;
    } else if (ret == R503_OK) {
      Serial.printf(" >> Image taken \n");
      fp->setAuraLED(aLEDFlash, aLEDWhite, 150, 255);
      break;
    } else {
      Serial.printf("[X] Could not take image (code: 0x%02X)\n", ret);
      fp->setAuraLED(aLEDFlash, aLEDRed, 50, 3);
      delay(1000);
      continue;
    }
  }

  if (millis() - start >= 10000) {
    Serial.printf("[X] Could not take image (timeout)\n");
    fp->setAuraLED(aLEDFlash, aLEDRed, 100, 2);
    searching = false;
    return;
  }

  ret = fp->extractFeatures(1);

  if (ret != R503_OK) {
    Serial.printf("[X] Could not extract features (code: 0x%02X)\n", ret);
    fp->setAuraLED(aLEDFlash, aLEDRed, 50, 3);
    searching = false;
    return;
  }

  uint16_t location, confidence;
  ret = fp->searchFinger(1, location, confidence);

  if (ret == R503_NO_MATCH_IN_LIBRARY) {
    Serial.printf(" >> No matching finger found\n");
    fp->setAuraLED(aLEDBreathing, aLEDRed, 255, 1);
  } else if (ret == R503_OK) {
    Serial.println(" >> Found finger");
    Serial.printf("    Finger ID: %d\n", location);
    Serial.printf("    Confidence: %d\n", confidence);
    fp->setAuraLED(aLEDBreathing, aLEDGreen, 255, 1);
  } else {
    Serial.printf("[X] Could not search library (code: 0x%02X)\n", ret);
    fp->setAuraLED(aLEDFlash, aLEDRed, 50, 3);
  }
  searching = false;
}

void matchFinger() {
  unsigned long start = millis();
  R503Lib* fp = &fps;
  int ret;
  String str;

  Serial.flush();  // flush the buffer
  Serial.println("Enter the fingerprint location (ID) to match with:");

  do {
    str = Serial.readStringUntil('\n');
  } while (str.length() < 1);

  uint16_t location = str.toInt();
  Serial.printf(" << %d\n\n", location);
  Serial.printf(" >> Getting template from finger ID: %d\n", location);
  ret = fp->getTemplate(1, location);

  if (ret != R503_OK) {
    Serial.printf("[X] Could not get template (code: 0x%02X)\n", ret);
    fp->setAuraLED(aLEDFlash, aLEDRed, 50, 3);
    return;
  }

  Serial.printf(" >> Place your finger on the sensor...\n");
  fp->setAuraLED(aLEDBreathing, aLEDBlue, 50, 255);

  while (millis() - start < 10000) {
    ret = fp->takeImage();

    if (ret == R503_NO_FINGER) {
      delay(250);
      continue;
    } else if (ret == R503_OK) {
      Serial.printf(" >> Image taken \n");
      fp->setAuraLED(aLEDBreathing, aLEDYellow, 150, 255);
      break;
    } else {
      Serial.printf("[X] Could not take image (code: 0x%02X)\n", ret);
      fp->setAuraLED(aLEDFlash, aLEDRed, 50, 3);
      delay(1000);
      continue;
    }
  }

  ret = fp->extractFeatures(3);  // extract features from buffer 3

  if (ret != R503_OK) {
    Serial.printf("[X] Could not extract features (code: 0x%02X)\n", ret);
    fp->setAuraLED(aLEDFlash, aLEDRed, 50, 3);
    return;
  }

  uint16_t confidence;
  ret = fp->matchFinger(confidence);

  if (ret == R503_NO_MATCH) {
    Serial.printf(" >> No matching finger found\n");
    fp->setAuraLED(aLEDBreathing, aLEDRed, 255, 1);
  } else if (ret == R503_OK) {
    Serial.println(" >> Found finger");
    fp->setAuraLED(aLEDBreathing, aLEDGreen, 255, 1);
  } else {
    Serial.printf("[X] Could not search library (code: 0x%02X)\n", ret);
    fp->setAuraLED(aLEDFlash, aLEDRed, 50, 3);
  }
}

void deleteFinger() {
  R503Lib* fp = &fps;
  int ret;
  String str;

  Serial.flush();  // flush the buffer
  Serial.println("Enter Finger ID to be deleted:");

  do {
    str = Serial.readStringUntil('\n');
  } while (str.length() < 1);

  uint16_t location = str.toInt();
  Serial.printf(" << %d\n\n", location);

  ret = fp->deleteTemplate(location);

  if (ret == R503_OK) {
    fp->setAuraLED(aLEDBreathing, aLEDGreen, 50, 2);
    Serial.printf("Finger with ID %d deleted\n", location);
  } else {
    fp->setAuraLED(aLEDFlash, aLEDRed, 50, 3);
    Serial.printf("\n[X] Failed to delete finger (code: 0x%02X)\n", ret);
  }
}

void clearLibrary() {
  R503Lib* fp = &fps;
  int ret;
  String str;

  Serial.flush();  // flush the buffer
  Serial.println("Do you really want to clear library Yes [y] / No [n]");
  fp->setAuraLED(aLEDON, aLEDRed, 255, 1);

  do {
    str = Serial.readStringUntil('\n');
  } while (str.length() < 1);

  Serial.printf(" << %c\n\n", str[0]);

  if (str[0] == 'y') {
    int ret = fp->emptyLibrary();
    if (ret == R503_OK) {
      Serial.printf("The fingerprint library has been cleared!\n");
      fp->setAuraLED(aLEDBreathing, aLEDGreen, 50, 2);
    } else {
      Serial.printf("[X] Failed to clear library (code: 0x%02X)\n", ret);
      fp->setAuraLED(aLEDFlash, aLEDRed, 50, 3);
    }
  } else {
    Serial.printf("The operation has been cancelled.\n");

    fp->setAuraLED(aLEDFadeOut, aLEDRed, 100, 0);
  }
}

void printIndexTable() {
  R503Lib* fp = &fps;
  int ret;
  String str;

  Serial.flush();  // flush the buffer
  Serial.printf("\nReading index table...\n");

  uint16_t count;
  ret = fp->getTemplateCount(count);
  if (ret == R503_OK) {
    Serial.printf("- Amount of templates: %d\n", count);
  } else {
    Serial.printf(" - Amount of templates: could not read (code: 0x%02X)\n", ret);
  }

  R503Parameters params;
  ret = fp->readParameters(params);
  if (ret == R503_OK) {
    Serial.printf("- Fingerprint library capacity: %d\n", params.fingerLibrarySize);
  } else {
    Serial.printf("- Fingerprint library capacity: could not read (code: 0x%02X)\n", ret);
  }

  // only print first page, since we know library size is less than 256
  uint8_t table[32];
  ret = fp->readIndexTable(table);

  if (ret == R503_OK) {
    Serial.printf("- Fingerprints stored at locations (ID):\n  ");

    for (int i = 0; i < 32; i++) {
      for (int b = 0; b < 8; b++) {
        if (table[i] >> b & 0x01) {
          Serial.printf("%d  ", i * 8 + b);
        }
      }
    }

    Serial.println();
  } else {
    Serial.printf("- Fingerprints stored at locations (ID): could not read (code: 0x%02X)\n", ret);
  }
}

void saveTemplateToBuffer() {
  R503Lib* fp = &fps;
  int ret;
  String str;

  Serial.flush();  // flush the buffer
  Serial.println("Which finger ID do you want to download the template from?");

  do {
    str = Serial.readStringUntil('\n');
  } while (str.length() < 1);

  uint8_t fingerID = str.toInt();
  Serial.printf(" << %d\n\n", fingerID);

  Serial.println();

  fp->setAuraLED(aLEDBreathing, aLEDYellow, 50, 255);
  Serial.printf(" >> Getting template from finger ID: %d\n", fingerID);

  ret = fp->getTemplate(1, fingerID);

  if (ret != R503_OK) {
    Serial.printf("Err: getting template failed: %d \n", ret);
    fp->setAuraLED(aLEDFlash, aLEDRed, 50, 3);
    return;
  }

  Serial.printf(" >> Template placed in buffer\n");
  Serial.printf("    Downloading template to MCU\n");

  ret = fp->downloadTemplate(2, templateData, sizeTemplateData);

  if (ret != R503_OK) {
    Serial.printf("Err: downloading template failed (code: 0x%02X)\n", ret);
    fp->setAuraLED(aLEDFlash, aLEDRed, 50, 3);
    return;
  }

  delay(250);

  Serial.println(" >> The template has been downloaded");
  fp->setAuraLED(aLEDBreathing, aLEDGreen, 255, 1);
  Serial.printf("    Template data:\n\n");
  for (int i = 0; i < sizeTemplateData; i++) {
    Serial.printf("%02X ", templateData[i]);
  }
  Serial.println();
}

void restoreTemplateFromBuffer() {
  R503Lib* fp = &fps;
  int ret;
  String str;

  Serial.flush();  // flush the buffer
  Serial.println("Which location (Finger ID) do you want to store the template to?");

  do {
    str = Serial.readStringUntil('\n');
  } while (str.length() < 1);

  uint8_t fingerID = str.toInt();
  Serial.printf(" << %d\n\n", fingerID);
  Serial.println();

  fp->setAuraLED(aLEDBreathing, aLEDYellow, 50, 255);
  Serial.println(" >> Uploading template to sensor buffer");

  ret = fp->uploadTemplate(1, templateData, sizeTemplateData);

  if (ret != R503_OK) {
    Serial.printf("[X] Failed to upload template (code: 0x%02X)\n", ret);
    fp->setAuraLED(aLEDFlash, aLEDRed, 50, 3);
    return;
  }

  Serial.printf(" >> Template uploaded to sensor buffer\n");

  ret = fp->storeTemplate(1, fingerID);

  if (ret != R503_OK) {
    Serial.printf("[X] Failed to store template at location %d (code: 0x%02X)\n", fingerID, ret);
    fp->setAuraLED(aLEDFlash, aLEDRed, 50, 3);
    return;
  }

  fp->setAuraLED(aLEDBreathing, aLEDGreen, 50, 2);
  Serial.printf(" >> Template stored at location %d\n", fingerID);
}