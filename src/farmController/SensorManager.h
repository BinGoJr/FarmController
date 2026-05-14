#pragma once
#include <Arduino.h>
#include <DHT.h>
#include <SensirionI2cScd4x.h>
#include <ModbusMaster.h>
#include "Config.h"

class SensorManager {
public:
    struct Data {
        float temp1, hum1, temp2, hum2;
        float pressureMPa;
        float flowRate;
        float totalVolume;
        int soilMoisture[NUM_CAP_SENSORS];
        float N, P, K, pH, soilMoistureModbus, soilTemp, EC;
        bool modbusValid;
        float scd40_co2, scd40_temp, scd40_hum;
    };

    void begin();               
    void powerOn();             
    void powerOff();            
    void readAll();             
    const Data& getData() const { return data; }
    float getAverageMoisture() const;
    void readDHT();
    void readPressure();
    void readFlow();
    void readSoilCapacitive();
    void readModbus();
    void readSCD40();
    static void flowISR();
    void enableFlowInterrupt();
    void disableFlowInterrupt();

    static SensorManager* instance;

private:
    Data data;

    DHT dht1{ Pins::dht1, DHT11 };
    DHT dht2{ Pins::dht2, DHT11 };
    SensirionI2cScd4x scd4x;
    ModbusMaster modbus;

    bool scd4xOk = false;

    volatile unsigned long flowPulseCount = 0;
    bool flowInterruptEnabled = false;
    unsigned long lastFlowRead = 0;

    
};