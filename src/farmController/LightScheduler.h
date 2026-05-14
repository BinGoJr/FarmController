// LightScheduler.h
#pragma once
#include <Arduino.h>
#include "LightController.h"
#include <RTClib.h>

struct TimeSlot {
    uint8_t startHour;
    uint8_t endHour;        // 0 = 24:00
    uint8_t brightness[3];
};

struct DayTactic {
    const char* name;
    const TimeSlot* slots;  
};

struct PhaseSec {
    uint8_t startDay;
    uint8_t endDay;
    const DayTactic* tactic;
};

class LightScheduler {
public:
    LightScheduler(LightController* ctrl);
    void update(const DateTime& now, int experimentDay);

private:
    LightController* leds;
    int lastDay = 0;

    const DayTactic* getTacticForSec(const PhaseSec* phases, int numPhases, int day);
    void applyBrightness(const DateTime& now,
        const DayTactic* tac1,
        const DayTactic* tac2);
    bool isTimeInSlot(const DateTime& now, const TimeSlot& slot);

    static const PhaseSec phasesSec1[];
    static const PhaseSec phasesSec2[];
    static const int numPhasesSec1;
    static const int numPhasesSec2;
};