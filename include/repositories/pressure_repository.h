#pragma once
#include <Arduino.h>
#include <mbed.h>
#include "sensors/SensorClass.h"
#include "drivers/bhi260_driver.h"

// Fallbacks in case they are not defined globally
#ifndef FREQ_PRESSURE
#define FREQ_PRESSURE 16
#endif

#ifndef LEN_BUFFER_PRESSURE_SEG
#define LEN_BUFFER_PRESSURE_SEG 16
#endif

#define PRESSURE_FIFO_SIZE (FREQ_PRESSURE * LEN_BUFFER_PRESSURE_SEG)

/**
 * @brief Repository de alto nivel para el sensor de Presión (Barómetro) heredado de SensorClass.
 * Mantiene un buffer circular (FIFO) propio para almacenar los últimos datos.
 */
class PressureRepository : public SensorClass {
public:
    static PressureRepository* getInstance();
    virtual ~PressureRepository();

private:
    PressureRepository();
    static PressureRepository* _instance;
    static PressureRepository* createInstance();

public:
    // --- Métodos virtuales puros reescritos de SensorClass ---
    void setData(SensorDataPacket &data) override;
    void setData(SensorLongDataPacket &data) override;
    String toString() override;

    // --- Métodos de la FIFO Circular ---
    void fifoFlush();
    bool push(const float& data);
    bool pop(float& data);
    uint16_t getAvailableCount() const;
    uint16_t getFifoValues(float* buffer, uint16_t maxLen);

    uint16_t copyFifoValues(float* buffer, uint16_t maxLen);
    uint16_t copyFifoValues(float* buffer, uint16_t maxLen, uint16_t index);
    uint16_t copyFifoValuesUnprotected(float* buffer, uint16_t maxLen, uint16_t index) const;

    bool isFull() const;
    bool hasOverflowed() const;
    void clearOverflow();
    uint16_t rewind(uint16_t steps);
    
    float getElementAt(uint16_t index);
    float getElementAtUnprotected(uint16_t index) const;

    // --- Métodos temporales de Test ---
    uint32_t getTotalPushed() const;
    void resetTotalPushed();

private:
    rtos::Mutex _mutex;
    float _fifo[PRESSURE_FIFO_SIZE];
    uint16_t _head;
    uint16_t _tail;
    uint16_t _count;
    bool _overflow;
    float _lastData;
    
    uint32_t _totalPushed;
};
