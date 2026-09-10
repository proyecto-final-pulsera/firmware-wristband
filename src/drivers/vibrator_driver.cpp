#include "drivers/vibrator_driver.h"

// ==================================================================
// Singleton
// ==================================================================
VibratorDriver* VibratorDriver::_instance = nullptr;

VibratorDriver* VibratorDriver::getInstance() {
    return _instance;
}

VibratorDriver* VibratorDriver::createInstance() {
    if (_instance == nullptr) {
        _instance = new VibratorDriver();
    }
    return _instance;
}

// ==================================================================
// Implementación de VibratorDriver
// ==================================================================

VibratorDriver::VibratorDriver() : 
    _maxStrength(255), 
    _minStrength(0), 
    _strength(0), 
    _state(false) 
{
    init();
}

void VibratorDriver::init() {
    // Inicialización del pin PWM
}

void VibratorDriver::enable() {
    _state = true;
    // Lógica para encender el PWM
}

void VibratorDriver::disable() {
    _state = false;
    // Lógica para apagar el PWM
}

void VibratorDriver::brake() {
    // Lógica para aplicar el duty de frenado
}

void VibratorDriver::setStrength(uint8_t strength) {
    if (strength > _maxStrength) {
        _strength = _maxStrength;
    } else if (strength < _minStrength) {
        _strength = _minStrength;
    } else {
        _strength = strength;
    }
    // Lógica para actualizar el duty cyle del PWM si está encendido
}

uint8_t VibratorDriver::getStrength() {
    return _strength;
}

bool VibratorDriver::getState() {
    return _state;
}

void VibratorDriver::setMaxStrength(uint8_t max_strength) {
    _maxStrength = max_strength;
    if (_strength > _maxStrength) {
        setStrength(_maxStrength);
    }
}
