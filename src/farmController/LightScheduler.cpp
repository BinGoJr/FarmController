#include "LightScheduler.h"
#include "Config.h"   

static const TimeSlot tactic_morning_blue[] = {
    { 8, 10, {255,   0,   255}},
    {10, 22, {0, 255,   0}},
    {22,  0, {255,   0, 255}},  
    { 0,  0, {  0,   0,   0}}
};

static const TimeSlot tactic_full_day[] = {
    { 8,  0, {67, 255, 0}},
    { 0,  0, {  0,   0,   0}}
};

static const DayTactic tactic1 = { "MorningBlue", tactic_morning_blue };
static const DayTactic tactic2 = { "FullDay",      tactic_full_day };

const PhaseSec LightScheduler::phasesSec1[] = {
    { 1,  5, &tactic1 },
    { 6, 15, &tactic2 },
    {16,  0, &tactic1 }
};
const int LightScheduler::numPhasesSec1 = 3;

const PhaseSec LightScheduler::phasesSec2[] = {
    { 1, 0, &tactic2 }
};
const int LightScheduler::numPhasesSec2 = 1;

// -----------------------------------------------------------
LightScheduler::LightScheduler(LightController* ctrl) : leds(ctrl) {}

void LightScheduler::update(const DateTime& now, int day) {
    const DayTactic* tac1 = getTacticForSec(phasesSec1, numPhasesSec1, day);
    const DayTactic* tac2 = getTacticForSec(phasesSec2, numPhasesSec2, day);

    applyBrightness(now, tac1, tac2);
}

const DayTactic* LightScheduler::getTacticForSec(const PhaseSec* phases, int num, int day) {
    for (int i = 0; i < num; i++) {
        if (day >= phases[i].startDay &&
            (phases[i].endDay == 0 || day <= phases[i].endDay)) {
            return phases[i].tactic;
        }
    }
    return nullptr;
}

bool LightScheduler::isTimeInSlot(const DateTime& now, const TimeSlot& slot) {
    uint8_t nowHour = now.hour();
    uint8_t startH = slot.startHour;
    uint8_t endH = slot.endHour;
    if (endH == 0) endH = 24;

    if (startH < endH) {
        return (nowHour >= startH && nowHour < endH);
    }
    else {
        return (nowHour >= startH || nowHour < endH);
    }
}

void LightScheduler::applyBrightness(const DateTime& now,
    const DayTactic* tac1,
    const DayTactic* tac2) {
    uint8_t s1[3] = { 0,0,0 }, s2[3] = { 0,0,0 };

    if (tac1) {
        for (const TimeSlot* s = tac1->slots;
            !(s->startHour == 0 && s->endHour == 0 && s->brightness[0] == 0
                && s->brightness[1] == 0 && s->brightness[2] == 0);
                ++s) {
            if (isTimeInSlot(now, *s)) {
                s1[0] = s->brightness[0];
                s1[1] = s->brightness[1];
                s1[2] = s->brightness[2];
                break;
            }
        }
    }

    if (tac2) {
        for (const TimeSlot* s = tac2->slots;
            !(s->startHour == 0 && s->endHour == 0 && s->brightness[0] == 0
                && s->brightness[1] == 0 && s->brightness[2] == 0);
                ++s) {
            if (isTimeInSlot(now, *s)) {
                s2[0] = s->brightness[0];
                s2[1] = s->brightness[1];
                s2[2] = s->brightness[2];    
                break;
            }
        }
    }

    leds->setSection1(s1[0], s1[1], s1[2]);
    leds->setSection2(s2[0], s2[1], s2[2]);
}