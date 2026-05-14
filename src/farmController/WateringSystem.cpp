// WateringSystem.cpp
#include "WateringSystem.h"

WateringSystem::WateringSystem(SensorManager* sensors, DataLogger* logger)
    : sensors(sensors), logger(logger)
{
    lastMeasureTime = millis() - MEASURE_INTERVAL * 1000UL;
}

void WateringSystem::update(const DateTime& now) {
    unsigned long nowMs = millis();

    switch (state) {
    case IDLE:
        if (nowMs - lastMeasureTime >= MEASURE_INTERVAL * 1000UL) {
            sensors->powerOn();       
            sensors->readAll();            
            lastMeasureTime = nowMs;

            if (sensors->getAverageMoisture() < LOW_MOISTURE_THRESHOLD) {
                startWatering();
            }
            else {
                logger->log(sensors->getData(), now);
                sensors->powerOff();
            }
        }
        break;

    case WATERING:
        if (nowMs - lastWateringCheck >= WATERING_CHECK_INTERVAL * 1000UL) {
            lastWateringCheck = nowMs;
            sensors->readSoilCapacitive();
            sensors->readModbus();

            if (sensors->getAverageMoisture() >= HIGH_MOISTURE_THRESHOLD) {
                stopWatering(now);
                break;
            }
        }

        if (nowMs - wateringStartTime >= MAX_WATERING_TIME * 1000UL) {
            stopWatering(now);
            break;
        }

        sensors->readFlow();
        break;
    }
}

void WateringSystem::startWatering() {
    digitalWrite(Pins::relayPump, HIGH);   
    sensors->enableFlowInterrupt();        
    wateringStartTime = millis();
    lastWateringCheck = millis();
    state = WATERING;
    Serial.println("Watering started");
}

void WateringSystem::stopWatering(const DateTime& now) {
    digitalWrite(Pins::relayPump, LOW);    
    sensors->disableFlowInterrupt();       
    sensors->readAll();                    
    logger->log(sensors->getData(), now); 
    sensors->powerOff();                   
    state = IDLE;
    Serial.println("Watering stopped");
}