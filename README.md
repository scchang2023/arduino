# Arduino
## ESP32
## ESP8266
## ESP32-CAM
### 環境設定
- Board："ESP32 Wrover 
Module"
- Partition Scheme ："Huge APP"(3MB NO OTA/1mb spiffs)
- download mode：Flash按住不放 + Rest，再放開。
- 使用範例->ESP32->Camera->CameraWebServer
- 註解：CAMERA_MODEL_WROVER_KIT
- 打開：CAMERA_MODEL_AI_THINKER
### issue
- 使用 Arduino core for esp32 V1.0.5 以上版本就無法使用人臉偵測與人臉辨識，一啟動人臉偵測就出錯重啟！
    - 釋放net_boxes記憶體時產生錯誤
    - 修改部份如下，修改完後 v1.0.5以上的版本就可以使用了。
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