#include <Wire.h> 
#include <LiquidCrystal_I2C.h> //請先安裝LiquidCrystal_I2C程式庫，作者是Frank de或者Macro  Schwartz都可以
LiquidCrystal_I2C lcd(0x27,16,2);//設定LCD位址與大小

void setup()
{
  lcd.init();   //初始化LCD
  lcd.backlight(); //開啟LCD背光
}

void loop(){
  lcd.setCursor(3,0);//設定游標
  lcd.print("Hello, world!");//印出文字
  lcd.setCursor(2,1);//設定游標
  lcd.print("ESP32 LCD Test!"); //印出文字
  delay(1000);
  lcd.clear();//清除所有內容
  delay(1000);
}
