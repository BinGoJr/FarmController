#pragma once
#include <Arduino.h>
#include "LightController.h"
#include <RTClib.h>

struct PhotoConfig {
    uint8_t  startDay;        
    uint8_t  endDay;          
    uint16_t DurationSec;
    bool     enableStart;
    uint8_t  startDelayMin;
    bool     enableMiddle;
    bool     enableEnd;
    uint8_t  endBeforeMin;
};

class PhotoManager {
public:
    PhotoManager(LightController* ctrl);

    void update(const DateTime& now, int experimentDay);

    void resetDaily();

private:
    LightController* leds;

    enum State { NORMAL, PHOTO };
    State state = NORMAL;

    bool photoStartDone = false;
    bool photoMiddleDone = false;
    bool photoEndDone = false;

    unsigned long photoStartTime = 0;
    unsigned long photoDuration = 0;

    uint8_t prevBrightness[6] = {0};

    PhotoConfig getConfigForDay(int day) const;
    void startPhoto(const PhotoConfig& cfg);
    void endPhoto();

    void checkMorning(const DateTime& now, const PhotoConfig& cfg);
    void checkMiddle(const DateTime& now, const PhotoConfig& cfg);
    void checkEvening(const DateTime& now, const PhotoConfig& cfg);
};