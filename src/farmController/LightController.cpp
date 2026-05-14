#include "LightController.h"
#include "Config.h"

void LightController::begin() {
    pinMode(Pins::mosfetSec1Ch0, OUTPUT);
    pinMode(Pins::mosfetSec1Ch1, OUTPUT);
    pinMode(Pins::mosfetSec1Ch2, OUTPUT);
    pinMode(Pins::mosfetSec2Ch0, OUTPUT);
    pinMode(Pins::mosfetSec2Ch1, OUTPUT);
    pinMode(Pins::mosfetSec2Ch2, OUTPUT);

    pinMode(Pins::relaySec1Ch0, OUTPUT);
    pinMode(Pins::relaySec1Ch1, OUTPUT);
    pinMode(Pins::relaySec1Ch2, OUTPUT);
    pinMode(Pins::relaySec2Ch0, OUTPUT);
    pinMode(Pins::relaySec2Ch1, OUTPUT);
    pinMode(Pins::relaySec2Ch2, OUTPUT);
    pinMode(Pins::relayCamera, OUTPUT);
    pinMode(Pins::relayLamp1, OUTPUT);
    pinMode(Pins::relayLamp2, OUTPUT);

    analogWrite(Pins::mosfetSec1Ch0, 0);
    analogWrite(Pins::mosfetSec1Ch1, 0);
    analogWrite(Pins::mosfetSec1Ch2, 0);
    analogWrite(Pins::mosfetSec2Ch0, 0);
    analogWrite(Pins::mosfetSec2Ch1, 0);
    analogWrite(Pins::mosfetSec2Ch2, 0);

    digitalWrite(Pins::relaySec1Ch0, HIGH);
    digitalWrite(Pins::relaySec1Ch1, HIGH);
    digitalWrite(Pins::relaySec1Ch2, HIGH);
    digitalWrite(Pins::relaySec2Ch0, HIGH);
    digitalWrite(Pins::relaySec2Ch1, HIGH);
    digitalWrite(Pins::relaySec2Ch2, HIGH);
    digitalWrite(Pins::relayCamera, HIGH);
    digitalWrite(Pins::relayLamp1, HIGH);
    digitalWrite(Pins::relayLamp2, HIGH);
    
}

void LightController::writeChannel(uint8_t mosfetPin, uint8_t relayPin, uint8_t brightness) {
    analogWrite(mosfetPin, brightness);
    digitalWrite(relayPin, brightness > 0 ? LOW : HIGH);
}

void LightController::setCamera(bool on) {
    digitalWrite(Pins::relayCamera, on ? LOW : HIGH);
}

void LightController::triggerFlash() {
    digitalWrite(Pins::relayLamp1, LOW);   // relayLamp2Pin
    delay(1000);
    digitalWrite(Pins::relayLamp1, HIGH);
    delay(1000);
    digitalWrite(Pins::relayLamp2, LOW);   // relayLamp1Pin
    delay(1000);
    digitalWrite(Pins::relayLamp1, LOW);
}

void LightController::flashOff() {
    digitalWrite(Pins::relayLamp1, HIGH);
    digitalWrite(Pins::relayLamp2, HIGH);
}

void LightController::setSection1(uint8_t ch0, uint8_t ch1, uint8_t ch2) {
    writeChannel(Pins::mosfetSec1Ch0, Pins::relaySec1Ch0, ch0);
    writeChannel(Pins::mosfetSec1Ch1, Pins::relaySec1Ch1, ch1);
    writeChannel(Pins::mosfetSec1Ch2, Pins::relaySec1Ch2, ch2);

    lastSec1Ch0 = ch0;
    lastSec1Ch1 = ch1;
    lastSec1Ch2 = ch2;
}

void LightController::setSection2(uint8_t ch0, uint8_t ch1, uint8_t ch2) {
    writeChannel(Pins::mosfetSec2Ch0, Pins::relaySec2Ch0, ch0);
    writeChannel(Pins::mosfetSec2Ch1, Pins::relaySec2Ch1, ch1);
    //writeChannel(Pins::mosfetSec2Ch2, Pins::relaySec2Ch2, ch2); //заменить потом на настоящую линию
    //digitalWrite(Pins::relayLamp2, ch2 > 0 ? LOW : HIGH);

    lastSec2Ch0 = ch0;
    lastSec2Ch1 = ch1;
    lastSec2Ch2 = ch2;
}

void LightController::setAll(uint8_t s1ch0, uint8_t s1ch1, uint8_t s1ch2,
                             uint8_t s2ch0, uint8_t s2ch1, uint8_t s2ch2) {
    setSection1(s1ch0, s1ch1, s1ch2);
    setSection2(s2ch0, s2ch1, s2ch2);
}

void LightController::getCurrentBrightness(uint8_t* buf) {
    buf[0] = lastSec1Ch0;
    buf[1] = lastSec1Ch1;
    buf[2] = lastSec1Ch2;
    buf[3] = lastSec2Ch0;
    buf[4] = lastSec2Ch1;
    buf[5] = lastSec2Ch2;
}