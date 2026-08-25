#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(0x40, Wire);

#define IN1_CH 1
#define IN2_CH 2
#define ENA_PIN 5

void setup() {
  Serial.begin(115200);
  pwm.begin();
  pwm.setPWMFreq(50);
  pinMode(ENA_PIN, OUTPUT);

  // BRAKE: stop and hold
  pwm.setPWM(IN1_CH, 0, 0);
  pwm.setPWM(IN2_CH, 0, 0);
  analogWrite(ENA_PIN, 0);

  Serial.println("Motor stopped (BRAKE).");
}

void loop() {
  // hold steady, nothing to do
}
