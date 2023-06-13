#include <ESP8266WiFi.h>
//請修改以下參數--------------------------------------------
char ssid[] = "linkou203-4F";
char password[] = "56665666";

int light_led = 0;
int fan = 14;

WiFiServer server(80); //宣告伺服器位在80 port

void setup()
{
  Serial.begin(115200);
  Serial.print("開始連線到無線網路SSID:");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }
  Serial.println("連線完成");
  server.begin();
  Serial.print("伺服器已啟動，http://");
  Serial.println(WiFi.localIP());
  pinMode(light_led, OUTPUT);
  pinMode(fan, OUTPUT);

}

void loop()
{
  //宣告一個連線
  WiFiClient client = server.available();
  if (client) {
    //有人連入時
    Serial.println("使用者連入");
    //-------------網頁的html部份開始--------------
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/html");
    client.println("");
    client.println("<!DOCTYPE HTML>");
    client.println("<html><head><meta charset='utf-8'></head>");
    client.println("<br>");
    client.println("<h1>電源控制</h1>");
    //HTML超連結指令
    client.println("<a href='/LIGHT_LED=ON'>開啟照明LED</a><br>");
    client.println("<a href='/LIGHT_LED=OFF'>關閉照明LED</a><br>");
    client.println("<a href='/FAN=ON'>開啟風扇</a><br>");
    client.println("<a href='/FAN=OFF'>關閉風扇</a><br>");


    client.println("</html>");
    //-------------網頁的html部份結束--------------
    //取得使用者輸入的網址
    String request = client.readStringUntil('\r');
    Serial.println(request);
    //判斷超連結指令
    //網址內包含Gled=ON，就開啟綠燈，如果Gled=OFF，關閉綠燈
    if (request.indexOf("LIGHT_LED=ON") >= 0) { digitalWrite(light_led, HIGH); }
    if (request.indexOf("LIGHT_LED=OFF") >= 0) { digitalWrite(light_led, LOW); }
    if (request.indexOf("FAN=ON") >= 0) { digitalWrite(fan, HIGH); }
    if (request.indexOf("FAN=OFF") >= 0) { digitalWrite(fan, LOW); }

    Serial.println("完成");
    client.stop();//停止連線
  }
}
