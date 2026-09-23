#pragma once
#include <Arduino.h>
#include "sensors/SensorClass.h"
#include "drivers/bhi260_driver.h"

/**
 * @brief Driver de alto nivel para el sensor de Temperatura.
 * A diferencia de IMU y Presión, no utiliza FIFO. Conserva únicamente 
 * el último valor censado junto con su marca de tiempo (timestamp).
 */
class TemperatureSensorDriver : public SensorClass {
public:
    TemperatureSensorDriver();
    virtual ~TemperatureSensorDriver();

    // --- Métodos virtuales puros reescritos de SensorClass ---
    void setData(SensorDataPacket &data) override;
    void setData(SensorLongDataPacket &data) override;
    String toString() override;

    // --- Métodos de la Temperatura ---
    
    /**
     * @brief Indica si hay un nuevo dato de temperatura disponible desde la última lectura.
     */
    bool isUpdated() const;

    /**
     * @brief Obtiene el último valor de temperatura en grados Celsius (°C) 
     *        y baja el flag de dato actualizado.
     */
    float getTemp();

    /**
     * @brief Obtiene el momento (en milisegundos desde que encendió la placa) 
     *        en el que se recibió el último valor.
     */
    uint32_t getLastUpdateMillis() const;

private:
    float _lastTemp;
    uint32_t _lastUpdateMillis;
    bool _isUpdated;
};
