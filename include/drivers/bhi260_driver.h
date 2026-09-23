#pragma once
#include <Arduino.h>
#include "BoschSensortec.h"
#include "sensors/SensorID.h"

/**
 * @brief Driver class for the BHI260AP sensor.
 * 
 * This class acts as a wrapper over the global BoschSensortec instance.
 * 
 * IMPORTANT NOTE:
 * For this class to work properly, it must be declared as a `friend class` 
 * inside the `BoschSensortec` class (in the Arduino_BHY2 library).
 * This grants access to the internal `_bhy2` structure.
 */
// IDs de Sensores Físicos y Virtuales (Comunes)
// Se eliminaron las macros manuales (#define SENSOR_ID_...) porque ya están
// declaradas nativamente como un `enum` en la librería Arduino_BHY2 (sensors/SensorID.h),
// y mantenerlas causaba un conflicto (error "expected identifier before numeric constant").

class BHI260Driver {
public:

    static BHI260Driver* getInstance();

    /**
     * @brief Initializes the driver, calling nicla initialization routines and BoschSensortec::begin().
     * 
     * @return true if initialization was successful.
     */
    bool init();

    void updateFifoData();
    void configureSensor(SensorConfigurationPacket& config);

    // Vacía las FIFOs
    void flushFIFOs();
    
    // Configuración del Host Interface Control (Low Power)
    void disableNonWakeupFIFO();
    void enableNonWakeupFIFO();
    
    // Validar si el sensor está presente en el firmware del BHI260
    bool hasSensor(uint8_t sensorId);

    uint8_t availableSensorData();
    uint8_t availableLongSensorData();
    bool readSensorData(SensorDataPacket &data);
    bool readLongSensorData(SensorLongDataPacket &data);

    /**
     * @brief Sets the interrupt pin used for the BHI260AP sensor.
     * 
     * @param pin The GPIO pin number.
     */
    void setInterruptPin(uint32_t pin);

    /**
     * @brief Gets the currently configured interrupt pin.
     * 
     * @return uint32_t The current GPIO pin number used for interrupts.
     */
    uint32_t getInterruptPin() const;

    /**
     * @brief Configures the hardware interrupt.
     * 
     * @param isr_handler Pointer to the ISR function.
     */
    void configureInterrupt(void (*isr_handler)(void));

    void enableInterrupt();
    void disableInterrupt();

protected:
    BHI260Driver();
    ~BHI260Driver();

private:
    static BHI260Driver* _instance;
    
    uint32_t _interruptPin;
    void (*_isr_handler)(void);
};
