#include <Arduino.h>
#include <iostream>
#include <AsyncMqttClient.h>
#include <WiFi.h>
#include <Esp.h>

#define WIFI_SSID "SMA"
#define WIFI_PASSWORD "ipc2320207"
#define MQTT_HOST IPAddress(185,213,2,21) // IP address of MQTT broker
#define MQTT_PORT 1883
#define PUMP_PIN 32
#define LIGHT_PIN 33
#define WL1_PIN 34

AsyncMqttClient mqttClient;
void connectToWifi();
void connectToMQTT();

void setup()
{
    //General setup
    Serial.begin(9600);
    WiFi.mode(WIFI_STA);
    //IO setup
    pinMode(PUMP_PIN, OUTPUT);
    pinMode(LIGHT_PIN, OUTPUT);
    pinMode(WL1_PIN, INPUT_PULLDOWN);
    //MQTT part
    AsyncMqttClient mqttClient;
    TimerHandle_t mqttReconnectTimer;
    TimerHandle_t wifiReconnectTimer;
    connectToWifi();
}

void loop()
{
    using std::cout;
    using std::cin;
    using std::endl;

}

void connectToWifi()
{
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    std::cout << "Connecting to Wi-Fi...";
    while (WiFi.status() != WL_CONNECTED)
    {
        std::cout << ".";
        delay(1000);
    }
    std::cout << "ESP32 IP address: /n" << WiFi.localIP();
}

void connectToMQTT()
{
    std::cout << "Connecting to MQTT...";
    mqttClient.connect();
}
