#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

// PCA9685 is wired on the main header's I2C0 (D18/D19 -> Wire), not Wire1.
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(0x40, Wire);

#define SERVO_CH 0
#define SERVO_MIN 205   // ~1000us -> 0 deg
#define SERVO_MAX 410   // ~2000us -> 180 deg
#define CENTER    90    // re-calibrated: horn re-mounted at true center (2026-08-25)

int angleToPulse(int angle) {
  angle = constrain(angle, 0, 180);
  return map(angle, 0, 180, SERVO_MIN, SERVO_MAX);
}

void setAngle(int angle) {
  pwm.setPWM(SERVO_CH, 0, angleToPulse(angle));
  Serial.print("Angle: ");
  Serial.println(angle);
}

void setup() {
  Serial.begin(115200);
  pwm.begin();
  pwm.setPWMFreq(50);
  delay(10);
  Serial.println("Centering at 90 deg first...");
  setAngle(CENTER);
  delay(1500);
}

void loop() {
  // CENTER -> -70, 5-deg steps, pausing each step so you can watch/listen
  Serial.println("--- sweeping toward -70 ---");
  for (int a = CENTER; a >= CENTER - 70; a -= 5) {
    setAngle(a);
    delay(500);
  }
  delay(1000);

  Serial.println("--- back to center ---");
  for (int a = CENTER - 70; a <= CENTER; a += 5) {
    setAngle(a);
    delay(500);
  }
  delay(1000);

  // CENTER -> +70
  Serial.println("--- sweeping toward +70 ---");
  for (int a = CENTER; a <= CENTER + 70; a += 5) {
    setAngle(a);
    delay(500);
  }
  delay(1000);

  Serial.println("--- back to center ---");
  for (int a = CENTER + 70; a >= CENTER; a -= 5) {
    setAngle(a);
    delay(500);
  }
  delay(2000);
}
