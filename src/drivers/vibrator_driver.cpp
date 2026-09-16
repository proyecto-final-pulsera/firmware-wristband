#include "drivers/vibrator_driver.h"
#include <mbed.h>

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
    _strength(0), 
    _maxDuty(PWM_DUTY_MAX),
    _minDuty(PWM_DUTY_MIN),
    _state(false),
    _pwm_pin(nullptr),
    _enable_pin(nullptr)
{
    init();
}

void VibratorDriver::init() {
    // Inicialización de pines usando Mbed OS
    _pwm_pin = new mbed::PwmOut(digitalPinToPinName(VIBRATOR_PWM_PIN));
    // Pasamos un 0 como segundo parámetro para que nazca en LOW y evitar glitches
    _enable_pin = new mbed::DigitalOut(digitalPinToPinName(VIBRATOR_ENABLE_PIN), 0);
    
    // Configurar frecuencia de PWM: 20kHz -> periodo = 1/20000 = 50 microsegundos
    _pwm_pin->period_us(50);
    
    // Inicia deshabilitado
    _pwm_pin->write(0.0f);
}

void VibratorDriver::updatePWM() {
    if (!_state) {
        _pwm_pin->write(0.0f);
        return;
    }
    
    // Mapeamos _strength (0 a 100) al rango del duty cycle (_minDuty a _maxDuty)
    // Fórmula: duty = min + (strength * (max - min) / 100)
    float duty_percent = _minDuty + (_strength * (_maxDuty - _minDuty) / 100.0f);
    
    // Escribimos el duty en mbed PwmOut (acepta valores de 0.0f a 1.0f)
    _pwm_pin->write(duty_percent / 100.0f);
}

void VibratorDriver::enable() {
    _state = true;
    
    // Activa primero el PWM
    updatePWM();
    
    // Luego enciende el pin de Enable
    _enable_pin->write(1);
}

void VibratorDriver::disable() {
    _state = false;
    
    // Apaga primero el pin de Enable
    _enable_pin->write(0);
    
    // Luego detiene el PWM
    _pwm_pin->write(0.0f);
}

void VibratorDriver::brake() {
    // Pone el duty del PWM al mínimo dinámico sin apagar el pin de Enable.
    if (_pwm_pin) {
        _pwm_pin->write(_minDuty / 100.0f);
    }
}

void VibratorDriver::setStrength(uint8_t strength) {
    _strength = (strength > 100) ? 100 : strength;
    
    if (_state) {
        updatePWM();
    }
}

uint8_t VibratorDriver::getStrength() {
    return _strength;
}

bool VibratorDriver::getState() {
    return _state;
}

void VibratorDriver::setMaxDuty(float max_duty) {
    _maxDuty = (max_duty > 100.0f) ? 100.0f : max_duty;
    if (_state) updatePWM();
}

void VibratorDriver::setMinDuty(float min_duty) {
    _minDuty = (min_duty < 0.0f) ? 0.0f : min_duty;
    if (_state) updatePWM();
}
