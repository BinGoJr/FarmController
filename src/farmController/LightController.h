#pragma once
#include <Arduino.h>
#include "Config.h"

class LightController {
public:
    void begin();
    void setSection1(uint8_t ch0, uint8_t ch1, uint8_t ch2);
    void setSection2(uint8_t ch0, uint8_t ch1, uint8_t ch2);
    void setAll(uint8_t s1ch0, uint8_t s1ch1, uint8_t s1ch2,
                uint8_t s2ch0, uint8_t s2ch1, uint8_t s2ch2);
    void getCurrentBrightness(uint8_t* buf);
    void setCamera(bool on);     
    void triggerFlash();
    void flashOff();

private:
    void writeChannel(uint8_t mosfetPin, uint8_t relayPin, uint8_t brightness);

    uint8_t lastSec1Ch0 = 0, lastSec1Ch1 = 0, lastSec1Ch2 = 0;
    uint8_t lastSec2Ch0 = 0, lastSec2Ch1 = 0, lastSec2Ch2 = 0;
};