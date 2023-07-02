#include "esp_camera.h"
#include "FS.h" //sd card esp32
#include "SD_MMC.h" //sd card esp32

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
  //設定鏡頭腳位及格式
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
  //解析度：FRAMESIZE_ + QVGA|CIF|VGA|SVGA|XGA|SXGA|UXGA
  config.frame_size = FRAMESIZE_XGA;//XGA=1024*768
  //設定鏡頭選項
  esp_err_t err = esp_camera_init(&config);
  if (err == ESP_OK) {
    Serial.println("鏡頭啟動成功");
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
  }  else {
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
}

int i = 0; //檔案序號
void loop() {
  int pir = digitalRead(12);
  if (pir == 1) {
    i++;
    Serial.println("有人經過，開始拍照");
    camera_fb_t * fb = esp_camera_fb_get(); //擷取影像
    if (!fb) {
      Serial.println("拍照失敗，請檢查");
    } else {
      //儲存到SD卡：SavePictoSD(檔名,影像)
      SavePictoSD("/pic" + String(i) + ".jpg", fb);
    }
    esp_camera_fb_return(fb);//清除影像緩衝區
  }
  Serial.println("無人");
  delay(1000);
}
