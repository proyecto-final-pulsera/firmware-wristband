#pragma once
#include <Arduino.h>
#include <mbed.h>
#define PANIC_BUTTON_PIN 10
#define NOTIF_BUTTON_PIN 11

using InterruptMode = decltype(FALLING);

// =============================================================================
// Clase Base: ButtonDriver
// =============================================================================
class ButtonDriver {
public:
    ButtonDriver(uint8_t pin, bool pullup, InterruptMode mode);

    // Asignación de IRQ externa
    void setIrqHandler(void (*handler)());
    void enableInterrupt();
    void disableInterrupt();
    
    // Lee el pin (devuelve true si está siendo presionado físicamente)
    bool getState() const;

    // Dispara el inicio de la Máquina de Estados. 
    // Retorna true si arrancó exitosamente (ignorando los rebotes)
    bool onInterrupt();

    // MDE no bloqueante: Procesa el Debounce y la retención. 
    void updateMDE();

    // Devuelve true una sola vez por cada flanco procesado, limpiando el flag interno.
    bool getPressed();

    // Devuelve true si la MDE está inactiva (IDLE)
    bool isIdle() const { return _mdeState == IDLE; }

protected:
    void init();

    uint8_t _pin;
    bool _pullup;
    InterruptMode _mode;
    void (*_irqHandler)();
    class mbed::InterruptIn* _mbedIrq;

    // Estados de la MDE
    enum ButtonState {
        IDLE,
        DEBOUNCE_PRESS,
        WAIT_RELEASE,
        DEBOUNCE_RELEASE
    };

    ButtonState _mdeState;
    unsigned long _mdeTimer;
    bool _eventPending;
};

// =============================================================================
// Singleton: PanicButton
// =============================================================================
class PanicButton : public ButtonDriver {
public:
    static PanicButton* getInstance();

    PanicButton(const PanicButton&) = delete;
    PanicButton& operator=(const PanicButton&) = delete;

private:
    PanicButton();
    static PanicButton* _instance;
};

// =============================================================================
// Singleton: NotifButton
// =============================================================================
class NotifButton : public ButtonDriver {
public:
    static NotifButton* getInstance();

    NotifButton(const NotifButton&) = delete;
    NotifButton& operator=(const NotifButton&) = delete;

private:
    NotifButton();
    static NotifButton* _instance;
};

