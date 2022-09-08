#include <iostream>
#include <Arduino.h>
#include <AsyncMqttClient.h>
#include <MQTT_topics.h>
#include <WiFi.h>
#include <Esp.h>
#include <time.h>
#include <esp_sntp.h>
#include <pinDefinition.h>
#include <EEPROM.h>
#include <structs.h>

enum statusONOFF {OFF, ON};

const char* WIFI_SSID = "SMA";
const char* WIFI_PASSWORD = "ipc2320207";
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 10800;
const int daylightOffset_sec = 0;
TimerHandle_t mqttReconnectTimer;
TimerHandle_t wifiReconnectTimer;
// prototypes
void connectToWifi();
void connectToMQTT();
void relay_control(int, statusONOFF);
void write_AppToFlash(programm*);  
void mqttflow_telemetrySend();
void mqttPublishDIO(const char*, int);
int mqttflow_getAppSettings();
//
tm LocalTime();
struct tm timeinfo;

using std::cout;
using std::cin;
using std::endl;

void setup()
{
    //General setup
    Serial.begin(9600);
    WiFi.mode(WIFI_STA);
    mqttReconnectTimer = xTimerCreate("mqttTimer", pdMS_TO_TICKS(2000), pdFALSE, (void*)0, reinterpret_cast<TimerCallbackFunction_t>(connectToMQTT));
    wifiReconnectTimer = xTimerCreate("wifiTimer", pdMS_TO_TICKS(2000), pdFALSE, (void*)0, reinterpret_cast<TimerCallbackFunction_t>(connectToWifi));
    //IO setup
    pinMode(PUMP_PIN, OUTPUT);
    pinMode(LIGHT_PIN, OUTPUT);
    pinMode(WATER_LEVEL_PIN, INPUT_PULLDOWN);
    pinMode(COMPRESSOR_PIN, OUTPUT);
    connectToWifi();
    connectToMQTT();
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    LocalTime();
    cout << "Start at: " << timeinfo.tm_hour << ":" << timeinfo.tm_min << endl;
    relay_control(PUMP_PIN, OFF);
    //programm setup
    //TODO: change structure to class object ////////////////////////////////////////////////////
    programm current_set;
    programm default_set 
    {
        .drip_start_h = 7,
        .drip_stop_h = 20,
        .lightON_h = 6,
        .lightOFF_h = 22,
        .pumpONtime_m = 15,
        .pumpOFFtime_m = 30
    };
    uint8_t app_exist = EEPROM.read(1); // check whether we have had set programm before
    if (app_exist == 0)
        {
            current_set = default_set;
            cout << "Default programm was setted\n";
            write_AppToFlash(&default_set);
        }
    write_AppToFlash(&current_set);     
    cout << "Watering start at: " << (int)current_set.drip_start_h << endl;    
    cout << "Watering stop at: " << (int)current_set.drip_stop_h << endl;
    cout << "Lighting ON at: " << (int)current_set.lightON_h << endl;
    cout << "Turn lighting OFF at: " << (int)current_set.lightOFF_h << endl;
    cout << "Turn pump every " << (int)current_set.pumpONtime_m << endl;
}

void loop()
{
    mqttflow_telemetrySend();
}

void connectToWifi()
{
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    std::cout << "Connecting to Wi-Fi\n";
    while (WiFi.status() != WL_CONNECTED)
    {
        Serial.print(".");
        delay(1000);
    }
    Serial.println("CONNECTED\n");
    Serial.print("ESP32 IP address: ");
    Serial.print(WiFi.localIP());
    std::cout << std::endl;
}

tm LocalTime()
{
    if (!getLocalTime(&timeinfo))
    {
        std::cout << "Failed to obtain a time\n";
    }
    return timeinfo;
}

void relay_control(int actuator, statusONOFF status)
{
    if (status == ON)
    {
        if (actuator == PUMP_PIN) // part of code for PUMP
        {
            if (WATER_LEVEL_PIN != 0)
                {
                    digitalWrite(actuator, HIGH);
                    Serial.println("Pump is ON");
                    mqttPublishDIO(topic_status_pump, PUMP_PIN);
                }
            else
                {
                    digitalWrite(actuator, LOW);
                    mqttPublishDIO(topic_status_pump, PUMP_PIN);
                    mqttPublishDIO(topic_status_waterlevel, WATER_LEVEL_PIN);
                    Serial.println("Water level is low, please add water to the boiler. Pump is OFF");
                }}
        else
            {
                digitalWrite(actuator, status);
                mqttflow_telemetrySend();
            }
    }
}

/*
EEPROM memory address map:
0 - 
1 - App exist
2 - Time (hour) when the watering must start
4 - Time (hour) when the watering must stop
6 - Time (hour) when the lighting must start
8 - Time (hour) when the lighting must stop
*/
void write_AppToFlash(programm* app)
{
    EEPROM.write(2, app->drip_start_h);
    EEPROM.write(4, app->drip_stop_h);
    EEPROM.write(6, app->lightON_h);
    EEPROM.write(8, app->lightOFF_h);
}