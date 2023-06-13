#include "Timer.h"
#include <WiFi.h>
#include <PubSubClient.h> // install library PubSubClient
#include <Preferences.h>

// DEVICE ID
#define DEVICE_ID 1
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
	MQTT_PUB_WATER_IS_ON,
	MQTT_PUB_DURATION_COUNT,
	MQTT_PUB_MAX,
}mqttPub_t;
int MQTTPubData[MQTT_PUB_MAX];

typedef enum {
	MQTT_SUB_DURATION_SETTING,
	MQTT_SUB_WATER_ON_SET,
	MQTT_SUB_MAX,
}mqttSub_t;

struct MqttSubData{
  boolean on;
  int delay;
  int duration;
};
struct MqttSubData myWaterSub;
int delayCnt=0, durationCnt=0;

char MQTTSubTopic[MQTT_SUB_MAX][32] = {
	"SCCHANG/durationSet",
	"SCCHANG/waterOnSet"
};
char MQTTPubTopic[MQTT_PUB_MAX][32] = {
	"SCCHANG/waterIsOn",
	"SCCHANG/durationCount",
};

// ------ Network Clock Setting ------
char* ntpServer = "pool.ntp.org";
long  gmtOffset_sec = 28800;// GMT+8 : 8*3600=28800
int   daylightOffset_sec = 3600;

// ------ Preferences Setting ------
Preferences preferences; 
typedef struct prefData_s
{
	unsigned char durationSetting;
	
} prefData_t;
prefData_t prefData;

// ------ Timer Setting ------
#define TIME_INTERVAL_CHECK_WIFI_MQTT 60
#define TIME_INTERVAL_SEND_PUB 1

Timer tChkWifiMQT, tSendPubData2MQTT, tWaterDuration;
int8_t eventIDWaterDuration=-1;


// ------ other ------
#define SERIAL_BAUD_RATE 115200
#define PIN_WATER_AREA_0 4
#define PIN_WATER_AREA_1 0
#define PIN_WATER_AREA_2 2
#define PIN_WATER_AREA_3 15
enum {
	WATER_SET_OFF,
	WATER_SET_ON,
	WATER_SET_MAX
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
	MQTTPubData[MQTT_PUB_WATER_IS_ON]=on;
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
void WifiMQTTConnect(void)
{
	Serial.println("=========== WifiMQTTConnect ===========");
	//Restart to connect WIFI if WIFI is disconnected
	if(WiFi.status()!= WL_CONNECTED){
		SetWifiLED(WIFI_STATUS_DISCONNECTED);
		WifiConnecte();
	}
	SetWifiLED(WIFI_STATUS_CONNECTED);
	if(!MQTTClient.connected()){
		MQTTConnecte();
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
		for(int i=MQTT_PUB_WATER_IS_ON; i<MQTT_PUB_MAX; i++){
   			String str2 = String(DEVICE_ID) + "," + String(MQTTPubData[i]);
    		const char* c_str = str2.c_str();			
			MQTTClient.publish(MQTTPubTopic[i], c_str);
			sprintf(str,"%s : %s",MQTTPubTopic[i], str2);
			Serial.println(str);			
		}
		Serial.println("The publish-data have pulbished to MQTT Broker");
	}
}
void OnTimerWaterDuration(void)
{
	Serial.println("=========== OnTimerWaterDuration ===========");
	MQTTPubData[MQTT_PUB_DURATION_COUNT]=durationCnt;
	if(delayCnt >= myWaterSub.delay){
		Water(true);
		if(myWaterSub.duration ==0){
			tWaterDuration.stop(eventIDWaterDuration);
		}else{
			if(durationCnt>= myWaterSub.duration){
				Water(false);
				tWaterDuration.stop(eventIDWaterDuration);
			}else{
				durationCnt++;
			}
		}
	}else{
		delayCnt++;
	}
}
void WifiConnecte(void)
{
	//Start to connect WIFI
	Serial.println("Start to connect WIFI!");
	WiFi.begin(ssid, password);
	while(WiFi.status()!=WL_CONNECTED){
		delay(500);
		Serial.print(".");
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
		//random ClietID
		String  MQTTClientid = "esp32-" + String(random(1000000, 9999999));
		if(MQTTClient.connect(MQTTClientid.c_str(), MQTTUser, MQTTPassword)){
			Serial.println("MQTT is connnected!");
			//Subscribe Topic
			for(int i=MQTT_SUB_DURATION_SETTING; i<MQTT_SUB_MAX; i++){
				MQTTClient.subscribe(MQTTSubTopic[i]);
			}
		}else{
			Serial.print("Failed to connect MQTT, Err = ");
			Serial.println(MQTTClient.state());
			delay(5000);
		}
	}
}


void MQTTCallback(char* topic, byte* payload, unsigned int length)
{
	Serial.println("***** MQTTCallback *****");
	String payloadString;//transfer the payload to string
	char str[64];
	
	for(int i = 0; i < length; i++){
		payloadString = payloadString + (char)payload[i];
	}
	Serial.printf("topic %s, payload %s", topic, payloadString);
	String token = "";
	int tokenIndex=0;
	int val1=-1, val2=0, val3=0, val4=0;
	for(int i = 0; i < payloadString.length(); i++){
		if(payloadString.charAt(i) == ','){
			if(tokenIndex == 0){
				val1 = token.toInt();
			}else if(tokenIndex == 1){
				val2 = token.toInt();
			}else if (tokenIndex == 2){
				val3 = token.toInt();
			}else if (tokenIndex == 3){
				val4 = token.toInt();
			}
			token = "";
			tokenIndex++;
		}else{
			token += payloadString.charAt(i);
		}
	}
	if (tokenIndex == 3) {
		val4 = token.toInt();
	}
	if(val1!=DEVICE_ID){
		Serial.printf("id %d is not equal DEVICE_ID %d\n", val1, DEVICE_ID);
		return;
	}	
	if(strcmp(topic, MQTTSubTopic[MQTT_SUB_DURATION_SETTING]) == 0){
		myWaterSub.duration = val2;
	}else if(strcmp(topic, MQTTSubTopic[MQTT_SUB_WATER_ON_SET]) == 0){
		myWaterSub.on = val2;
		myWaterSub.delay = val3;
		myWaterSub.duration= val4;
		tWaterDuration.stop(eventIDWaterDuration);
		Water(false);
		delayCnt=0;
		durationCnt=0;		
		if(myWaterSub.on == true){
			eventIDWaterDuration = tWaterDuration.every(1000,OnTimerWaterDuration);
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
	preferences.begin("smart-water", false);// false : read/write mode
	prefData.durationSetting= preferences.getUChar("durationSetting", 0); // default : 0, continue
	preferences.end();
}
void WritePref(void)
{
	//Serial.println("***** WritePref *****");
	preferences.begin("smart-water", false);// false : read/write mode
	preferences.putUChar("durationSetting", prefData.durationSetting);
	preferences.end();
}
void SetSubData2Pref(void)
{
	//Serial.println("***** SetSubData2Pref *****");

}
void SetPref2PubData(void)
{
	//Serial.println("***** SetPref2PubData *****");
	MQTTPubData[MQTT_PUB_DURATION_COUNT] = prefData.durationSetting;
}
void setup(void)
{
	memset(&myWaterSub,0,sizeof(myWaterSub));
	memset(&MQTTPubData,0,sizeof(MQTTPubData));
	Serial.begin(SERIAL_BAUD_RATE);
	ReadPref();
	SetPref2PubData();
	InitWifiLED();
	InitWater();
	// Init and get the time
	configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
	PrintLocalTime();	
	WifiMQTTConnect();
	tSendPubData2MQTT.every(TIME_INTERVAL_SEND_PUB *1000,OnTimerSendPubData2MQTT);
}
void loop(void)
{
	WifiMQTTConnect();
	tSendPubData2MQTT.update();
	tWaterDuration.update();
	MQTTClient.loop();// update the subscribe status
	delay(10);
}
