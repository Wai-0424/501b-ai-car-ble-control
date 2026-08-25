/*************************************************
  Adapted from BM7701-00-1 library example "simpleWriteAndRead"
  for BMduino-UNO, BLE module on SPI1 header's Serial3 (D27/D28).
  Works with any generic BLE debug app (nRF Connect, Serial Bluetooth Terminal, etc).
**************************************************/
#include "BM7701-00-1.h"
BM7701_00_1 BC7701(&Serial3);

#define TX_POWER     0x0F
#define XTAL_CLOAD   0x04
#define ADV_MIN      100
#define ADV_MAX      100
#define CON_MIN      30
#define CON_MAX      30
#define CON_LATENCY  0
#define CON_TIMEOUT  300
uint8_t BDAddress[6] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66};
uint8_t BDName[] = {'B', 'M', 'C', '_', 'C', 'A', 'R'};
uint8_t Adata[] = {0x02, 0x01, 0x06};
uint8_t Sdata[] = {0x03, 0x02, 0x0f, 0x18};

bool board_connect = false;
bool board_receive = false;
bool board_conIntv = false;
bool initOK = false;
uint8_t Status;
uint8_t sel = 1;
uint8_t receiveBuf[256] = {0};
uint8_t receiveLen = 0;
uint8_t sendBuf[256] = {0};
uint8_t sendLen = 0;
uint32_t timeLast = 0;
uint32_t lastHeartbeat = 0;

void setup() {
  delay(60);
  Serial.begin(115200);
  BC7701.begin(BAUD_115200);
  Serial.println("Starting BLE init sequence...");
  while (sel != 10) {
    switch (sel) {
      case 1: if (BC7701.setAddress(BDAddress)) { Serial.println("1 setAddress OK"); sel++; } else { Serial.println("1 setAddress FAIL"); sel = 0xFF; } break;
      case 2: if (BC7701.setName(sizeof(BDName), BDName)) { Serial.println("2 setName OK"); sel++; } else { Serial.println("2 setName FAIL"); sel = 0xFF; } break;
      case 3: if (BC7701.setAdvIntv(ADV_MIN / 0.625, ADV_MAX / 0.625, 7)) { Serial.println("3 setAdvIntv OK"); sel++; } else { Serial.println("3 setAdvIntv FAIL"); sel = 0xFF; } break;
      case 4: if (BC7701.setAdvData(APPEND_NAME, sizeof(Adata), Adata)) { Serial.println("4 setAdvData OK"); sel++; } else { Serial.println("4 setAdvData FAIL"); sel = 0xFF; } break;
      case 5: if (BC7701.setScanData(sizeof(Sdata), Sdata)) { Serial.println("5 setScanData OK"); sel++; } else { Serial.println("5 setScanData FAIL"); sel = 0xFF; } break;
      case 6: if (BC7701.setTXpower(TX_POWER)) { Serial.println("6 setTXpower OK"); sel++; } else { Serial.println("6 setTXpower FAIL"); sel = 0xFF; } break;
      case 7: if (BC7701.setCrystalOffset(XTAL_CLOAD)) { Serial.println("7 setCrystalOffset OK"); sel++; } else { Serial.println("7 setCrystalOffset FAIL"); sel = 0xFF; } break;
      case 8: if (BC7701.setFeature(FEATURE_DIR, AUTO_SEND_SATUS)) { Serial.println("8 setFeature OK"); sel++; } else { Serial.println("8 setFeature FAIL"); sel = 0xFF; } break;
      case 9: if (BC7701.setAdvCtrl(ENABLE)) { Serial.println("9 setAdvCtrl(ENABLE) OK - should be advertising now"); initOK = true; sel++; } else { Serial.println("9 setAdvCtrl FAIL"); sel = 0xFF; } break;
      case 0xFF:
        Serial.println("BLE init FAILED - stopping here. Check wiring/power to BLE module.");
        initOK = false;
        sel = 10;
        break;
    }
  }
  delay(650);
  Serial.print("BLE init sequence done. initOK=");
  Serial.println(initOK);
}

bool readSerialMonitor(uint8_t buff[], uint8_t &len) {
  if (Serial.available() > 0) {
    len = 0;
    while (Serial.available() != 0) {
      timeLast = millis();
      buff[len] = Serial.read();
      len++;
      while (Serial.available() == 0) {
        if (millis() - timeLast >= 50) break;
      }
    }
    return true;
  }
  return false;
}

void loop() {
  if (millis() - lastHeartbeat >= 1000) {
    lastHeartbeat = millis();
    Serial.print("[hb] initOK=");
    Serial.print(initOK);
    Serial.print(" connected=");
    Serial.println(board_connect);
  }

  Status = BC7701.receiveData(receiveBuf, receiveLen);

  if (Status) {
    switch (Status) {
      case API_CONNECTED:
        if (board_connect == false) {
          board_connect = true;
          board_receive = false;
          Serial.println("BLE Connected");
          if (board_conIntv == false) {
            BC7701.wakeUp();
            delay(30);
            if (BC7701.setConnIntv(CON_MIN / 1.25, CON_MAX / 1.25, CON_LATENCY, CON_TIMEOUT)) {
              board_conIntv = true;
            }
          }
        }
        break;
      case API_DISCONNECTED:
        board_connect = false;
        board_receive = false;
        board_conIntv = false;
        Serial.println("BLE Disconnected");
        break;
      case DATA_RECEIVED:
        board_receive = true;
        break;
      case API_ERROR:
        break;
    }
  }

  if (board_receive == true) {
    board_receive = false;
    Serial.print("APP -> BLE : ");
    for (uint8_t i = 0; i < receiveLen; i++) {
      Serial.write(receiveBuf[i]);
    }
    Serial.println();

    // Echo straight back to the phone
    if (BC7701.sendData(receiveBuf, receiveLen)) {
      Serial.println("(echoed back)");
    }
  }

  // Also allow typing in the USB serial monitor to send to the phone
  if (readSerialMonitor(sendBuf, sendLen)) {
    if (BC7701.sendData(sendBuf, sendLen)) {
      Serial.print("PC -> BLE sent: ");
      for (uint8_t i = 0; i < sendLen; i++) Serial.write(sendBuf[i]);
      Serial.println();
    } else {
      Serial.println("PC -> BLE send FAILED (not connected?)");
    }
  }
}
