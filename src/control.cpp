#include <iostream>
#include <EEPROM.h>
#include <pinDefinition.h>
#include <Arduino.h>
#include <structs.h>
#include <DHTesp.h>
#include <MQTT_topics.h>
#include <time.h>
#include <string>

using std::cout;
using std::cin;
using std::endl;

hw_timer_s *pump_on_countdown = NULL;
hw_timer_s *idle_countdown = NULL;

enum statusONOFF {OFF, ON};
enum EEPROM_enum{start_hour = 2, stop_hour = 4, lightON_hour = 6, lightOFF_hour = 8};

// -- prototypes
int watering();
void mqttflow_telemetrySend();
void mqttPublishDIO(const char*, int);
void relay_control(int actuator, statusONOFF status);
void IRAM_ATTR wateringTimeout();
void IRAM_ATTR idleTimeout();
void timerSetup();
// --
extern tm timeinfo;
extern tm Localtime();
extern programm current_set;
// --
std::string mode = "initial";

void timerSetup()
{
    timerAttachInterrupt(pump_on_countdown, &wateringTimeout, false);
    timerAlarmWrite(pump_on_countdown, current_set.pumpONtime_m * 60000000, false);
    pump_on_countdown = timerBegin(0, 80, false);
    timerAttachInterrupt(idle_countdown, &idleTimeout, false);
    timerAlarmWrite(idle_countdown, current_set.pumpOFFtime_m * 60000000, false);
    idle_countdown = timerBegin(1, 80, false);
}

int watering()
{
    Localtime();
    if ((timeinfo.tm_hour >= current_set.drip_start_h) && (timeinfo.tm_hour <= current_set.drip_stop_h)) // check that the current time is in the work period 
    {
        if ((mode != "watering") && (mode != "idle")) // check the current mode
        {
            relay_control(PUMP_PIN, ON);
        }
    }
    return 0;
}

void IRAM_ATTR wateringTimeout()
{
    relay_control(PUMP_PIN, OFF);
    mode = "idle";
    timerAlarmEnable(idle_countdown);
}

void IRAM_ATTR idleTimeout()
{
    relay_control(PUMP_PIN, ON);
    mode = "watering";
    timerAlarmEnable(pump_on_countdown);
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
                    mode = "watering";
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