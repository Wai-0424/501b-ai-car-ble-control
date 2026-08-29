#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

// PCA9685 is on the main header's I2C0 (D18/D19 -> Wire).
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(0x40, Wire);

#define IN1_CH 1      // XY-160D IN1, via PCA9685
#define IN2_CH 2      // XY-160D IN2, via PCA9685
#define ENA_PIN 5     // XY-160D ENA, via BMduino D5 -> 74HC14 -> ENA (5V PWM)

void motorForward(int speed) {   // speed: 0~255
  pwm.setPWM(IN1_CH, 4096, 0);   // full ON
  pwm.setPWM(IN2_CH, 0, 0);      // full OFF
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

void setup() {
  Serial.begin(115200);
  pwm.begin();
  pwm.setPWMFreq(50);
  pinMode(ENA_PIN, OUTPUT);
  motorBrake();
  Serial.println("Motor test: braking for 3s first, wheels should be off the ground.");
  delay(3000);
}

void loop() {
  Serial.println("Forward @ speed 120");
  motorForward(120);
  delay(1500);

  Serial.println("Brake");
  motorBrake();
  delay(1000);

  Serial.println("Backward @ speed 120");
  motorBackward(120);
  delay(1500);

  Serial.println("Brake");
  motorBrake();
  delay(1500);
}
