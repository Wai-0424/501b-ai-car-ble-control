/*************************************************
  BMduino-UNO car control firmware.
  BLE (BMC77M001) on SPI1 header's Serial3 (D27/D28) receives dual-joystick
  move packets from the Android app (CarBleController.kt's sendMove) and drives:
    - Steering servo  : PCA9685 CH0 (I2C0 / Wire, D18/D19)
    - Drive direction  : PCA9685 CH1(IN1) / CH2(IN2) -> XY-160D
    - Drive speed(ENA) : BMduino D5 (~PWM, 3.3V) -> 74HC14 level shift -> XY-160D ENA (5V)

  Protocol: 3-byte move packet ['M', throttleByte, steerByte]
    throttleByte: 0-255, 128 = stop; >128 forward, <128 backward (magnitude scales to 0-255)
    steerByte   : 0-255, 128 = center (CENTER deg); >128 right, <128 left (scales to CENTER +/- STEER_OFFSET)
  App sends this at a fixed ~20Hz rate the whole time it's connected (release = neutral 128/128),
  so the packet itself doubles as a link heartbeat; see the watchdog in loop() below.

  Calibration (see docs/CALIBRATION.md):
    Servo mechanical center = 90 deg (horn re-mounted at true center, 2026-08-25;
      old 130 deg value was calibrated against the previous horn position)
    IN1=PCA9685 CH1, IN2=PCA9685 CH2 (not CH2/CH3 - fixed 2026-08-20)
**************************************************/
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include "BM7701-00-1.h"

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(0x40, Wire);
BM7701_00_1 BC7701(&Serial3);

// ---- Motor (XY-160D via PCA9685 direction + D5/74HC14 speed) ----
#define IN1_CH     1
#define IN2_CH     2
#define ENA_PIN    5

// ---- Steering servo (PCA9685 CH0) ----
#define SERVO_CH    0
#define SERVO_MIN   205   // ~1000us -> 0 deg
#define SERVO_MAX   410   // ~2000us -> 180 deg
#define CENTER      90    // horn re-mounted at true mechanical center (2026-08-25)
#define STEER_OFFSET 70   // deg left/right of center; re-validated with servo_endpoint_test
                           // against the new horn position, no binding up to +-70 (2026-08-25)

// ---- Move packet protocol ----
#define CMD_MOVE          'M'
#define THROTTLE_DEADZONE 6     // +/- around neutral (128) treated as stop
#define STEER_DEADZONE    6     // +/- around neutral (128) treated as centered
#define COMMAND_TIMEOUT_MS 300  // failsafe if no move packet arrives for this long

// ---- BLE advertising config ----
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
uint32_t lastHeartbeat = 0;
uint32_t lastCommandMillis = 0;
bool failsafeTriggered = false;

// ---------------- motor / servo helpers ----------------
int angleToPulse(int angle) {
  angle = constrain(angle, 0, 180);
  return map(angle, 0, 180, SERVO_MIN, SERVO_MAX);
}
void setSteer(int angle) {
  pwm.setPWM(SERVO_CH, 0, angleToPulse(angle));
}
void motorForward(int speed) {
  pwm.setPWM(IN1_CH, 4096, 0);
  pwm.setPWM(IN2_CH, 0, 0);
  analogWrite(ENA_PIN, speed);
}
void motorBackward(int speed) {
  pwm.setPWM(IN1_CH, 0, 0);
  pwm.setPWM(IN2_CH, 4096, 0);
  analogWrite(ENA_PIN, speed);
}
void motorBrake() {
  pwm.setPWM(IN1_CH, 0, 0);
  pwm.setPWM(IN2_CH, 0, 0);
  analogWrite(ENA_PIN, 0);
}

void handleMovePacket(uint8_t throttleByte, uint8_t steerByte) {
  int throttleDiff = (int)throttleByte - 128;
  if (abs(throttleDiff) <= THROTTLE_DEADZONE) {
    motorBrake();
  } else {
    int speed = constrain(abs(throttleDiff) * 255 / 127, 0, 255);
    if (throttleDiff > 0) motorForward(speed);
    else motorBackward(speed);
  }

  int steerDiff = (int)steerByte - 128;
  if (abs(steerDiff) <= STEER_DEADZONE) {
    setSteer(CENTER);
  } else {
    int angle = CENTER + (steerDiff * STEER_OFFSET) / 127;
    angle = constrain(angle, CENTER - STEER_OFFSET, CENTER + STEER_OFFSET);
    setSteer(angle);
  }

  lastCommandMillis = millis();
  failsafeTriggered = false;
}

// ---------------- setup ----------------
void setup() {
  delay(60);
  Serial.begin(115200);

  pwm.begin();
  pwm.setPWMFreq(50);
  pinMode(ENA_PIN, OUTPUT);
  motorBrake();
  setSteer(CENTER);
  lastCommandMillis = millis();
  failsafeTriggered = true;
  Serial.println("Motor/servo initialized to safe state (braked, centered).");

  BC7701.begin(BAUD_115200);
  Serial.println("Starting BLE init sequence...");
  while (sel != 10) {
    switch (sel) {
      case 1: if (BC7701.setAddress(BDAddress)) sel++; else { Serial.println("1 setAddress FAIL"); sel = 0xFF; } break;
      case 2: if (BC7701.setName(sizeof(BDName), BDName)) sel++; else { Serial.println("2 setName FAIL"); sel = 0xFF; } break;
      case 3: if (BC7701.setAdvIntv(ADV_MIN / 0.625, ADV_MAX / 0.625, 7)) sel++; else { Serial.println("3 setAdvIntv FAIL"); sel = 0xFF; } break;
      case 4: if (BC7701.setAdvData(APPEND_NAME, sizeof(Adata), Adata)) sel++; else { Serial.println("4 setAdvData FAIL"); sel = 0xFF; } break;
      case 5: if (BC7701.setScanData(sizeof(Sdata), Sdata)) sel++; else { Serial.println("5 setScanData FAIL"); sel = 0xFF; } break;
      case 6: if (BC7701.setTXpower(TX_POWER)) sel++; else { Serial.println("6 setTXpower FAIL"); sel = 0xFF; } break;
      case 7: if (BC7701.setCrystalOffset(XTAL_CLOAD)) sel++; else { Serial.println("7 setCrystalOffset FAIL"); sel = 0xFF; } break;
      case 8: if (BC7701.setFeature(FEATURE_DIR, AUTO_SEND_SATUS)) sel++; else { Serial.println("8 setFeature FAIL"); sel = 0xFF; } break;
      case 9: if (BC7701.setAdvCtrl(ENABLE)) { initOK = true; sel++; } else { Serial.println("9 setAdvCtrl FAIL"); sel = 0xFF; } break;
      case 0xFF:
        Serial.println("BLE init FAILED - check wiring/power to BLE module.");
        initOK = false;
        sel = 10;
        break;
    }
  }
  delay(650);
  Serial.print("BLE init sequence done. initOK=");
  Serial.println(initOK);
}

// ---------------- loop ----------------
void loop() {
  if (board_connect && !failsafeTriggered && millis() - lastCommandMillis > COMMAND_TIMEOUT_MS) {
    motorBrake();
    setSteer(CENTER);
    failsafeTriggered = true;
    Serial.println("Watchdog failsafe: no move packet received in time");
  }

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
        if (!board_connect) {
          board_connect = true;
          board_receive = false;
          Serial.println("BLE Connected");
          if (!board_conIntv) {
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
        Serial.println("BLE Disconnected - failsafe: brake + center");
        motorBrake();
        setSteer(CENTER);
        failsafeTriggered = true;
        break;
      case DATA_RECEIVED:
        board_receive = true;
        break;
      case API_ERROR:
        break;
    }
  }

  if (board_receive) {
    board_receive = false;

    if (receiveLen == 3 && receiveBuf[0] == CMD_MOVE) {
      handleMovePacket(receiveBuf[1], receiveBuf[2]);
      Serial.print("APP -> BLE : move throttle=");
      Serial.print(receiveBuf[1]);
      Serial.print(" steer=");
      Serial.println(receiveBuf[2]);
    } else {
      Serial.print("APP -> BLE : unrecognized packet, len=");
      Serial.println(receiveLen);
    }

    // echo back so the app's log shows confirmation
    BC7701.sendData(receiveBuf, receiveLen);
  }
}
