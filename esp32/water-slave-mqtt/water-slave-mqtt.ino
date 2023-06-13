#include <SimpleDHT.h>
#include <LiquidCrystal_I2C.h>
#include "Timer.h"
#include <WiFi.h>
#include <PubSubClient.h> // install library PubSubClient

typedef enum {
	SENSOR_TEMP,
	SENSOR_HUMI,
	SENSOR_MOIS,
	SENSOR_MAX
}sensorData_t;
byte SensorData[SENSOR_MAX];

// ------ DHT11 Sensor Setting ------
#define PIN_DHT11_S 4
SimpleDHT11 dht11(PIN_DHT11_S);

// ------ Moisture Sensor Setting ------
#define PIN_FC28_AO 36
#define MOIS_AD_MAX 0xFFF

// ------ LCD Setting ------
#define LCD_I2C_ADDR 0x27 //SDA = GPIO 21, SCL = GPIO 22
#define LCD_TEMP_STR_X 0
#define LCD_TEMP_STR_Y 0
#define LCD_HUM_STR_X 0
#define LCD_HUM_STR_Y 1
#define LCD_MOI_STR_X 11
#define LCD_MOI_STR_Y 0
#define LCD_MOI_NUM_X 11
#define LCD_MOI_NUM_Y 1
LiquidCrystal_I2C lcd(LCD_I2C_ADDR, 16, 2);

// ------ Timer Setting ------
#define TIME_INTERVAL_SHOW_SENSOR 2
#define TIME_INTERVAL_CHECK_WIFI_MQTT 60
#define TIME_INTERVAL_SEND_PUB 5
Timer tShowSensorInfo;
Timer tChkWifiMQT, tSendPubData2MQTT;

// ------ WIFI Setting ------
#define PIN_WIFI_LED 2
//char* ssid = "linkou203-4F";
//char* password = "56665666";
char* ssid = "scchang_iphone";
char* password = "26082638";
WiFiClient WifiClient;

// ------ MQTT Setting ------
//char* MQTTServer = "mqtt.eclipseprojects.io";// free server, don't need to register
char* MQTTServer = "test.mosquitto.org";// free server, dont need to register
int MQTTPort = 1883;//MQTT Port
char* MQTTUser = "";
char* MQTTPassword = "";
PubSubClient MQTTClient(WifiClient);
char MQTTPubTopic[SENSOR_MAX][32] = {
	"SCCHANG/temp", //to phone, server
	"SCCHANG/humi", //to phone, server
	"SCCHANG/mois", //to phone, server
};

// ------ other ------
#define SERIAL_BAUD_RATE 115200
enum {
	WIFI_STATUS_DISCONNECTED,
	WIFI_STATUS_CONNECTED,
	WIFI_STATUS_MAX,
};

//==============================================================
int GetDht11TempHum(byte *temp, byte *hum)
{
	*temp=0;
	*hum=0;
	return dht11.read(temp, hum, NULL);
}
void ShowDht11TempHum(byte temp, byte hum)
{
	char str[32];
	sprintf(str,"Temp:%dC, Hum:%d%%", temp, hum);
	Serial.println(str);
	lcd.setCursor(LCD_TEMP_STR_X, LCD_TEMP_STR_Y);
	sprintf(str,"Temp:%dC", temp);
	lcd.print(str);
	lcd.setCursor(LCD_HUM_STR_X, LCD_HUM_STR_Y);
	sprintf(str,"Hum:%d%%", hum);
	lcd.print(str);	
}
void ShowDht11Err(int err)
{
	char str[32];
	sprintf(str,"Dht11 Err = 0x%x", err);
	Serial.println(str);
	lcd.setCursor(LCD_TEMP_STR_X, LCD_TEMP_STR_Y);
	strcpy(str,"Dht11 Err");
	lcd.print(str);
}
void InitFc28Moisture(void)
{
	Serial.println("InitFc28Moisture !!!");
	pinMode(PIN_FC28_AO, INPUT);
}
int GetFc28Moisture(void)
{
	return analogRead(PIN_FC28_AO);
}
void InitLCD(void)
{
	lcd.init();
	lcd.backlight();
	lcd.clear();
}
void InitWifiLED(void)
{
	Serial.println("InitWifiLED !!!");
	pinMode(PIN_WIFI_LED, OUTPUT);
	SetWifiLED(WIFI_STATUS_DISCONNECTED);
}
void SetWifiLED(byte status)
{
	Serial.print("SetWifiLED : ");
	Serial.println(status);
	if(status == WIFI_STATUS_DISCONNECTED){
		digitalWrite(PIN_WIFI_LED, LOW);
	}else{
		digitalWrite(PIN_WIFI_LED, HIGH);
	}
}
int NormalizedFc28Moisture(int moi)
{
	return 100-(moi*100/MOIS_AD_MAX);
}
void ShowFc28Moisture(int moi)
{
	int moiPercent=0;
	char str[32];
	moiPercent = NormalizedFc28Moisture(moi);
	sprintf(str,"Moi: 0x%x, %d%%", moi, moiPercent);
	Serial.println(str);
	lcd.setCursor(LCD_MOI_STR_X, LCD_MOI_STR_Y);
	lcd.print("Moi:");
	lcd.setCursor(LCD_MOI_NUM_X, LCD_MOI_NUM_Y);
	lcd.print(moiPercent);
	lcd.print("%");	
}
void OnTimerShowSensorInfo(void)
{
	byte temp, hum;
	int err, moi;
	Serial.println("=========== OnTimerShowSensorInfo ===========");
	lcd.clear();
	err=GetDht11TempHum(&temp, &hum);
	if(err==SimpleDHTErrSuccess){
		ShowDht11TempHum(temp, hum);
		SensorData[SENSOR_TEMP]=temp;
		SensorData[SENSOR_HUMI]=hum;
	}else{
		ShowDht11Err(err);
	}
	moi=GetFc28Moisture();
	ShowFc28Moisture(moi);
	SensorData[SENSOR_MOIS]=(byte)NormalizedFc28Moisture(moi);
}
void OnTimerChkWifiMQTTConnect(void)
{
	Serial.println("=========== OnTimerChkWifiMQTTConnect ===========");
	//Restart to connect WIFI if WIFI is disconnected
	if(WiFi.status()!= WL_CONNECTED){
		SetWifiLED(WIFI_STATUS_DISCONNECTED);
		WifiConnecte();
	}
	if(WiFi.status()== WL_CONNECTED){
		SetWifiLED(WIFI_STATUS_CONNECTED);
		//Restart to connect MQTT if MQTT is disconnected
		if(!MQTTClient.connected()){
			MQTTConnecte();
		}
	}
}
void OnTimerSendPubData2MQTT(void)
{
	char str[32];
	Serial.println("=========== OnTimerSendPubData2MQTT ===========");
	if(WiFi.status()!= WL_CONNECTED){
		SetWifiLED(WIFI_STATUS_DISCONNECTED);
		Serial.println("Wifi is disconnected!");
		return;
	}	
	if(MQTTClient.connected()){
		for(int i=SENSOR_TEMP; i<SENSOR_MAX; i++){
			MQTTClient.publish(MQTTPubTopic[i], String(SensorData[i]).c_str());
			sprintf(str,"%s: %d",MQTTPubTopic[i], SensorData[i]);
			Serial.println(str);			
		}		
		Serial.println("The publish-data have pulbished to MQTT Broker");
	}
}
void WifiConnecte()
{
	//Start to connect WIFI
	long StartTime = millis();
	Serial.println("Start to connect WIFI!");
	WiFi.begin(ssid, password);
	while(WiFi.status()!=WL_CONNECTED){
		delay(500);
		Serial.print(".");
		if(millis()-StartTime >= 10000){
			Serial.println(".");
			Serial.println("Failed to connect Wifi!");
			return;
		}
	}
	Serial.println("Wifi was connected sucessfully!");
	Serial.print("IP Address:");
	Serial.println(WiFi.localIP());
}
void MQTTConnecte()
{
	//Start to connect MQTT
	long StartTime = millis();
	Serial.println("Start to connect MQTT!");
	MQTTClient.setServer(MQTTServer, MQTTPort);
	//MQTTClient.setCallback(MQTTCallback);
	while (!MQTTClient.connected()){
	//if(!MQTTClient.connected()){
		//random ClietID
		String  MQTTClientid = "esp32-" + String(random(1000000, 9999999));
		if(MQTTClient.connect(MQTTClientid.c_str(), MQTTUser, MQTTPassword)){
			Serial.println("MQTT is connnected!");
			//Subscribe Topic
		}else{
			Serial.print("Failed to connect MQTT, Err = ");
			Serial.println(MQTTClient.state());
			if(millis()-StartTime < 40000){
				Serial.println("Reconnect MQTT 5 second later");
				delay(5000);
			}else{
				Serial.println("Timout");
				break;
			}
		}
	}
}
void MQTTCallback(char* topic, byte* payload, unsigned int length)
{
	Serial.println("Received the notification of subscription!!");
	Serial.println(topic);
	String payloadString;//transfer the payload to string
	//show the content of subscription
	for(int i = 0; i < length; i++){
		payloadString = payloadString + (char)payload[i];
	}
	Serial.println(payloadString);
}
void setup(void)
{
	memset(&SensorData,0,sizeof(SensorData));
	Serial.begin(SERIAL_BAUD_RATE);
	InitWifiLED();
	InitLCD();
	InitFc28Moisture();
	OnTimerShowSensorInfo();
	OnTimerChkWifiMQTTConnect();
	tShowSensorInfo.every(TIME_INTERVAL_SHOW_SENSOR *1000,OnTimerShowSensorInfo);
	tChkWifiMQT.every(TIME_INTERVAL_CHECK_WIFI_MQTT *1000,OnTimerChkWifiMQTTConnect);
	tSendPubData2MQTT.every(TIME_INTERVAL_SEND_PUB *1000,OnTimerSendPubData2MQTT);
}
void loop(void)
{
	tShowSensorInfo.update();
	tChkWifiMQT.update();
	tSendPubData2MQTT.update();
	delay(50);
}
