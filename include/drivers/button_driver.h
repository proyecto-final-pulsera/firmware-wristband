#pragma once
#include <Arduino.h>

#define MAX_BUTTONS 2

/**
 * @typedef InterruptMode
 * @brief Usamos el tipo exacto que define Arduino para FALLING/CHANGE.
 *        En cores modernos (como Mbed) es PinStatus (enum), en AVR es int.
 */
using InterruptMode = decltype(FALLING);

/**
 * @class ButtonDriver
 * @brief Maneja un botón individual conectado a un pin GPIO.
 */
class ButtonDriver {
public:
    /**
     * @brief Constructor del driver de botón.
     * @param pin Pin GPIO.
     * @param pullup True si el pin requiere resistencia pull-up interna.
     * @param mode Modo de interrupción (FALLING, RISING, CHANGE, etc).
     */
    ButtonDriver(uint8_t pin, bool pullup, InterruptMode mode);
    
    /**
     * @brief Setea la función global (de nivel superior) que se ejecutará 
     *        cuando cualquier botón dispare la interrupción.
     */
    static void setGlobalHandler(void(*handler)());

    /**
     * @brief Reactiva la interrupción para este pin usando el handler global y modo.
     */
    void enableInterrupt();

    /**
     * @brief Desactiva la interrupción de este pin.
     */
    void disableInterrupt();

    inline uint8_t getPin() const { return _pin; }
    inline bool getPullup() const { return _pullup; }
    inline InterruptMode getMode() const { return _mode; }
    
    /**
     * @brief Lee el estado digital actual del pin del botón.
     */
    bool getState() const;

    /**
     * @brief Llama al handler de nivel superior. Invocado por el ISR único.
     */
    static void triggerGlobalHandler();

private:
    void init();
    
    uint8_t _pin;
    bool _pullup;
    InterruptMode _mode;
    
    static void (*_globalHandler)();
};

// =============================================================================
// ISR Handler único
// =============================================================================

/** @brief ISR único que comparten todos los botones al dispararse. */
void btn_isr();

