/*************************************************
  Holds the steering servo at a fixed 90 deg (1500us standard center pulse)
  so you can loosen the servo horn, re-align it straight, and re-tighten
  at the true mechanical center.

  This intentionally does NOT use CENTER=130 from car_control.ino - that
  130 deg was calibrated against the OLD horn position. Once the horn is
  re-mounted at 90 deg here, CALIBRATION.md and car_control.ino's CENTER
  should be updated back to 90 (or whatever the new true center turns out
  to be, if it's not exactly 90 after assembly).

  PCA9685 is on the main header's I2C0 (D18/D19 -> Wire).
**************************************************/
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(0x40, Wire);

#define SERVO_CH   0
#define SERVO_MIN  205   // ~1000us -> 0 deg
#define SERVO_MAX  410   // ~2000us -> 180 deg
#define ZERO_ANGLE 90    // standard 1500us center pulse

int angleToPulse(int angle) {
  angle = constrain(angle, 0, 180);
  return map(angle, 0, 180, SERVO_MIN, SERVO_MAX);
}

void setup() {
  Serial.begin(115200);
  pwm.begin();
  pwm.setPWMFreq(50);
  delay(10);
  pwm.setPWM(SERVO_CH, 0, angleToPulse(ZERO_ANGLE));
  Serial.println("Holding at 90 deg (1500us) - safe to loosen/re-align the horn now.");
}

void loop() {
  // PCA9685 keeps outputting the PWM on its own; nothing to do here.
}
