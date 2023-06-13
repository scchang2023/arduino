#include <BluetoothSerial.h>
BluetoothSerial BT;//宣告藍芽物件，名稱為BT

void setup() {
  Serial.begin(115200);
  BT.begin("SCC");//請改名例如英文+生日
}

void loop() {
  BT.println("Hello World!");
  delay(1000);
}
