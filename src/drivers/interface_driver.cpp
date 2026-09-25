#include "drivers/interface_driver.h"

// ==================================================================
// Singleton
// ==================================================================
InterfaceDriver* InterfaceDriver::_instance = nullptr;

InterfaceDriver* InterfaceDriver::getInstance() {
    if (_instance == nullptr) {
        _instance = createInstance();
    }
    return _instance;
}

InterfaceDriver* InterfaceDriver::createInstance() {
    return new InterfaceDriver();
}

// ==================================================================
// Implementación de InterfaceDriver
// ==================================================================

InterfaceDriver::InterfaceDriver() 
    : _ledState(false), 
      _ledColor(RGBColors::off),
      buttons{ 
          // Pullup = true, por lo tanto presionar es FALLING
          ButtonDriver(1, true, FALLING), // PANIC_BUTTON
          ButtonDriver(2, true, FALLING)  // NOTIF_BUTTON
      },
      _mdeState(ButtonState::IDLE),
      _mdeTimer(0),
      _panicAlert(false),
      _notifAlert(false)
{
    led = &nicla::leds;
    // init() debe llamarse desde App::init()
}

void InterfaceDriver::init() {
    // Inicialización del LED
    if (led != nullptr) {
        led->begin();
    }

    // Configurar handler global para todos los botones
    ButtonDriver::setGlobalHandler(buttonInterruptHandler);

    // Habilitar las interrupciones para cada botón inicialmente
    for (int i = 0; i < MAX_BUTTONS; i++) {
        buttons[i].enableInterrupt();
    }
}

// ==================================================================
// LED
// ==================================================================

void InterfaceDriver::setLedState(bool active) {
    _ledState = active;
    if (led != nullptr) {
        if (_ledState) {
            led->setColor(_ledColor);
        } else {
            led->setColor(RGBColors::off); 
        }
    }
}

void InterfaceDriver::setLedColor(RGBColors color) {
    _ledColor = color;
    if (_ledState && led != nullptr) {
        led->setColor(_ledColor);
    }
}

void InterfaceDriver::setLedNotif() {
    setLedColor(RGBColors::blue);
    setLedState(true);
}

void InterfaceDriver::setLedAlarm() {
    setLedColor(RGBColors::red);
    setLedState(true);
}

void InterfaceDriver::setLedWarn() {
    setLedColor(RGBColors::yellow);
    setLedState(true);
}

void InterfaceDriver::toggleLed() {
    setLedState(!_ledState);
}

// ==================================================================
// Botones y MDE
// ==================================================================

void InterfaceDriver::buttonInterruptHandler() {
    InterfaceDriver* inst = InterfaceDriver::getInstance();
    if (inst) {
        // IMPORTANTE: Mbed OS crashea si llamamos a detachInterrupt() desde una ISR
        // ya que la función nativa intenta tomar un Mutex ("Not allowed in ISR context").
        // Desactivaremos las interrupciones en la tarea de MDE (fuera de la ISR).
        inst->giveSemaphoreFromISR();
    }
}

void InterfaceDriver::giveSemaphoreFromISR() {
    buttonSemaphore.release();
}

void InterfaceDriver::processButtonsMDE() {
    switch (_mdeState) {
        case ButtonState::IDLE:
            // Estado de reposo (normalmente la tarea está bloqueada en el semáforo)
            // Cuando la tarea pase el semáforo, transicionamos a DEBOUNCE.
            // Es aquí, FUERA de la ISR, donde es seguro deshabilitar las interrupciones
            for (int i = 0; i < MAX_BUTTONS; i++) {
                buttons[i].disableInterrupt();
            }
            _mdeState = ButtonState::DEBOUNCE;
            _mdeTimer = millis();
            break;

        case ButtonState::DEBOUNCE:
            // Espera de 50 ms
            if (millis() - _mdeTimer >= 50) {
                _mdeState = ButtonState::CHECK;
            }
            break;

        case ButtonState::CHECK:
            // Evaluar los botones

            if (buttons[PANIC_BUTTON].getState()) {
                _panicAlert = true;
            }
            if (buttons[NOTIF_BUTTON].getState()) {
                _notifAlert = true;
            }

            _mdeState = ButtonState::FINISH_EVENT;
            break;

        case ButtonState::FINISH_EVENT:
            // Esperar a que se suelten todos los botones
            bool anyPressed = false;
            for (int i = 0; i < MAX_BUTTONS; i++) {
                if (buttons[i].getState()) {
                    anyPressed = true;
                    break;
                }
            }

            if (!anyPressed) {
                // Reactivamos las interrupciones para volver a detectar eventos
                for (int i = 0; i < MAX_BUTTONS; i++) {
                    buttons[i].enableInterrupt();
                }
                
                // Retornamos a estado seguro para que la tarea se vuelva a dormir
                _mdeState = ButtonState::IDLE;
            }
            break;
    }
}
