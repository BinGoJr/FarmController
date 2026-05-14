#pragma once
#include <Arduino.h>

// ========== ПИНЫ ==========
namespace Pins {
    // Питание
    const int relayPower = 11;

    // MOSFET (ШИМ)
    const uint8_t mosfetSec1Ch0 = 6;   
    const uint8_t mosfetSec1Ch1 = 5;
    const uint8_t mosfetSec1Ch2 = 1;
    const uint8_t mosfetSec2Ch0 = 7;
    const uint8_t mosfetSec2Ch1 = 4;
    const uint8_t mosfetSec2Ch2 = 0; //не подключен

    // Реле секций
    const uint8_t relaySec1Ch0 = 39;
    const uint8_t relaySec1Ch1 = 41;
    const uint8_t relaySec1Ch2 = 45;
    const uint8_t relaySec2Ch0 = 38;
    const uint8_t relaySec2Ch1 = 40;
    const uint8_t relaySec2Ch2 = 46;//не подключен

    // Камера и вспышка
    const uint8_t relayCamera = 9;
    const uint8_t relayLamp1 = 12;
    const uint8_t relayLamp2 = 25;

    // Датчики
    const uint8_t dht1 = 2;
    const uint8_t dht2 = 3;
    const uint8_t flowSensor = 8;
    const uint8_t pressure = A5;
    const uint8_t soilMoisture[5] = { A0, A1, A2, A3, A4 };
    const uint8_t relayPump = 10;

    // SD карта
    const uint8_t sdCS = 53;

    // RS485
    const uint8_t rs485Tx = 18;
    const uint8_t rs485Rx = 19;
}

// ========== НАСТРОЙКИ ДАТЧИКОВ ==========
constexpr float PULSES_PER_LITER = 553.0f;
constexpr float VREF = 5.0f;
constexpr uint8_t NUM_CAP_SENSORS = 5;
constexpr float calibA[NUM_CAP_SENSORS] = { 0.6682, 1.2399, 1.2762, 1.3174, 0.9967 };
constexpr float calibB[NUM_CAP_SENSORS] = { -0.1476, -0.4017, -0.3726, -0.4650, -0.4427 };

// ========== НАСТРОЙКИ ПОЛИВА ==========
constexpr uint8_t  LOW_MOISTURE_THRESHOLD = 60;   // %
constexpr uint8_t  HIGH_MOISTURE_THRESHOLD = 80;   // %
constexpr uint32_t MEASURE_INTERVAL = 10800; // секунд
constexpr uint8_t  WATERING_CHECK_INTERVAL = 10;    // секунд
constexpr uint16_t MAX_WATERING_TIME = 600;   // секунд
constexpr uint8_t  SENSOR_STABILIZE_DELAY = 2;     // секунд

// ========== НАСТРОЙКИ СВЕТА ==========
namespace Light {
    constexpr uint8_t ON_HOUR = 8;
    constexpr uint8_t ON_MINUTE = 0;
    constexpr uint8_t OFF_HOUR = 24;
    constexpr uint8_t OFF_MINUTE = 5;
}

// ========== НАСТРОЙКИ ФОТО ==========
constexpr uint16_t PHOTO_DURATION = 180;  // секунд