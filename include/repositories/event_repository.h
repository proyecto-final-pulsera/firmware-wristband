#pragma once
#include <Arduino.h>
#include "sensors/SensorClass.h"
#include "drivers/bhi260_driver.h"

/**
 * @brief Repository general para sensores de tipo "Evento" (Ej: Tilt, Wake Gesture, Step Detector).
 * No almacena un valor numérico, sino que reacciona a las notificaciones (paquetes)
 * actualizando banderas de estado, contadores de ocurrencia y la marca de tiempo.
 */
class EventRepository : public SensorClass {
public:
    /**
     * @brief Constructor.
     * @param sensorId El ID del sensor de evento que se quiere instanciar (Ej: SENSOR_ID_TILT_DETECTOR).
     */
    EventRepository(uint8_t sensorId);
    virtual ~EventRepository();

    // --- Métodos virtuales puros reescritos de SensorClass ---
    void setData(SensorDataPacket &data) override;
    void setData(SensorLongDataPacket &data) override;
    String toString() override;

    // --- Métodos del Evento ---

    /**
     * @brief Indica si el evento ocurrió (se recibió un paquete) desde la última vez que se limpió.
     */
    bool hasEventOccurred() const;

    /**
     * @brief Baja la bandera de ocurrencia de evento.
     */
    void clearEventFlag();

    /**
     * @brief Obtiene el momento (en milisegundos) en el que ocurrió el último evento.
     */
    uint32_t getLastEventMillis() const;
    
    /**
     * @brief Obtiene el acumulado de cuántas veces saltó este evento.
     */
    uint32_t getEventCount() const;

    /**
     * @brief Resetea a cero el contador total de eventos ocurridos.
     */
    void clearEventCount();

private:
    bool _eventReceived;
    uint32_t _lastEventMillis;
    uint32_t _eventCount;
};

// ============================================================================
// Singletons Específicos para Eventos
// ============================================================================

class MotionRepository : public EventRepository {
public:
    static MotionRepository* getInstance();
private:
    MotionRepository();
    static MotionRepository* _instance;
};

class NoMotionRepository : public EventRepository {
public:
    static NoMotionRepository* getInstance();
private:
    NoMotionRepository();
    static NoMotionRepository* _instance;
};

class StepCounterRepository : public EventRepository {
public:
    static StepCounterRepository* getInstance();
private:
    StepCounterRepository();
    static StepCounterRepository* _instance;
};
