#include <ESP8266WiFi.h>
#include "ESP8266WiFiMulti.h"
#include <BlynkSimpleEsp8266.h>
#include <Ticker.h>

#define OTA_BASIC
#define OTA_ELEGANT
#if defined(OTA_BASIC)
#include <ArduinoOTA.h>
#endif

#if defined(OTA_ELEGANT)
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>
#endif

/* Fill-in information from Blynk Device Info here */
#define BLYNK_TEMPLATE_ID           "TMPL60ZFH1wIB"
#define BLYNK_TEMPLATE_NAME         "PowerControl"
#define BLYNK_AUTH_TOKEN            "GmUiRdwdT722z8xYPXsWQOtNiJDdYWIy"
/* Comment this out to disable prints and save space */
#define BLYNK_PRINT Serial

ESP8266WiFiMulti WiFiMulti;
int led_light = 0;
int led_indicator = 1;
int fan = 14;
int blink_interval_wifi =1;
int blink_interval_upload =1;
Ticker ticker_led_indicator;

#if defined(OTA_ELEGANT)
AsyncWebServer server(80);
#endif

BLYNK_WRITE(V0)
{
    int pinValue = param.asInt();
    // You can also use:
    // String i = param.asStr();
    // double d = param.asDouble();
    Serial.print("V0 value is: ");
    Serial.println(pinValue);
    digitalWrite(led_light, pinValue);
}
BLYNK_WRITE(V1)
{
    int pinValue = param.asInt();
    // You can also use:
    // String i = param.asStr();
    // double d = param.asDouble();
    Serial.print("V1 value is: ");
    Serial.println(pinValue);
    digitalWrite(fan, pinValue);
}
void gpio_init()
{
    pinMode(led_light, OUTPUT);
    digitalWrite(led_light, LOW);
    pinMode(fan, OUTPUT);
    digitalWrite(fan, LOW);
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);
}
void ticker_led_count(){
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
}
void wifi_multi_init()
{
    WiFiMulti.addAP("scchang_iphone", "0928136004");
    WiFiMulti.addAP("linkou203-4F", "56665666");
    WiFi.mode(WIFI_STA); 
}
void wifi_multi_connect()
{
    Serial.println("\nWait for WiFi... ");
    ticker_led_indicator.attach(blink_interval_wifi, ticker_led_count);
    while(WiFiMulti.run() != WL_CONNECTED) {
        Serial.print(".");
        delay(500);
    }
    Serial.println('\n');
    Serial.print("Connected to ");
    Serial.println(WiFi.SSID());
    Serial.print("IP address:\t");
    Serial.println(WiFi.localIP());
    Serial.print("Password:\t");
    Serial.println(WiFi.psk());
    ticker_led_indicator.detach();
    digitalWrite(LED_BUILTIN, LOW);
}
void blynk_init()
{
    Blynk.begin(BLYNK_AUTH_TOKEN, WiFi.SSID().c_str(), WiFi.psk().c_str());
}
#if defined(OTA_BASIC)
void basic_ota_init()
{
    Serial.println("ota init ...");
    // Port defaults to 8266
    // ArduinoOTA.setPort(8266);

    // Hostname defaults to esp8266-[ChipID]
    ArduinoOTA.setHostname("esp8266-scchang");

    // No authentication by default
    ArduinoOTA.setPassword("scchang");    
    ArduinoOTA.onStart([]() {
        String type;
        if (ArduinoOTA.getCommand() == U_FLASH) {
            type = "sketch";
        } else {  // U_FS
            type = "filesystem";
        }
        
        // NOTE: if updating FS this would be the place to unmount FS using FS.end()
        Serial.println("Start updating " + type);
        ticker_led_indicator.attach(blink_interval_upload, ticker_led_count);
    });
    ArduinoOTA.onEnd([]() {
        Serial.println("\nEnd");
        ticker_led_indicator.detach();
        digitalWrite(LED_BUILTIN, HIGH);        
    });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    });
    ArduinoOTA.onError([](ota_error_t error) {
        Serial.printf("Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR) {
            Serial.println("Auth Failed");
        } else if (error == OTA_BEGIN_ERROR) {
            Serial.println("Begin Failed");
        } else if (error == OTA_CONNECT_ERROR) {
            Serial.println("Connect Failed");
        } else if (error == OTA_RECEIVE_ERROR) {
            Serial.println("Receive Failed");
        } else if (error == OTA_END_ERROR) {
            Serial.println("End Failed");
        }
        ticker_led_indicator.detach();
        digitalWrite(LED_BUILTIN, HIGH);        
    });
    ArduinoOTA.begin();
    Serial.println("ota ready ...");
}
#endif
#if defined(OTA_ELEGANT)
void async_elegant_ota_init()
{
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "Hi! I am ESP8266.");
  });

  AsyncElegantOTA.begin(&server);    // Start ElegantOTA
  server.begin();
  Serial.println("HTTP server started");
}
#endif
void setup()
{
    // Debug console
    Serial.begin(115200);
    gpio_init();
    wifi_multi_init();
    wifi_multi_connect();
    blynk_init();
    #if defined(OTA_BASIC)
    basic_ota_init();
    #endif
    #if defined(OTA_ELEGANT)
    async_elegant_ota_init();
    #endif
}
void loop()
{
    if(WiFiMulti.run() != WL_CONNECTED){
        wifi_multi_connect();
        blynk_init();
    }
    Blynk.run();
    #if defined(OTA_BASIC)
    ArduinoOTA.handle();
    #endif
}
