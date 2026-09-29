#include "drivers/button_driver.h"

// ==================================================================
// Implementación de ButtonDriver (Clase Base)
// ==================================================================

#include "mbed.h"

ButtonDriver::ButtonDriver(uint8_t pin, bool pullup, InterruptMode mode) 
    : _pin(pin), _pullup(pullup), _mode(mode), _irqHandler(nullptr), _mbedIrq(nullptr),
      _mdeState(IDLE), _mdeTimer(0), _eventPending(false)
{
    init();
}

void ButtonDriver::init() {
    // Solo configuramos el pin para lectura si NO vamos a usar _mbedIrq aun.
    // Mbed maneja la dirreccion y pull cuando creamos el InterruptIn.
    // Arduino pinMode a veces entra en conflicto con Mbed InterruptIn.
}

void ButtonDriver::setIrqHandler(void (*handler)()) {
    _irqHandler = handler;
}

void ButtonDriver::enableInterrupt() {
    if (_irqHandler != nullptr) {
        if (_mbedIrq == nullptr) {
            _mbedIrq = new mbed::InterruptIn(digitalPinToPinName(_pin));
            if (_pullup) {
                _mbedIrq->mode(PullUp);
            } else {
                _mbedIrq->mode(PullNone); // Usa PullNone para no interferir con tu pull-down externo
            }
        }
        
        if (_mode == RISING) {
            _mbedIrq->rise(_irqHandler);
        } else if (_mode == FALLING) {
            _mbedIrq->fall(_irqHandler);
        } else {
            _mbedIrq->rise(_irqHandler);
            _mbedIrq->fall(_irqHandler);
        }
    }
}

void ButtonDriver::disableInterrupt() {
    if (_mbedIrq != nullptr) {
        _mbedIrq->rise(nullptr);
        _mbedIrq->fall(nullptr);
    }
}

bool ButtonDriver::getState() const {
    // Si es pullup, presionado significa nivel BAJO.
    // Usamos mbedIrq->read() si esta inicializado, sino digitalRead.
    bool pinState;
    if (_mbedIrq != nullptr) {
        pinState = (_mbedIrq->read() == 1);
    } else {
        pinState = (digitalRead(_pin) == HIGH);
    }
    return _pullup ? !pinState : pinState;
}

bool ButtonDriver::onInterrupt() {
    if (_mdeState == IDLE) {
        // En MbedOS no podemos llamar a detachInterrupt() adentro de una ISR porque usa un Mutex
        // En su lugar, hacemos un enmascaramiento por software (ignoramos los siguientes rebotes)
        _mdeState = DEBOUNCE_PRESS;  // Arrancamos la MDE
        _mdeTimer = millis();
        return true; // Se aceptó la interrupción
    }
    return false; // Estaba en rebote, se ignora
}

void ButtonDriver::updateMDE() {
    switch (_mdeState) {
        case IDLE:
            // En reposo, no hacemos nada
            break;

        case DEBOUNCE_PRESS:
            if ((millis() - _mdeTimer) >= 50) {
                if (getState() == true) { 
                    _eventPending = true;       // Registramos el evento (pulsación válida)
                    _mdeState = WAIT_RELEASE;   // Esperamos a que suelte el botón
                } else {
                    _mdeState = IDLE;           // Falso contacto
                }
            }
            break;

        case WAIT_RELEASE:
            if (getState() == false) {          // Si el usuario soltó el botón
                _mdeTimer = millis();
                _mdeState = DEBOUNCE_RELEASE;
            }
            break;

        case DEBOUNCE_RELEASE:
            if ((millis() - _mdeTimer) >= 50) {
                if (getState() == false) {
                    _mdeState = IDLE;           // Rebote superado
                } else {
                    _mdeState = WAIT_RELEASE;   // Sigue presionado físicamente
                }
            }
            break;
    }
}

bool ButtonDriver::getPressed() {
    if (_eventPending) {
        _eventPending = false;
        return true;
    }
    return false;
}

// ==================================================================
// Implementación de PanicButton
// ==================================================================

PanicButton* PanicButton::_instance = nullptr;

PanicButton::PanicButton() 
    : ButtonDriver(PANIC_BUTTON_PIN, false, RISING) 
{
}

PanicButton* PanicButton::getInstance() {
    if (_instance == nullptr) {
        _instance = new PanicButton();
    }
    return _instance;
}

// ==================================================================
// Implementación de NotifButton
// ==================================================================

NotifButton* NotifButton::_instance = nullptr;

NotifButton::NotifButton() 
    : ButtonDriver(NOTIF_BUTTON_PIN, false, RISING) 
{
}

NotifButton* NotifButton::getInstance() {
    if (_instance == nullptr) {
        _instance = new NotifButton();
    }
    return _instance;
}
