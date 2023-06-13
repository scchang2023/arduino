void setup() {
  Serial.begin(115200);
  pinMode(4, OUTPUT);
}
void loop() {
  Serial.println("注意開啟閃光燈");
  digitalWrite(4, HIGH);
  delay(200);
  Serial.println("關閉閃光燈");
  digitalWrite(4, LOW);
  delay(5000);
}