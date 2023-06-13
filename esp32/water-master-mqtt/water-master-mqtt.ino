#include "Timer.h"
#include <WiFi.h>
#include <PubSubClient.h> // install library PubSubClient
#include <Preferences.h>

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

typedef enum {
	MQTT_PUB_WATER_STATUS,
	MQTT_PUB_WATER_MODE,
	MQTT_PUB_MOIS_THRES,
	MQTT_PUB_WATER_DURATION,
	MQTT_PUB_WATER_TIMER_ENABLE,
	MQTT_PUB_WATER_TIMER_0,
	MQTT_PUB_WATER_TIMER_1,
	MQTT_PUB_WATER_TIMER_2,	
	MQTT_PUB_MAX,
}mqttPub_t;
byte MQTTPubData[MQTT_PUB_MAX];

typedef enum {
	MQTT_SUB_TEMP,
	MQTT_SUB_HUMI,
	MQTT_SUB_MOIS,
	MQTT_SUB_WATER_SET,
	MQTT_SUB_WATER_MODE_SET,
	MQTT_SUB_MOIS_THRES_SET,
	MQTT_SUB_WATER_DURATION_SET,
	MQTT_SUB_WATER_TIMER_ENABLE_SET,
	MQTT_SUB_WATER_TIMER_0_SET,
	MQTT_SUB_WATER_TIMER_1_SET,
	MQTT_SUB_WATER_TIMER_2_SET,
	MQTT_SUB_MAX,
}mqttSub_t;
byte MQTTSubData[MQTT_SUB_MAX];

char MQTTSubTopic[MQTT_SUB_MAX][32] = {
	"SCCHANG/temp", //from slave
	"SCCHANG/humi", //from slave
	"SCCHANG/mois", //from slave
	"SCCHANG/water", //from phone
	"SCCHANG/waterModeSet", //from phone
	"SCCHANG/moisThresSet", //from phone
	"SCCHANG/waterDurationSet", //from phone
	"SCCHANG/waterTimerEnSet", //from phone
	"SCCHANG/waterTimer0Set", //from phone
	"SCCHANG/waterTimer1Set", //from phone
	"SCCHANG/waterTimer2Set" //from phone
};
char MQTTPubTopic[MQTT_PUB_MAX][32] = {
	"SCCHANG/waterStatus", //to phone
	"SCCHANG/waterMode", //to phone
	"SCCHANG/moisThres", //to phone
	"SCCHANG/waterDuration", //to phone
	"SCCHANG/waterTimerEn", //to phone
	"SCCHANG/waterTimer0", //to phone
	"SCCHANG/waterTimer1", //to phone
	"SCCHANG/waterTimer2" //to phone
};

// ------ Network Clock Setting ------
char* ntpServer = "pool.ntp.org";
long  gmtOffset_sec = 28800;// GMT+8 : 8*3600=28800
int   daylightOffset_sec = 3600;

// ------ Preferences Setting ------
Preferences preferences; 
typedef struct prefData_s
{
	unsigned char MoisThres;
	unsigned char WaterMode, WaterDuration,WaterTimerEnable;
	unsigned char WaterTimer[3];
} prefData_t;
prefData_t prefData;

// ------ Timer Setting ------
#define TIME_INTERVAL_CHECK_WIFI_MQTT 60
#define TIME_INTERVAL_SEND_PUB 5
#define TIME_INTERVAL_CHECK_MOIS 2
Timer tChkWifiMQT, tSendPubData2MQTT;
Timer tCheckMois;

// ------ other ------
#define SERIAL_BAUD_RATE 115200
#define PIN_WATER 4
enum {
	WATER_SET_OFF,
	WATER_SET_ON,
	WATER_SET_MAX
};
enum {
	WATER_MODE_MANUAL,
	WATER_MODE_SENSOR,
	WATER_MODE_TIMER,
	WATER_MODE_MAX
};
enum {
	WIFI_STATUS_DISCONNECTED,
	WIFI_STATUS_CONNECTED,
	WIFI_STATUS_MAX,
};

//==============================================================
void InitWater(void)
{
	Serial.println("InitWater !!!");
	pinMode(PIN_WATER, OUTPUT);
	Water(WATER_SET_OFF);
}
void Water(boolean on)
{
	Serial.print("Water : ");
	Serial.println(on);
	digitalWrite(PIN_WATER, on);
	MQTTPubData[MQTT_PUB_WATER_STATUS]=on;
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
void CheckMoisture(void)
{
	char str[32];
	Serial.println("***** CheckMoisture *****");
	sprintf(str,"moi:%d%%, moisThres:%d%%", MQTTSubData[MQTT_SUB_MOIS], prefData.MoisThres);
	Serial.println(str);
	if(MQTTSubData[MQTT_SUB_MOIS]<=prefData.MoisThres){
		Water(WATER_SET_ON);
	}else{
		Water(WATER_SET_OFF);
	}
}
void OnTimerCheckMoisture(void)
{
	Serial.println("=========== OnTimerCheckMoisture ===========");
	if(WiFi.status()!= WL_CONNECTED){
		SetWifiLED(WIFI_STATUS_DISCONNECTED);
		Water(WATER_SET_OFF);
		Serial.println("Wifi is disconnected!");
		return;
	}	
	if(MQTTClient.connected()){
		if(prefData.WaterMode == WATER_MODE_SENSOR){
			Serial.println("water mode : sensor");
			CheckMoisture();
		}else{
			Serial.println("water mode : manual");
			if(MQTTSubData[MQTT_SUB_WATER_SET] == WATER_SET_OFF){
				Water(WATER_SET_OFF);
			}else{
				Water(WATER_SET_ON);
			}
		}
	}else{
		Serial.println("MQTT server is disconnected!");
		Water(WATER_SET_OFF);
	}
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
		// Init and get the time
		configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
		PrintLocalTime();
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
		for(int i=MQTT_PUB_WATER_STATUS; i<MQTT_PUB_MAX; i++){
			MQTTClient.publish(MQTTPubTopic[i], String(MQTTPubData[i]).c_str());
			sprintf(str,"%s: %d",MQTTPubTopic[i], MQTTPubData[i]);
			Serial.println(str);			
		}
		Serial.println("The publish-data have pulbished to MQTT Broker");
	}
}
void WifiConnecte(void)
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
void MQTTConnecte(void) {
	//Start to connect MQTT
	long StartTime = millis();
	Serial.println("Start to connect MQTT!");
	MQTTClient.setServer(MQTTServer, MQTTPort);
	MQTTClient.setCallback(MQTTCallback);
	while (!MQTTClient.connected()){
	//if(!MQTTClient.connected()){
		//random ClietID
		String  MQTTClientid = "esp32-" + String(random(1000000, 9999999));
		if(MQTTClient.connect(MQTTClientid.c_str(), MQTTUser, MQTTPassword)){
			Serial.println("MQTT is connnected!");
			//Subscribe Topic
			for(int i=MQTT_SUB_TEMP; i<MQTT_SUB_MAX; i++){
				MQTTClient.subscribe(MQTTSubTopic[i]);
			}
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
	Serial.println("***** MQTTCallback *****");
	//Serial.println(topic);
	String payloadString;//transfer the payload to string
	char str[64];

	for(int i = 0; i < length; i++){
		payloadString = payloadString + (char)payload[i];
	}
	//Serial.println(payloadString);
	for(int i = MQTT_SUB_TEMP; i < MQTT_SUB_MAX; i++){
		if(strcmp(topic, MQTTSubTopic[i]) == 0){
			MQTTSubData[i] = payloadString.toInt();
			sprintf(str,"%s: %d",MQTTSubTopic[i], MQTTSubData[i]);
			Serial.println(str);				
		}
	}	
	if(length>0){
		SetSubData2Pref();
		SetPref2PubData();
		WritePref();
	}
}
void PrintLocalTime(void){
	struct tm timeinfo;
	if(!getLocalTime(&timeinfo)){
		Serial.println("Failed to obtain time");
		return;
	}
	Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
	Serial.print("Day of week: ");
	Serial.println(&timeinfo, "%A");
	Serial.print("Month: ");
	Serial.println(&timeinfo, "%B");
	Serial.print("Day of Month: ");
	Serial.println(&timeinfo, "%d");
	Serial.print("Year: ");
	Serial.println(&timeinfo, "%Y");
	Serial.print("Hour: ");
	Serial.println(&timeinfo, "%H");
	Serial.print("Hour (12 hour format): ");
	Serial.println(&timeinfo, "%I");
	Serial.print("Minute: ");
	Serial.println(&timeinfo, "%M");
	Serial.print("Second: ");
	Serial.println(&timeinfo, "%S");
	#if 0
	Serial.println("TEST variables");
	char myHour[3];
	strftime(myHour,3, "%H", &timeinfo);
	Serial.println(myHour);
	String A = String(myHour);
	Serial.println(A);
	#endif
}
void ReadPref(void)
{
	//Serial.println("***** ReadPref *****");
	preferences.begin("water-master", false);// false : read/write mode
	prefData.WaterDuration= preferences.getUChar("WaterDuration", 0); // default : 0, continue
	prefData.WaterMode= preferences.getUChar("WaterMode", 0);
	prefData.MoisThres= preferences.getUChar("MoisThres", 0);
	prefData.WaterTimer[0]= preferences.getUChar("WaterTimer0", 0);
	prefData.WaterTimer[1]= preferences.getUChar("WaterTimer1", 0);
	prefData.WaterTimer[2]= preferences.getUChar("WaterTimer2", 0);
	prefData.WaterTimerEnable= preferences.getUChar("WaterTimerEnable", 0);
	preferences.end();
}
void WritePref(void)
{
	//Serial.println("***** WritePref *****");
	preferences.begin("water-master", false);// false : read/write mode
	preferences.putUChar("WaterDuration", prefData.WaterDuration);
	preferences.putUChar("WaterMode", prefData.WaterMode);
	preferences.putUChar("MoisThres", prefData.MoisThres);
	preferences.putUChar("WaterTimer0", prefData.WaterTimer[0]);
	preferences.putUChar("WaterTimer1", prefData.WaterTimer[1]);
	preferences.putUChar("WaterTimer2", prefData.WaterTimer[2]);
	preferences.putUChar("WaterTimerEnable", prefData.WaterTimerEnable);
	preferences.end();
}
void SetSubData2Pref(void)
{
	//Serial.println("***** SetSubData2Pref *****");
	prefData.WaterDuration= MQTTSubData[MQTT_SUB_WATER_DURATION_SET];
	prefData.WaterMode= MQTTSubData[MQTT_SUB_WATER_MODE_SET];
	prefData.MoisThres= MQTTSubData[MQTT_SUB_MOIS_THRES_SET];	
	prefData.WaterTimer[0]= MQTTSubData[MQTT_SUB_WATER_TIMER_0_SET];
	prefData.WaterTimer[1]= MQTTSubData[MQTT_SUB_WATER_TIMER_1_SET];
	prefData.WaterTimer[2]= MQTTSubData[MQTT_SUB_WATER_TIMER_2_SET];
	prefData.WaterTimerEnable= MQTTSubData[MQTT_SUB_WATER_TIMER_ENABLE_SET];
}
void SetPref2PubData(void)
{
	//Serial.println("***** SetPref2PubData *****");
	MQTTPubData[MQTT_PUB_WATER_DURATION] = prefData.WaterDuration;
	MQTTPubData[MQTT_PUB_WATER_MODE] = prefData.WaterMode;
	MQTTPubData[MQTT_PUB_MOIS_THRES] = prefData.MoisThres;	
	MQTTPubData[MQTT_PUB_WATER_TIMER_0] = prefData.WaterTimer[0];
	MQTTPubData[MQTT_PUB_WATER_TIMER_1] = prefData.WaterTimer[1];
	MQTTPubData[MQTT_PUB_WATER_TIMER_2] = prefData.WaterTimer[2];
	MQTTPubData[MQTT_PUB_WATER_TIMER_ENABLE] = prefData.WaterTimerEnable;
}
void setup(void)
{
	memset(&MQTTSubData,0,sizeof(MQTTSubData));
	memset(&MQTTPubData,0,sizeof(MQTTPubData));
	Serial.begin(SERIAL_BAUD_RATE);
	ReadPref();
	SetPref2PubData();
	InitWifiLED();
	InitWater();
	OnTimerChkWifiMQTTConnect();
	tChkWifiMQT.every(TIME_INTERVAL_CHECK_WIFI_MQTT *1000,OnTimerChkWifiMQTTConnect);
	tSendPubData2MQTT.every(TIME_INTERVAL_SEND_PUB *1000,OnTimerSendPubData2MQTT);
	tCheckMois.every(TIME_INTERVAL_CHECK_MOIS *1000,OnTimerCheckMoisture);
}
void loop(void)
{
	tChkWifiMQT.update();
	tSendPubData2MQTT.update();
	tCheckMois.update();
	MQTTClient.loop();// update the subscribe status
	delay(50);
}
