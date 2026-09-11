#pragma once
#include <Arduino.h>
#include "BoschSensortec.h"

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
// --- IDs de Sensores Físicos y Virtuales (Comunes) ---
#define SENSOR_ID_ACCEL           1
#define SENSOR_ID_ACCEL_CORRECTED 4
#define SENSOR_ID_TILT_DETECTOR   48
#define SENSOR_ID_STEP_COUNTER    53
#define SENSOR_ID_SIGNIFICANT_MOT 55
#define SENSOR_ID_WAKE_GESTURE    57
#define SENSOR_ID_GLANCE_GESTURE  59
#define SENSOR_ID_PICKUP_GESTURE  61
#define SENSOR_ID_WRIST_TILT      67
#define SENSOR_ID_DEVICE_ORI      70
#define SENSOR_ID_STATIONARY_DET  75
#define SENSOR_ID_MOTION_DET      77
#define SENSOR_ID_STEP_DETECTOR   94

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

    // Vacía las FIFOs para forzar el pin de interrupción a LOW
    void flushFIFOs();
    
    // Configuración del Host Interface Control (Low Power)
    void suspendHost();
    void resumeHost();
    
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

    /**
     * @brief ISR handler logic to be called when the interrupt triggers.
     */
    void handleInterrupt();

    /**
     * @brief Processes data from the wakeup FIFO.
     */
    void updateWakeupFIFO();

    /**
     * @brief Processes data from the non-wakeup FIFO.
     */
    void updateNonWakeupFIFO();

    /**
     * @brief Checks if the interrupt flag is set.
     * 
     * @return true if the interrupt was triggered.
     */
    bool isInterruptTriggered() const;

    /**
     * @brief Clears the interrupt flag.
     */
    void clearInterruptFlag();

protected:
    BHI260Driver();
    ~BHI260Driver();

private:
    static BHI260Driver* _instance;
    
    // Default pin is GPIO13 on the ANNA-B112 module (to be implemented)
    uint32_t _interruptPin;
    void (*_isr_handler)(void);
    
    volatile bool _interruptTriggered;
};
