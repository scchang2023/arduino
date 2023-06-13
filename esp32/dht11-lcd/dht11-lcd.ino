#include <SimpleDHT.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
int pinDHT11 = 4;
SimpleDHT11 dht11(pinDHT11);
LiquidCrystal_I2C lcd(0x27, 16, 2);
void setup() {
  Serial.begin(115200);
  lcd.init();
  lcd.backlight();
}

void loop() {
 Serial.println("=================================");
  Serial.println("Sample DHT11...");
  byte temperature = 0;
  byte humidity = 0;
  int err = SimpleDHTErrSuccess;
  if ((err = dht11.read(&temperature, &humidity, NULL)) != 
    SimpleDHTErrSuccess) {
    Serial.print("Read DHT11 failed, err="); Serial.println(err); delay(1000);
    return;
  }

  Serial.print("Sample OK: ");
  Serial.print((int)temperature); Serial.print(" *C, ");
  Serial.print((int)humidity); Serial.println(" H");
  lcd.clear();
  lcd.setCursor(0, 0); //先設定游標
  lcd.print("Temperature: ");//顯示溫度
  lcd.print((int)temperature);
  lcd.print("c");
  lcd.setCursor(0, 1); //顯示溼度
  lcd.print("Humidity:    ");
  lcd.print((int)humidity);
  lcd.print("%");
  delay(1500);
}
