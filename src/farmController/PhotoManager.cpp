#include "PhotoManager.h"
#include "Config.h"

static const PhotoConfig photoSchedules[] = {
    {1,  10, 180, true, 5, true, true, 5},
    {11, 20, 180, true, 5, true, true, 5},
    {21, 0,  180, true, 5, true, true, 5}
};
static const int numPhotoSchedules = sizeof(photoSchedules) / sizeof(photoSchedules[0]);

PhotoManager::PhotoManager(LightController* ctrl)
    : leds(ctrl)
{
}

void PhotoManager::update(const DateTime& now, int experimentDay) {
    PhotoConfig cfg = getConfigForDay(experimentDay);

    if (state == NORMAL) {
        uint32_t nowMinutes = now.hour() * 60 + now.minute();

        if (cfg.enableStart && !photoStartDone) {
            checkMorning(now, cfg);
        }
        if (cfg.enableMiddle && !photoMiddleDone) {
            checkMiddle(now, cfg);
        }
        if (cfg.enableEnd && !photoEndDone) {
            checkEvening(now, cfg);
        }
    }
    else if (state == PHOTO) {
        if (millis() - photoStartTime >= photoDuration) {
            endPhoto();
        }
    }
}

void PhotoManager::resetDaily() {
    photoStartDone = false;
    photoMiddleDone = false;
    photoEndDone = false;
}

PhotoConfig PhotoManager::getConfigForDay(int day) const {
    for (int i = 0; i < numPhotoSchedules; i++) {
        if (day >= photoSchedules[i].startDay &&
            (photoSchedules[i].endDay == 0 || day <= photoSchedules[i].endDay))
        {
            return photoSchedules[i];
        }
    }
    PhotoConfig empty = { 0, false, 0, false, false, 0 };
    return empty;
}

void PhotoManager::checkMorning(const DateTime& now, const PhotoConfig& cfg) {
    uint32_t nowMinutes = now.hour() * 60 + now.minute();
    uint32_t onMinutes = Light::ON_HOUR * 60 + Light::ON_MINUTE;
    int32_t startPhotoTime = onMinutes - cfg.startDelayMin;
    if (startPhotoTime < 0) startPhotoTime += 24 * 60;
    if (nowMinutes == startPhotoTime) {
        startPhoto(cfg);
        photoStartDone = true;  
    }
}

void PhotoManager::checkMiddle(const DateTime& now, const PhotoConfig& cfg) {
    uint32_t nowMinutes = now.hour() * 60 + now.minute();
    if (nowMinutes == 16 * 60) {
        startPhoto(cfg);
        photoMiddleDone = true;
    }
}

void PhotoManager::checkEvening(const DateTime& now, const PhotoConfig& cfg) {
    uint32_t nowMinutes = now.hour() * 60 + now.minute();
    uint32_t offMinutes = Light::OFF_HOUR * 60 + Light::OFF_MINUTE;
    if (offMinutes == 0) offMinutes = 24 * 60;
    uint32_t endPhotoTime = offMinutes + cfg.endBeforeMin;
    if (endPhotoTime >= 24 * 60) endPhotoTime -= 24 * 60;
    if (nowMinutes == endPhotoTime) {
        startPhoto(cfg);
        photoEndDone = true;
    }
}

void PhotoManager::startPhoto(const PhotoConfig& cfg) {
    leds->getCurrentBrightness(prevBrightness);
    leds->setAll(0, 0, 0, 0, 0, 0);
    leds->setCamera(true);
    leds->triggerFlash();
    photoStartTime = millis();
    photoDuration = cfg.DurationSec * 1000UL;
    state = PHOTO;
    Serial.println("Photo session started");
}

void PhotoManager::endPhoto() {
    leds->setCamera(false);
    leds->flashOff();
    leds->setAll(prevBrightness[0], prevBrightness[1], prevBrightness[2],
        prevBrightness[3], prevBrightness[4], prevBrightness[5]);

    state = NORMAL;
    Serial.println("Photo session ended");
}
