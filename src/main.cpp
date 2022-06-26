#include <Arduino.h>
#include <iostream>
#include <AsyncMqttClient.h>
#include <WiFi.h>
#include <Esp.h>
#include <time.h>
#include <esp_sntp.h>
#include <pinDefinition.h>
#include <EEPROM.h>
#include <mqttflow>
#include <structs.h>
#define WIFI_SSID "SMA"
#define WIFI_PASSWORD "ipc2320207"

const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 10800;
const int daylightOffset_sec = 0;
TimerHandle_t mqttReconnectTimer;
TimerHandle_t wifiReconnectTimer;
// prototypes
void connectToWifi();
void connectToMQTT();
void turn_on(int actuator);
void turn_off(int actuator);
void write_AppToFlash(programm*);  
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
    std::cout << "Start at: ";
    LocalTime();
    turn_off(PUMP_PIN);
    //programm setup
    programm current_set;
    programm default_set 
    {
        .drip_start_h = 7,
        .drip_start_m = 0,
        .drip_stop_h = 20,
        .drip_stop_m = 0,
        .lightON_h = 6,
        .lightOFF_h = 22
    };
    uint8_t app_exist = EEPROM.read(1); // check whether we have had set programm before
    if (app_exist != 1)
        {
            current_set = default_set;
            cout << "Default programm was setted, please check the MQTT settings to apply the new programm\n";
            write_AppToFlash(&default_set);
        }
    cout << "Current programm:\n";
    if (!mqttflow_getAppSettings(&current_set))
        Serial.println("Unable to get settings from MQTT server");
    write_AppToFlash(&current_set);        
}

void loop()
{
    mqttflow_telemetrySend();
    delay(1000);
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

void connectToMQTT()
{
    mqttClient.setServer(MQTT_HOST, MQTT_PORT);
    std::cout << "Connecting to MQTT...";
    mqttClient.connect();
}

tm LocalTime()
{
    if (!getLocalTime(&timeinfo))
    {
        std::cout << "Failed to obtain a time\n";
    }
    return timeinfo;
}

void turn_on(int actuator)
{
    digitalWrite(actuator, HIGH);
}

void turn_off(int actuator)
{
    digitalWrite(actuator, LOW);
}

void write_AppToFlash(programm* app)
{
    EEPROM.write(2, app->drip_start_h);
    EEPROM.write(3, app->drip_start_m);
    EEPROM.write(4, app->drip_stop_h);
    EEPROM.write(5, app->drip_stop_m);
    EEPROM.write(6, app->lightON_h);
    EEPROM.write(7, app->lightOFF_h);
}  