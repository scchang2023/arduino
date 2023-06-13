#include <SimpleDHT.h>
#include <LiquidCrystal_I2C.h>
#include "Timer.h"
#include <BluetoothSerial.h>

#define SERIAL_BAUD_RATE 115200
#define PIN_DHT11_S 4
#define PIN_FC28_AO 36
#define PIN_VALVE 2
#define LCD_I2C_ADDR 0x27 //SDA = GPIO 21, SCL = GPIO 22
#define SHOW_SENSOR_INFO_TIME 2 // second
#define CHECK_MOI_TIME 10 // second
#define VALVE_ON_TIME 5  // second

#define VALVE_ON_MOI_THRESHOLD 30 //petcent
#define MOI_MAX_AD 0xFFF

#define LCD_TEMP_STR_X 0
#define LCD_TEMP_STR_Y 0
#define LCD_HUM_STR_X 0
#define LCD_HUM_STR_Y 1
#define LCD_MOI_STR_X 11
#define LCD_MOI_STR_Y 0
#define LCD_MOI_NUM_X 11
#define LCD_MOI_NUM_Y 1


SimpleDHT11 dht11(PIN_DHT11_S);
LiquidCrystal_I2C lcd(LCD_I2C_ADDR, 16, 2);
Timer tShowSensorInfo, tValve, tCheckMois;
BluetoothSerial BT;
byte SensorData[4];

int GetDht11TempHum(byte *temp, byte *hum)
{
	*temp=0;
	*hum=0;
	return dht11.read(temp, hum, NULL);
}
void ShowDht11Temp(byte val)
{
	char str[32];
	sprintf(str,"Temp:%dC", val);
	Serial.println(str);
	//BT.println(str);
	lcd.setCursor(LCD_TEMP_STR_X, LCD_TEMP_STR_Y);
	lcd.print(str);
}
void ShowDht11Hum(byte val)
{
	char str[32];
	sprintf(str,"Hum:%d%%", val);
	Serial.println(str);
	//BT.println(str);
	lcd.setCursor(LCD_HUM_STR_X, LCD_HUM_STR_Y);
	lcd.print(str);
}
void ShowDht11Err(int err)
{
	char str[32];
	sprintf(str,"Dht11 Err = %d", err);
	Serial.println(str);
	//BT.println(str);
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
void InitValve(void)
{
	Serial.println("InitValve !!!");
	pinMode(PIN_VALVE, OUTPUT);
	digitalWrite(PIN_VALVE, LOW);
}
void TurnOnValve(boolean on)
{
	Serial.print("Turn on Valve:");
	Serial.println(on);
	digitalWrite(PIN_VALVE, on);
}
void TurnOnValveTime(int time)
{
	static int EvntValve=NULL;
	Serial.println("TurnOnValveTime");
	if(EvntValve!=NULL){
		Serial.println("stop EventValve");
		tValve.stop(EvntValve);
		EvntValve=NULL;
	}
	Serial.print("Turn on Valve and last ");
	Serial.print(time);
	Serial.println(" second");
	if(time == NULL)
		digitalWrite(PIN_VALVE, HIGH);
	else
		EvntValve = tValve.pulseImmediate(PIN_VALVE, time * 1000, HIGH);
}
int NormalizedFc28Moisture(int moi)
{
	return 100-(moi*100/MOI_MAX_AD);
}
void ShowFc28Moisture(int moi)
{
	int moiPercent=0;
	char str[32];
	moiPercent = NormalizedFc28Moisture(moi);
	sprintf(str,"Moi: %d, %d%%", moi, moiPercent);
	Serial.println(str);
	//BT.println(str);	
	lcd.setCursor(LCD_MOI_STR_X, LCD_MOI_STR_Y);
	lcd.print("Moi:");
	lcd.setCursor(LCD_MOI_NUM_X, LCD_MOI_NUM_Y);
	lcd.print(moiPercent);
	lcd.print("%");	
}
void CheckFc28Moisture(int moi)
{
	int moiPercent=0;
	moiPercent = NormalizedFc28Moisture(moi);
	Serial.println("CheckFc28Moisture !!!");
	Serial.print("Moi-ad: ");
	Serial.println(moi);
	Serial.print("Moi-percent: ");
	Serial.println(moiPercent);		
	if(moiPercent<=VALVE_ON_MOI_THRESHOLD){
		TurnOnValveTime(VALVE_ON_TIME);
	}else{
		TurnOnValve(false);
	}
}
void SendSensorInfo2BT(void)
{
	char str[32];
	Serial.println("=========== SendSensorInfo2BT ===========");
	sprintf(str,"%d,%d,%d,%d",SensorData[0], SensorData[1],
		SensorData[2], SensorData[3]);
	Serial.println(str);
	BT.write(&SensorData[0], sizeof(SensorData));
}

void OnTimerShowSensorInfo(void)
{
	byte temp, hum;
	int err, moi;
	Serial.println("=========== OnTimerShowSensorInfo ===========");
	lcd.clear();
	err=GetDht11TempHum(&temp, &hum);
	if(err==SimpleDHTErrSuccess){
		ShowDht11Temp(temp);
		ShowDht11Hum(hum);
		SensorData[0]=temp;
		SensorData[1]=hum;
	}else{
		ShowDht11Err(err);
	}
	moi=GetFc28Moisture();
	ShowFc28Moisture(moi);
	SensorData[2]=(byte)NormalizedFc28Moisture(moi);
	SensorData[3]=VALVE_ON_MOI_THRESHOLD;
	SendSensorInfo2BT();
}
void OnTimerCheckMoisture(void)
{
	Serial.println("=========== OnTimerCheckMoisture ===========");
	CheckFc28Moisture(GetFc28Moisture());
}

void setup()
{
	memset(&SensorData,0,sizeof(SensorData));
	Serial.begin(SERIAL_BAUD_RATE);
	lcd.init();
	lcd.backlight();
	//lcd.clear();
	InitValve();
	InitFc28Moisture();
	BT.begin("SCCHANG");
	OnTimerShowSensorInfo();
	OnTimerCheckMoisture();
	tShowSensorInfo.every(SHOW_SENSOR_INFO_TIME *1000,OnTimerShowSensorInfo);
	tCheckMois.every(CHECK_MOI_TIME *1000,OnTimerCheckMoisture);
}
void loop()
{
	tShowSensorInfo.update();
	tCheckMois.update();
	tValve.update();
}
