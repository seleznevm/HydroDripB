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

hw_timer_t *watering_countdown = NULL;
hw_timer_t *idle_countdown = NULL;

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
extern tm LocalTime();
extern programm current_set;
// --
std::string mode = "initial";

void timerSetup()
{
    watering_countdown = timerBegin(0, 80, true);
    timerAttachInterrupt(watering_countdown, &wateringTimeout, false);
    timerAlarmWrite(watering_countdown, current_set.pumpONtime_m * 60000000, true);
    idle_countdown = timerBegin(2, 80, true);
    timerAttachInterrupt(idle_countdown, &idleTimeout, true);
    timerAlarmWrite(idle_countdown, current_set.pumpOFFtime_m * 60000000, true);
    timerStop(watering_countdown);
    timerStop(idle_countdown);   
}

int watering()
{
    LocalTime();
    if ((timeinfo.tm_hour >= current_set.drip_start_h) && (timeinfo.tm_hour <= current_set.drip_stop_h)) // check that the current time is in the work period 
    {
        if ((mode != "watering") && (mode != "idle")) // check the current mode
        {
            cout << "\n If mode not watering or idle\n";
            relay_control(PUMP_PIN, ON);
        }
    }
    cout << "\nWatering timeout: " << timerReadSeconds(watering_countdown) << endl;
    cout << "\nIdle timeout: " << timerReadSeconds(idle_countdown) << endl;
    if (mode == "idle") 
    timerAlarmEnable(idle_countdown);
    return 0;
}

void IRAM_ATTR wateringTimeout()
{
    relay_control(PUMP_PIN, OFF);
    mode = "idle";
}

void IRAM_ATTR idleTimeout()
{
    relay_control(PUMP_PIN, ON);
    mode = "watering";
}

void relay_control(int actuator, statusONOFF status)
{
    if (status == ON)
    {
        if (actuator == PUMP_PIN) // part of code for PUMP
        {
            if (digitalRead(WATER_LEVEL_PIN) != 0)
                {
                    digitalWrite(actuator, HIGH);
                    cout << "Pump is ON\n";
                    mqttPublishDIO(topic_status_pump, PUMP_PIN);
                    mode = "watering";
                    timerAlarmEnable(watering_countdown);
                }
            else
                {
                    digitalWrite(actuator, LOW);
                    mqttPublishDIO(topic_status_pump, PUMP_PIN);
                    mqttPublishDIO(topic_status_waterlevel, WATER_LEVEL_PIN);
                    cout << "Water level is low, please add water to the boiler. Pump is OFF\n";
                }
        }
        else
            {
                digitalWrite(actuator, status);
                mqttflow_telemetrySend();
            }
    }
}