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
#include <DHTesp.h>
#include <Adafruit_SSD1306.h>
#include <SPI.h>
#include <WiFi.h>

DHTesp dht;
enum statusONOFF {OFF, ON};
enum EEPROM_enum{start_hour = 2, stop_hour = 4, lightON_hour = 6, lightOFF_hour = 8};
// const char* WIFI_SSID = "SMA";
// const char* WIFI_PASSWORD = "ipc2320207";
const char* WIFI_SSID = "MiKate2";
const char* WIFI_PASSWORD = "yaslujukate";
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 10800;
const int daylightOffset_sec = 0;
TempAndHumidity environment;
TimerHandle_t mqttReconnectTimer;
TimerHandle_t wifiReconnectTimer;
// -- prototypes
void connectToWifi();
void connectToMQTT();
void relay_control(int, statusONOFF);
void write_AppToFlash(programm*);  
void mqttflow_telemetrySend();
void mqttPublishDIO(const char*, int);
void setupDisplay();
int mqttflow_getAppSettings();
void displayInitialInfo(TempAndHumidity &environment);
int watering();
void timerSetup();
void lightControl();
//
programm current_set;
programm default_set 
    {
        .drip_start_h = 7,
        .drip_stop_h = 23,
        .lightON_h = 6,
        .lightOFF_h = 22,
        .pumpONtime_m = 15,
        .pumpOFFtime_m = 30
    };
Adafruit_SSD1306 display(128, 32, &Wire, -1);
struct tm timeinfo;

using std::cout;
using std::cin;
using std::endl;

tm LocalTime()
{
    if (!getLocalTime(&timeinfo))
    {
        cout << "Failed to obtain a time\n";
    }
    return timeinfo;
}

void setup()
{
    // -- General setup
    Serial.begin(9600);
    WiFi.mode(WIFI_STA);
    mqttReconnectTimer = xTimerCreate("mqttTimer", pdMS_TO_TICKS(2000), pdFALSE, (void*)0, reinterpret_cast<TimerCallbackFunction_t>(connectToMQTT));
    wifiReconnectTimer = xTimerCreate("wifiTimer", pdMS_TO_TICKS(2000), pdFALSE, (void*)1, reinterpret_cast<TimerCallbackFunction_t>(connectToWifi));
    // -- IO setup
    pinMode(PUMP_PIN, OUTPUT);
    pinMode(LIGHT_PIN, OUTPUT);
    pinMode(WATER_LEVEL_PIN, INPUT_PULLUP);
    pinMode(COMPRESSOR_PIN, OUTPUT);
    dht.setup(TEMP_PIN, DHTesp::DHT22);
    // -- connection
    connectToWifi();
    connectToMQTT();
    // -- time setup
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    LocalTime();
    cout << "Start at: " << timeinfo.tm_hour << ":" << timeinfo.tm_min << endl;
    relay_control(PUMP_PIN, OFF);
    // -- programm setup
    // TODO: change structure to class object ////////////////////////////////////////////////////
    uint8_t app_exist = EEPROM.read(1); // check whether we have had set programm before
    if (app_exist == 0)
        {
            current_set = default_set;
            cout << "Default programm was setted\n";
            write_AppToFlash(&default_set);
        }
    write_AppToFlash(&current_set);    
    setupDisplay();
    cout << "Watering start at: " << (int)current_set.drip_start_h << endl;    
    cout << "Watering stop at: " << (int)current_set.drip_stop_h << endl;
    cout << "Lighting ON at: " << (int)current_set.lightON_h << endl;
    cout << "Turn lighting OFF at: " << (int)current_set.lightOFF_h << endl;
    cout << "Turn pump every " << (int)current_set.pumpONtime_m << endl << endl;
    environment = dht.getTempAndHumidity();
    cout << "Temperature: " << environment.temperature << endl
         << "Humidity: " << environment.humidity << endl << endl;
    timerSetup();
}

void loop()
{
    LocalTime();
    mqttflow_telemetrySend();
    watering();
    lightControl();
    displayInitialInfo(environment);
}

void connectToWifi()
{
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.println("Connecting to Wi-Fi");
    while (WiFi.status() != WL_CONNECTED)
    {
        Serial.print(".");
        delay(1000);
    }
    cout << "CONNECTED\n"
         << "ESP32 IP address: " << endl;
    Serial.println(WiFi.localIP());
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
    EEPROM.write(start_hour, app->drip_start_h);
    EEPROM.write(stop_hour, app->drip_stop_h);
    EEPROM.write(lightON_hour, app->lightON_h);
    EEPROM.write(lightOFF_hour, app->lightOFF_h);
}