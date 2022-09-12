#include <iostream>
#include <Arduino.h>
#include <Adafruit_SSD1306.h>
#include <Esp.h>
#include <time.h>
#include <WiFi.h>
#include <DHTesp.h>

const int SCREEN_ADDRESS = 0x3C;

void setupDisplay();
void displayInitialInfo(TempAndHumidity &environment);

extern Adafruit_SSD1306 display;
extern std::string mode; 

void setupDisplay()
{
    if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) 
    {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
    }
    delay(500);
    display.clearDisplay();
    display.setTextColor(WHITE);
    display.setTextSize(1);
}

void displayInitialInfo(TempAndHumidity &environment)
{
    display.clearDisplay();
    display.setCursor(1, 2); display.println("Mode: ");
    display.setCursor(14, 2); display.println(mode.c_str());
    display.setCursor(1, 15); display.println("Current temp:");
    display.setCursor(80, 15); display.print(environment.temperature);
    display.display();
}