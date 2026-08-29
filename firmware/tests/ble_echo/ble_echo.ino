// BLE (BMC77M001) is on the SPI1 header's UART3 (D27=TX3, D28=RX3), 115200bps.
void setup() {
  Serial.begin(115200);   // USB debug
  Serial3.begin(115200);  // BLE module
  Serial.println("BLE echo test ready. Send from phone BLE terminal app.");
}

void loop() {
  if (Serial3.available()) {
    char c = Serial3.read();
    Serial.write(c);     // show on USB serial monitor
    Serial3.write(c);    // echo back to phone
  }
}
