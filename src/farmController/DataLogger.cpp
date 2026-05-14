#include "DataLogger.h"
#include "Config.h"

DataLogger::DataLogger(uint8_t csPin) : csPin(csPin) {}

bool DataLogger::begin() {
    if (!SD.begin(csPin)) {
        Serial.println("SD card init failed");
        return false;
    }

    if (!SD.exists("datalog.csv")) {
        File f = SD.open("datalog.csv", FILE_WRITE);
        if (f) {
            f.println("DateTime,Temp1,Hum1,Temp2,Hum2,PressureMPa,FlowRate,TotalVolume,"
                "Soil1,Soil2,Soil3,Soil4,Soil5,"
                "N,P,K,pH,SoilMoistureModbus,SoilTemp,EC,"
                "CO2,SCD40_Temp,SCD40_Hum");
            f.close();
            Serial.println("datalog.csv created with header");
        }
        else {
            Serial.println("Can't create datalog.csv");
            return false;
        }
    }

    ready = true;
    return true;
}

void DataLogger::log(const SensorManager::Data& data, const DateTime& dt) {
    if (!ready) {
        if (!begin()) return;
    }

    File f = SD.open("datalog.csv", FILE_WRITE);
    if (!f) {
        Serial.println("Can't open datalog.csv for writing");
        ready = false;
        return;
    }

    char buffer[20];
    sprintf(buffer, "%04d-%02d-%02d %02d:%02d:%02d",
        dt.year(), dt.month(), dt.day(),
        dt.hour(), dt.minute(), dt.second());

    String line = String(buffer);
    line += "," + String(data.temp1) + "," + String(data.hum1);
    line += "," + String(data.temp2) + "," + String(data.hum2);
    line += "," + String(data.pressureMPa);
    line += "," + String(data.flowRate) + "," + String(data.totalVolume);

    for (int i = 0; i < NUM_CAP_SENSORS; i++) {
        line += "," + String(data.soilMoisture[i]);
    }

    if (data.modbusValid) {
        line += "," + String(data.N) + "," + String(data.P) + "," + String(data.K);
        line += "," + String(data.pH) + "," + String(data.soilMoistureModbus);
        line += "," + String(data.soilTemp) + "," + String(data.EC);
    }
    else {
        line += ",-999,-999,-999,-999,-999,-999,-999";
    }

    line += "," + String(data.scd40_co2);
    line += "," + String(data.scd40_temp);
    line += "," + String(data.scd40_hum);

    f.println(line);
    f.close();

    Serial.print("DATA:");
    Serial.println(line);
}