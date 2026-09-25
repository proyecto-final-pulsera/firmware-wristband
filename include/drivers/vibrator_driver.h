#pragma once
#include <Arduino.h>

namespace mbed {
    class PwmOut;
    class DigitalOut;
}

/////////////////
////
//// Drive de DRV2603 Haptic Drive With Auto-Resonance Detection for Linear Resonance Actuators (LRA)
////
///////////////7//
#define PWM_FREQ 20000 // Frecuencia de 20KHz
#define PWM_DUTY_MAX 75.0f // Duty máximo 75%
#define PWM_DUTY_MIN 50.0f // Duty mínimo 50%

// Definición de pines para la placa Nicla
#define VIBRATOR_PWM_PIN    0
#define VIBRATOR_ENABLE_PIN 3                       

/**
 * @class VibratorDriver
 * @brief Controlador para el motor vibrador mediante PWM.
 */
class VibratorDriver {
public:
    void init();
    /**
     * @brief Obtiene la instancia existente del driver.
     * @return Puntero a la instancia. nullptr si aún no fue creada.
     */
    static VibratorDriver* getInstance();

    /**
     * @brief Crea la instancia del driver (si no existe) y la retorna.
     * @return Puntero a la instancia única.
     */
    // Prohibimos copia y asignación (refuerzo del Singleton)
    VibratorDriver(const VibratorDriver&) = delete;
    VibratorDriver& operator=(const VibratorDriver&) = delete;

    /**
     * @brief Habilita el vibrador.
     * Activa primero el PWM y luego enciende el pin de Enable.
     */
    void enable();

    /**
     * @brief Deshabilita el vibrador.
     * Apaga primero el pin de Enable y luego detiene el PWM.
     */
    void disable();
    
    /**
     * @brief Aplica un freno activo al vibrador.
     * Pone el duty del PWM al mínimo (50%) sin apagar el pin de Enable.
     */
    void brake(); 
    
    /**
     * @brief Configura la fuerza del vibrador.
     * @param strength Valor de 0 a 100. Se mapea al rango dinámico entre _minDuty y _maxDuty.
     */
    void setStrength(uint8_t strength);
    uint8_t getStrength();
    
    bool getState();
    
    /**
     * @brief Permite ajustar los límites del duty cycle en tiempo de ejecución.
     */
    void setMaxDuty(float max_duty);
    void setMinDuty(float min_duty);

private:
    static VibratorDriver* createInstance();
    /// Instancia única del Singleton.
    static VibratorDriver* _instance;

    /**
     * @brief Constructor privado.
     */
    VibratorDriver();


    
    // Función de ayuda para actualizar el PWM con el duty actual mapeado
    void updatePWM();

    uint8_t _strength;    // 0 a 100
    float _maxDuty;       // Límite máximo de duty (por defecto PWM_DUTY_MAX)
    float _minDuty;       // Límite mínimo de duty (por defecto PWM_DUTY_MIN)
    bool _state;

    // Punteros a los pines usando Mbed OS
    mbed::PwmOut* _pwm_pin;
    mbed::DigitalOut* _enable_pin;
};
