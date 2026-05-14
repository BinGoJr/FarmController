#include "SensorManager.h"
#include "Config.h"

SensorManager* SensorManager::instance = nullptr;

void SensorManager::begin() {
    instance = this;   

    pinMode(Pins::relayPower, OUTPUT);
    digitalWrite(Pins::relayPower, LOW);   
    pinMode(Pins::flowSensor, INPUT_PULLUP);

    // DHT
    dht1.begin();
    dht2.begin();

    // Modbus RTU
    Serial1.begin(9600);
    modbus.begin(1, Serial1);

    // SCD40 (CO2)
    Wire.begin();
    scd4x.begin(Wire, 0x62);
    scd4x.wakeUp();
    delay(30);
    scd4x.stopPeriodicMeasurement();
    scd4x.reinit();
    scd4x.startPeriodicMeasurement();
    scd4xOk = true;

    memset(&data, 0, sizeof(data));
    data.scd40_co2 = -999;
    data.scd40_temp = -999;
    data.scd40_hum = -999;
}


void SensorManager::powerOn() {
    digitalWrite(Pins::relayPower, HIGH);
    delay(SENSOR_STABILIZE_DELAY * 1000UL);   
    enableFlowInterrupt();
}

void SensorManager::powerOff() {
    digitalWrite(Pins::relayPower, LOW);
    disableFlowInterrupt();
}


void SensorManager::readAll() {
    readDHT();
    readPressure();
    readSoilCapacitive();
    readModbus();
    readSCD40();
    readFlow();   
}


void SensorManager::readDHT() {
    data.temp1 = dht1.readTemperature();
    data.hum1 = dht1.readHumidity();
    if (isnan(data.temp1) || isnan(data.hum1)) {
        data.temp1 = data.hum1 = -999;
    }
    data.temp2 = dht2.readTemperature();
    data.hum2 = dht2.readHumidity();
    if (isnan(data.temp2) || isnan(data.hum2)) {
        data.temp2 = data.hum2 = -999;
    }
}


void SensorManager::readPressure() {
    int raw = analogRead(Pins::pressure);
    float voltage = raw * VREF / 1023.0;
    data.pressureMPa = (voltage - 0.5) / 3.75;
}


void SensorManager::readSoilCapacitive() {
    for (int i = 0; i < NUM_CAP_SENSORS; i++) {
        int raw = analogRead(Pins::soilMoisture[i]);
        float voltage = raw * 3.3 / 1023.0;  
        float VWC = calibA[i] / voltage + calibB[i];
        if (VWC < 0.0) VWC = 0.0;
        if (VWC > 1.0) VWC = 1.0;
        data.soilMoisture[i] = (int)(VWC * 100.0 + 0.5);
    }
}

void SensorManager::readModbus() {
    struct Param {
        uint16_t addr;
        float* dest;
        float    (*conv)(uint16_t);
    };

    auto asIs = [](uint16_t v) -> float { return v; };
    auto div10 = [](uint16_t v) -> float { return v / 10.0f; };
    auto div100 = [](uint16_t v) -> float { return v / 100.0f; };

    Param params[] = {
        {0x001E, &data.N,                  asIs},    // N
        {0x001F, &data.P,                  asIs},    // P
        {0x0020, &data.K,                  asIs},    // K
        {0x0006, &data.pH,                 div100},  // pH
        {0x0012, &data.soilMoistureModbus, div10},   // Moisture
        {0x0013, &data.soilTemp,           div10},   // Temperature
        {0x0015, &data.EC,                 asIs}     // EC
    };
    const int numParams = sizeof(params) / sizeof(params[0]);

    bool anySuccess = false;
    for (int i = 0; i < numParams; i++) {
        uint8_t res = modbus.readHoldingRegisters(params[i].addr, 1);
        if (res == modbus.ku8MBSuccess) {
            *params[i].dest = params[i].conv(modbus.getResponseBuffer(0));
            anySuccess = true;
        }
        else {
            *params[i].dest = -999;
        }
        delay(50);
    }
    data.modbusValid = anySuccess;
}

void SensorManager::readSCD40() {
    if (!scd4xOk) return;

    bool dataReady = false;
    for (int i = 0; i < 50; i++) {
        if (scd4x.getDataReadyStatus(dataReady) == 0 && dataReady) break;
        delay(100);
    }
    if (!dataReady) {
        data.scd40_co2 = -999;
        data.scd40_temp = -999;
        data.scd40_hum = -999;
        return;
    }

    uint16_t co2 = 0;
    float temp = 0.0, hum = 0.0;
    if (scd4x.readMeasurement(co2, temp, hum) == 0) {
        data.scd40_co2 = co2;
        data.scd40_temp = temp;
        data.scd40_hum = hum;
    }
    else {
        data.scd40_co2 = -999;
        data.scd40_temp = -999;
        data.scd40_hum = -999;
    }
}


void SensorManager::enableFlowInterrupt() {
    if (!flowInterruptEnabled) {
        flowInterruptEnabled = true;
        attachInterrupt(digitalPinToInterrupt(Pins::flowSensor), flowISR, RISING);
    }
}

void SensorManager::disableFlowInterrupt() {
    if (flowInterruptEnabled) {
        flowInterruptEnabled = false;
        detachInterrupt(digitalPinToInterrupt(Pins::flowSensor));
    }
}

void SensorManager::flowISR() {
    if (instance) {
        instance->flowPulseCount++;
    }
}

void SensorManager::readFlow() {
    unsigned long now = millis();
    if (now - lastFlowRead >= 1000) {
        detachInterrupt(digitalPinToInterrupt(Pins::flowSensor));
        unsigned long pulses = flowPulseCount;
        flowPulseCount = 0;
        lastFlowRead = now;
        if (flowInterruptEnabled) {
            attachInterrupt(digitalPinToInterrupt(Pins::flowSensor), flowISR, RISING);
        }

        data.flowRate = (pulses * 60.0f) / PULSES_PER_LITER;   
        data.totalVolume += pulses / PULSES_PER_LITER;             
    }
}

float SensorManager::getAverageMoisture() const {
    float sum = 0;
    int count = 0;
    for (int i = 0; i < NUM_CAP_SENSORS; i++) {
        sum += data.soilMoisture[i];
        count++;
    }
    if (data.modbusValid) {
        sum += data.soilMoistureModbus;
        count++;
    }
    return sum / count;
}