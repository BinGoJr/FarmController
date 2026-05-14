#pragma once
#include <Arduino.h>
#include <SD.h>
#include <RTClib.h>
#include "SensorManager.h"   

class DataLogger {
public:
    DataLogger(uint8_t csPin);

    bool begin();            
    void log(const SensorManager::Data& data, const DateTime& dt);  

private:
    uint8_t csPin;
    bool ready = false;      
};