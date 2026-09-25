#pragma once
#include <Arduino.h>
#include "drivers/button_driver.h"
#include "Nicla_System.h"
#include "rtos.h"

// =============================================================================
// Enumeraciones y constantes
// =============================================================================

#define PANIC_BUTTON 0 
#define NOTIF_BUTTON 1 
#define BUTTON_COUNT 2

// =============================================================================
// Clase InterfaceDriver (Singleton)
// =============================================================================

class InterfaceDriver {
public:
    static InterfaceDriver* getInstance();
    void init();

    // Prohibimos copia y asignación (refuerzo del Singleton)
    InterfaceDriver(const InterfaceDriver&) = delete;
    InterfaceDriver& operator=(const InterfaceDriver&) = delete;

    // =========================================================================
    // Métodos para control del LED (delega a RGBled interno)
    // =========================================================================
    
    void setLedState(bool active); 
    void setLedColor(RGBColors color);
    
    void setLedNotif();
    void setLedAlarm();
    void setLedWarn();

    void toggleLed();

    inline bool getLedState() const { return _ledState; }
    inline RGBColors getLedColor() const { return _ledColor; }
    
    // =========================================================================
    // Métodos para manejo de pulsadores y alertas
    // =========================================================================
    
    /**
     * @brief Handler de interrupción único para todos los botones.
     *        Destraba el semáforo para que la tarea ejecute la MDE.
     */
    static void buttonInterruptHandler();

    /**
     * @brief Máquina de estados (MDE) para evaluar el estado de los botones.
     *        Debe ser llamada desde una tarea (unbalecked by buttonSemaphore).
     */
    void processButtonsMDE();

    // Consultar y limpiar flags de eventos
    bool isPanicPressed() const { return _panicAlert; }
    bool isNotifPressed() const { return _notifAlert; }
    void clearPanicAlert() { _panicAlert = false; }
    void clearNotifAlert() { _notifAlert = false; }

    // Semáforo para destrabar la tarea que lee botones (mbed OS).
    // count = 0 (inicia bloqueado), max_count = 1 (semáforo binario)
    rtos::Semaphore buttonSemaphore{0, 1};
    
    // Función para liberar semáforo desde ISR
    void giveSemaphoreFromISR();

private:
    static InterfaceDriver* _instance;
    static InterfaceDriver* createInstance();

    InterfaceDriver();
    
    // =========================================================================
    // LED
    // =========================================================================
    
    RGBled* led = &nicla::leds;
    bool _ledState;
    RGBColors _ledColor;
    
    // =========================================================================
    // Botones y MDE
    // =========================================================================
    
    ButtonDriver buttons[MAX_BUTTONS];

    enum class ButtonState {
        IDLE,
        DEBOUNCE,
        CHECK,
        FINISH_EVENT
    };

    ButtonState _mdeState;
    unsigned long _mdeTimer;

    volatile bool _panicAlert;
    volatile bool _notifAlert;
};