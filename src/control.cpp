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
int showWateringCountdown();
int showIdleCountdown();
void lightControl();
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
    
    idle_countdown = timerBegin(1, 80, true);
    timerAttachInterrupt(idle_countdown, &idleTimeout, false);
    timerAlarmWrite(idle_countdown, current_set.pumpOFFtime_m * 60000000, true);
    
    timerAlarmEnable(idle_countdown);
    timerAlarmEnable(watering_countdown);
    
    timerStop(watering_countdown);
    timerStop(idle_countdown);   
    
    timerRestart(watering_countdown);
    timerRestart(idle_countdown);
}

int watering()
{
    if ((timeinfo.tm_hour >= current_set.drip_start_h) && (timeinfo.tm_hour <= current_set.drip_stop_h)) // check that the current time is in the work period 
    {
        if (mode != "watering" && mode != "idle")
        {
            relay_control(PUMP_PIN, ON);
        }
    }
    //cout << "\nWatering timeout: " << timerReadSeconds(watering_countdown) << endl;
    //cout << "Idle timeout: " << timerReadSeconds(idle_countdown) << endl;
    return 0;
}

void IRAM_ATTR wateringTimeout()
{
    timerStop(watering_countdown);
    timerRestart(watering_countdown);
    relay_control(PUMP_PIN, OFF);
    mode = "idle";
    timerStart(idle_countdown);
}

void IRAM_ATTR idleTimeout()
{
    timerStop(idle_countdown);
    timerRestart(idle_countdown);
    mode = "Idle timeout";
}

void relay_control(int actuator, statusONOFF status)
{
    if (status == ON)
    {
        if (actuator == PUMP_PIN) // part of code for PUMP
        {
            if (digitalRead(WATER_LEVEL_PIN) != 0) // check the water level
                {
                    digitalWrite(PUMP_PIN, HIGH);
                    cout << "Pump is ON\n";
                    mqttPublishDIO(topic_status_pump, PUMP_PIN);
                    mode = "watering";
                    timerStart(watering_countdown);
                }
            else
                {
                    digitalWrite(PUMP_PIN, LOW);
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
    else 
    {
        digitalWrite(actuator, LOW);
    }
}

void lightControl()
{
    if ((timeinfo.tm_hour >= current_set.lightON_h) && (digitalRead(LIGHT_PIN) != ON))
        {
            relay_control(LIGHT_PIN, ON);
        }
    else if ((timeinfo.tm_hour > current_set.lightOFF_h) && (digitalRead(LIGHT_PIN) == ON))
        {
            relay_control(LIGHT_PIN, OFF);
        }
}

int showWateringCountdown()
{
    return (int)timerReadSeconds(watering_countdown);
}

int showIdleCountdown()
{
    return (int)timerReadSeconds(idle_countdown);
}