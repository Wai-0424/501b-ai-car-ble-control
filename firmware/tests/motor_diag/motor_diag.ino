#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(0x40, Wire);

#define IN1_CH 1
#define IN2_CH 2
#define ENA_PIN 5

void setIN(int in1, int in2) {
  pwm.setPWM(IN1_CH, in1 ? 4096 : 0, in1 ? 0 : 0);
  pwm.setPWM(IN2_CH, in2 ? 4096 : 0, in2 ? 0 : 0);
}

void setup() {
  Serial.begin(115200);
  pwm.begin();
  pwm.setPWMFreq(50);
  pinMode(ENA_PIN, OUTPUT);
  analogWrite(ENA_PIN, 0);
  setIN(0, 0);
  delay(1000);
}

void hold(const char* label, int in1, int in2, int ena) {
  Serial.print("=== ");
  Serial.print(label);
  Serial.print("  IN1=");
  Serial.print(in1);
  Serial.print(" IN2=");
  Serial.print(in2);
  Serial.print(" ENA=");
  Serial.println(ena);
  setIN(in1, in2);
  analogWrite(ENA_PIN, ena);
  delay(4000);
}

void loop() {
  hold("BRAKE (00)", 0, 0, 0);
  hold("PATTERN A (10)", 1, 0, 150);
  hold("BRAKE (00)", 0, 0, 0);
  hold("PATTERN B (01)", 0, 1, 150);
  hold("BRAKE (00)", 0, 0, 0);
  hold("FLOATING (11) - should coast freely, try spinning wheel by hand", 1, 1, 0);
  delay(2000);
}
