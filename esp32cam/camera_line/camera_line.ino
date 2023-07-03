#include "esp_camera.h"
#include "FS.h" //檔案系統
#include "SD_MMC.h" //記憶卡
#include <WiFi.h>
#include <WiFiClientSecure.h>
//char* ssid = "HUAWEI-B315-9878";
//char* password = "64TA6NG2DQR";
//char* ssid = "scchang_iphone";
//char* password = "0928136004";
char* ssid = "linkou203-4F";
char* password = "56665666";
String Linetoken = "WZOXUjuecnR60keRBxIKTZBOGXy2soFzvoshoJOVdsP"; //改為您的Line權杖密碼
String SendImageLine(String msg, camera_fb_t * fb) {
  WiFiClientSecure client_tcp;
  client_tcp.setInsecure(); //run version 1.0.5 or above
  if (client_tcp.connect("notify-api.line.me", 443)) {
    Serial.println("連線到Line成功");
    //組成HTTP POST表頭
    String head = "--Cusboundary\r\nContent-Disposition: form-data;";
    head += "name=\"message\"; \r\n\r\n" + msg + "\r\n";
    head += "--Cusboundary\r\nContent-Disposition: form-data; ";
    head += "name=\"imageFile\"; filename=\"esp32-cam.jpg\"";
    head += "\r\nContent-Type: image/jpeg\r\n\r\n";
    String tail = "\r\n--Cusboundary--\r\n";
    uint16_t imageLen = fb->len;
    uint16_t extraLen = head.length() + tail.length();
    uint16_t totalLen = imageLen + extraLen;
    //開始POST傳送
    client_tcp.println("POST /api/notify HTTP/1.1");
    client_tcp.println("Connection: close");
    client_tcp.println("Host: notify-api.line.me");
    client_tcp.println("Authorization: Bearer " + Linetoken);
    client_tcp.println("Content-Length: " + String(totalLen));
    client_tcp.println("Content-Type: multipart/form-data; boundary=Cusboundary");
    
    client_tcp.println();
    client_tcp.print(head);
    uint8_t *fbBuf = fb->buf;
    size_t fbLen = fb->len;
    Serial.println("傳送影像檔...");
    for (size_t n = 0; n < fbLen; n = n + 2048) {
      if (n + 2048 < fbLen) {
        client_tcp.write(fbBuf, 2048);
        fbBuf += 2048;
      } else if (fbLen % 2048 > 0) {
        size_t remainder = fbLen % 2048;
        client_tcp.write(fbBuf, remainder);
      }
    }
    client_tcp.print(tail);
    client_tcp.println();
    String Feedback = "";
    boolean state = false;
    int waitTime = 3000;//等候時間3秒鐘
    long startTime = millis();
    delay(1000);
    Serial.print("等候回應...");
    while ((startTime + waitTime) > millis()) {
      Serial.print(".");
      delay(100);
      while (client_tcp.available()) {
        //已收到回覆，依序讀取內容
        char c = client_tcp.read();
        Feedback += c;
      }
    }
    client_tcp.stop();
    return Feedback;
  }
  else {
    return "傳送失敗，請檢查網路設定";
  }
}

//拍照存檔SD卡副程式
void SavePictoSD(String filename, camera_fb_t * fb) {
  Serial.print("寫入檔案:" + filename + ",檔案大小=" );
  Serial.println(String(fb->len) + "bytes");
  fs::FS &fs = SD_MMC;//設定SD卡裝置
  File file = fs.open(filename, FILE_WRITE);//開啟檔案
  if (!file) {
    Serial.println("存檔失敗，請檢查SD卡");
  } else {
    file.write(fb->buf , fb->len); //
    Serial.println("存檔成功");
  }
}

void setup() {
  Serial.begin(115200);
  //初始化相機結束
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = 5;
  config.pin_d1 = 18;
  config.pin_d2 = 19;
  config.pin_d3 = 21;
  config.pin_d4 = 36;
  config.pin_d5 = 39;
  config.pin_d6 = 34;
  config.pin_d7 = 35;
  config.pin_xclk = 0;
  config.pin_pclk = 22;
  config.pin_vsync = 25;
  config.pin_href = 23;
  config.pin_sscb_sda = 26;
  config.pin_sscb_scl = 27;
  config.pin_pwdn = 32;
  config.pin_reset = -1;
  config.xclk_freq_hz = 20000000;
  config.fb_count = 2;
  config.pixel_format = PIXFORMAT_JPEG;
  config.jpeg_quality = 12;
  //設定解析度：FRAMESIZE_ + QVGA|CIF|VGA|SVGA|XGA|SXGA|UXGA
  config.frame_size = FRAMESIZE_VGA;//VGA=640*480(VGA格式較穩定)
  //Line notify don't accept bigger than XGA
  esp_err_t err = esp_camera_init(&config);
  if (err == ESP_OK) {
    Serial.println("鏡頭啟動成功");
    // setup stream ------------------------
    sensor_t * s = esp_camera_sensor_get();
    int res = 0;
    res = s->set_brightness(s, 1); //亮度:(-2~2)
    res = s->set_contrast(s, 1); //對比度:(-2~2)
    res = s->set_saturation(s, 1); //色彩飽和度:(-2~2)
    //res = s->set_special_effect(s, 0);//特殊效果:(0~6)
    //res = s->set_whitebal(s, 1);//啟動白平衡:(0或1)
    //res = s->set_awb_gain(s, 1);//自動白平衡增益:(0或1)
    //res = s->set_wb_mode(s, 0);//白平衡模式:(0~4)
    //res = set_exposure_ctrl(s, 1);;//曝光控制:(0或1)
    //res = set_aec2(s, 0);//自動曝光校正:(0或1)
    //res = set_ae_level(s, 0);//自動曝光校正程度:(-2~2)
    //res = set_aec_value(s, 300);//自動曝光校正值：(0~1200)
    //res = set_gain_ctrl(s, 1);//增益控制:(0或1)
    //res = set_agc_gain(s, 0);//自動增益:(0~30)
    //res = set_gainceiling(s, (gainceiling_t)0); //增益上限:(0~6)
    //res = set_bpc(s, 1);//bpc開啟:(0或1)
    //res = set_wpc(s, 1);//wpc開啟:(0或1)
    //res = set_raw_gma(s, 1);//影像GMA:(0或1)
    //res = s->set_lenc(s, 1);//鏡頭校正:(0或1)
    //res = s->set_hmirror(s, 1);//水平翻轉:(0或1)
    //res = s->set_vflip(s, 1);//垂直翻轉:(0或1)
    //res = set_dcw(s, 1);//dcw開啟:(0或1)
  } else {
    Serial.printf("鏡頭設定失敗，5秒後重新啟動");
    delay(5000);
    ESP.restart();
  }

  //設定SD卡
  if (!SD_MMC.begin()) {
    Serial.println("SD卡讀取失敗，5秒後重新啟動");
    delay(5000);
    ESP.restart();
  } else {
    Serial.println("SD卡偵測成功");
  }

  //開始網路連線
  Serial.print("連線到WiFi:");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
}

int i = 0; //檔案序號
void loop() {
  int pir = digitalRead(12);
  if (pir == 1) {
	i++;
    camera_fb_t * fb = esp_camera_fb_get(); //擷取影像
    if (!fb) {
      Serial.println("拍照失敗，請檢查");
    } else {
      //儲存到SD卡:SavePictoSD(檔名,影像);
      SavePictoSD("/pic" + String(i) + ".jpg", fb);
      //傳送到Line:SendImageLine(訊息,影像);
      String payload = SendImageLine("有人進出房間，請查看影像", fb);
      Serial.println(payload);
      esp_camera_fb_return(fb);//清除影像緩衝區
    }
  }
  Serial.println("無人");
  delay(1000);
}
