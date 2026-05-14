#pragma once
#include <Arduino.h>
#include "Config.h"
#include "SensorManager.h"
#include "DataLogger.h"
#include <RTClib.h>

class WateringSystem {
public:
    WateringSystem(SensorManager* sensors, DataLogger* logger);

    void update(const DateTime& now);

private:
    SensorManager* sensors;
    DataLogger* logger;

    enum State { IDLE, WATERING };
    State state = IDLE;

    unsigned long lastMeasureTime = 0;
    unsigned long wateringStartTime = 0;
    unsigned long lastWateringCheck = 0;

    void startWatering();
    void stopWatering(const DateTime& now);
};