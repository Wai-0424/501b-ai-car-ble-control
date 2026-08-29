#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

// PCA9685 is wired on the main header's I2C0 (D18/D19 -> Wire).
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(0x40, Wire);

#define SERVO_CH 0
#define SERVO_MIN 205   // ~1000us -> 0 deg
#define SERVO_MAX 410   // ~2000us -> 180 deg
#define CENTER    90    // re-calibrated: horn re-mounted at true center (2026-08-25)
#define HOLD_AT   60

int angleToPulse(int angle) {
  angle = constrain(angle, 0, 180);
  return map(angle, 0, 180, SERVO_MIN, SERVO_MAX);
}

void setup() {
  Serial.begin(115200);
  pwm.begin();
  pwm.setPWMFreq(50);
  delay(10);
  pwm.setPWM(SERVO_CH, 0, angleToPulse(HOLD_AT));
  Serial.print("Holding at ");
  Serial.print(HOLD_AT);
  Serial.println(" deg.");
}

void loop() {
  // PCA9685 keeps outputting the PWM on its own; nothing to do here.
}
