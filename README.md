# Arduino
[Arduino IDE](https://www.arduino.cc/en/software)
## ESP32
## ESP8266
### 環境設定
- Board Manager：esp8266 by ESP8266 Community v3.1.2
- Board："ESP8266 > LOLIN(WeMos)D1 R1"
### OTA
- Basic OTA
    - #include <ArduinoOTA.h>
    - File > Examples > Arduino OTA > BasicOTA.ino，使用內建範例修改。
- Ele OTA
    - install Library AsyncElegantOTA
    - install Library ESPAsyncTcp
    - #include <ESPAsyncTCP.h>
    - #include <ESPAsyncWebServer.h>
    - #include <AsyncElegantOTA.h>
### Blynk
- install Library Blynk
- #include <BlynkSimpleEsp8266.h>
## ESP32-CAM
### 環境設定
- Board Manager："ESP32 by Espressif Systems" v1.0.6
- Board："ESP32 > ESP32 Wrover Module"
- Partition Scheme ："Huge APP"(3MB NO OTA/1mb spiffs)
- download mode：Flash按住不放 + Reset，再放開。
- 使用範例->ESP32->Camera->CameraWebServer
- 註解 #define CAMERA_MODEL_WROVER_KIT
- 打開 #define CAMERA_MODEL_AI_THINKER
### Issue
- 使用 Arduino core for esp32 V1.0.5 以上版本就無法使用人臉偵測與人臉辨識，一啟動人臉偵測就出錯重啟。
    - 主要是釋放net_boxes記憶體時產生錯誤
    - 修改部份如下，修改完後 v1.0.5 以上的版本就可以使用了。
    ```
    app_httpd.cpp

    stream_handler 與 capture_handler 這兩個 function

    //free(net_boxes->score);
    //free(net_boxes->box);
    //free(net_boxes->landmark);
    //free(net_boxes);  

    dl_lib_free(net_boxes->score);
    dl_lib_free(net_boxes->box);
    dl_lib_free(net_boxes->landmark);
    dl_lib_free(net_boxes);
    net_boxes = NULL;
    ```
## Line Notify 服務
### 申請 Line Notify 權杖
- 至 [Line Notify](https://notify-bot.line.me/zh_TW/) 網站申請開通服務，點選右上角的登入
- 點選右上角的「各人頁面」，即可進入「已連動服務」管理頁面。
- 在設定權杖頁面分別輸入名稱、對象，完成輸入後按下發行，可獲得權杖。
### Issue
```
client_tcp.setInsecure(); //run version 1.0.5 or above
// connect 會回傳 fail，如已確定網址是安全的，可在以上呼叫 setInsecure。
if (client_tcp.connect("notify-api.line.me", 443))
```
## MQTT服務
- install library pubsubclient
- 使用 mqttlens 測試