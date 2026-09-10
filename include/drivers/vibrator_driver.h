#pragma once
#include <Arduino.h>

#define PWM_FREQ 1000 // Frecuencia por defecto, ajustable según el motor
#define PWM_DUTY_BREAK 50

/**
 * @class VibratorDriver
 * @brief Controlador para el motor vibrador mediante PWM.
 */
class VibratorDriver {
public:
    /**
     * @brief Obtiene la instancia existente del driver.
     * @return Puntero a la instancia. nullptr si aún no fue creada.
     */
    static VibratorDriver* getInstance();

    /**
     * @brief Crea la instancia del driver (si no existe) y la retorna.
     * @return Puntero a la instancia única.
     */
    static VibratorDriver* createInstance();

    // Prohibimos copia y asignación (refuerzo del Singleton)
    VibratorDriver(const VibratorDriver&) = delete;
    VibratorDriver& operator=(const VibratorDriver&) = delete;

    void enable();
    void disable();
    
    /**
     * @brief Aplica un freno activo al vibrador.
     */
    void brake(); 
    
    void setStrength(uint8_t strength);
    uint8_t getStrength();
    
    bool getState();
    
    void setMaxStrength(uint8_t max_strength);

private:
    /// Instancia única del Singleton.
    static VibratorDriver* _instance;

    /**
     * @brief Constructor privado.
     */
    VibratorDriver();

    void init();

    uint8_t _maxStrength;
    uint8_t _minStrength;
    uint8_t _strength;
    bool _state;
};
