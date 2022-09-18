#include <MQTT_topics.h>
#include <string>
#include <pinDefinition.h>
#include <structs.h>
#include <AsyncMqttClient.h>
#include <iostream>
#include <DHTesp.h>

using std::cout;
using std::cin;
using std::endl;
using std::string;
using std::to_string;

AsyncMqttClient mqttClient;
tm LocalTime();
extern tm timeinfo;
extern programm current_set;
extern TempAndHumidity environment;
extern DHTesp dht;
const IPAddress MQTT_HOST = IPAddress(91,121,93,94);
const int MQTT_PORT = 1883;

// functions prototypes
void mqttflow_telemetrySend();
void mqttPublishDIO(const char*, int);
int mqttflow_getAppSettings();
// --------------------

void mqttflow_telemetrySend()
{
    if (!mqttClient.connected())
        mqttClient.connect();
    timeinfo = LocalTime();
    string time_h = to_string(timeinfo.tm_hour); // use to_string and c_string to convert int to c type string
    string time_m = to_string(timeinfo.tm_min);
    string time;
    time = time_h + ':' + time_m;
    const char* msg = time.c_str();
    delay(500);
    environment = dht.getTempAndHumidity();
    string sTemp = to_string(environment.temperature);
    mqttClient.publish(topic_espTime, 0, 1, msg); // send time
    mqttPublishDIO(topic_status_pump, PUMP_PIN); // send pump status
    mqttPublishDIO(topic_status_light, LIGHT_PIN); 
    mqttPublishDIO(topic_status_compressor, COMPRESSOR_PIN); 
    mqttPublishDIO(topic_status_waterlevel, WATER_LEVEL_PIN);
    msg = sTemp.c_str();
    mqttClient.publish(topic_status_temperature, 0, 1, msg);
}

void mqttPublishDIO(const char* topic, int pin)
{
    string value = to_string(digitalRead(pin)); 
    const char* msg = value.c_str();
    mqttClient.publish(topic, 0, 1, msg);
}

void connectToMQTT()
{
    mqttClient.setServer(MQTT_HOST, MQTT_PORT);
    cout << "Connecting to MQTT" << endl;
    mqttClient.connect();
    while (mqttClient.connected() == 0)
    {
        cout << ".";
        delay(1000);
    }
    cout << "Connected to MQTT broker" << endl;
}

/*
int mqttflow_getAppSettings()
{
   mqttClient.onMessage(onMqttMessage("smadripsystem/settings/drip_start_h", ));
   return 0;
}
*/