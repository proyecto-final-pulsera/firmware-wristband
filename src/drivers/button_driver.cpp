#include "drivers/button_driver.h"

// ==================================================================
// Implementación de ButtonDriver
// ==================================================================

void (*ButtonDriver::_globalHandler)() = nullptr;

ButtonDriver::ButtonDriver(uint8_t pin, bool pullup, InterruptMode mode) 
    : _pin(pin), _pullup(pullup), _mode(mode)
{
    init();
}

void ButtonDriver::init() {
    pinMode(_pin, _pullup ? INPUT_PULLUP : INPUT);
}

void ButtonDriver::setGlobalHandler(void(*handler)()) {
    _globalHandler = handler;
}

void ButtonDriver::enableInterrupt() {
    attachInterrupt(digitalPinToInterrupt(_pin), &btn_isr, _mode);
}

void ButtonDriver::disableInterrupt() {
    detachInterrupt(digitalPinToInterrupt(_pin));
}

void ButtonDriver::triggerGlobalHandler() {
    if (_globalHandler) {
        _globalHandler();
    }
}

bool ButtonDriver::getState() const {
    return digitalRead(_pin) == HIGH;
}

// ==================================================================
// ISR Handler único
// ==================================================================

void btn_isr() {
    ButtonDriver::triggerGlobalHandler();
}
