#include <Wire.h>
#include <RTClib.h>
#include "Config.h"
#include "LightController.h"
#include "LightScheduler.h"
#include "PhotoManager.h"
#include "SensorManager.h"
#include "DataLogger.h"
#include "WateringSystem.h"

RTC_DS3231 rtc;
LightController leds;
LightScheduler lightSched(&leds);
PhotoManager photo(&leds);
SensorManager sensors;
DataLogger logger(Pins::sdCS);
WateringSystem watering(&sensors, &logger);

DateTime experimentStart(2026, 4, 4, 0, 0, 0);

int getExperimentDay() {
    TimeSpan diff = rtc.now() - experimentStart;
    return diff.days() + 1;
}

void setup() {
    Serial.begin(57600);
    Wire.begin();
    analogReference(EXTERNAL);
    // RTC
    if (!rtc.begin()) {
        Serial.println("RTC not found!");
    } else if (rtc.lostPower()) {
        rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    }

    // Инициализация модулей
    leds.begin();
    sensors.begin();
    logger.begin();

    pinMode(Pins::relayPump, OUTPUT);
    digitalWrite(Pins::relayPump, LOW);

    Serial.println("Farm Controller ready.");
    Serial.print("Experiment day: ");
    Serial.println(getExperimentDay());
}

void loop() {
    DateTime now = rtc.now();
    int day = getExperimentDay();

    // Смена суток
    static int lastDay = 0;
    if (day != lastDay) {
        photo.resetDaily();
        lastDay = day;
    }

    // Обновление освещения и фото
    lightSched.update(now, day);
    photo.update(now, day);

    // Полив
    watering.update(now);

    delay(10);
}