#include "drivers/led_driver.h"

LedDriver* LedDriver::getInstance() {
    static LedDriver instance;
    return &instance;
}

LedDriver::LedDriver() : _ledState(false), _ledColor(RGBColors::off) {
}

void LedDriver::init() {
    nicla::leds.begin();
    nicla::leds.setColor(RGBColors::off);
}

void LedDriver::setLedState(bool active) {
    _ledState = active;
    if (_ledState) {
        nicla::leds.setColor(_ledColor);
    } else {
        nicla::leds.setColor(RGBColors::off); 
    }
}

void LedDriver::setLedColor(RGBColors color) {
    _ledColor = color;
    if (_ledState) {
        nicla::leds.setColor(_ledColor);
    }
}

void LedDriver::setLedNotif() {
    setLedColor(RGBColors::blue);
    setLedState(true);
}

void LedDriver::setLedAlarm() {
    setLedColor(RGBColors::red);
    setLedState(true);
}

void LedDriver::setLedWarn() {
    setLedColor(RGBColors::yellow);
    setLedState(true);
}

void LedDriver::toggleLed() {
    setLedState(!_ledState);
}
