#include "repositories/event_repository.h"

EventRepository::EventRepository(uint8_t sensorId)
    : SensorClass(sensorId), 
      _eventReceived(false), 
      _lastEventMillis(0), 
      _eventCount(0)
{
}

EventRepository::~EventRepository() {
}

void EventRepository::setData(SensorDataPacket &data) {
    // Al ser un sensor de evento puro, no se parsea ningún valor interno (payload),
    // sino que la sola llegada del paquete ya implica que el evento ocurrió.
    
    _eventReceived = true;
    _lastEventMillis = millis();
    _eventCount++;
    
    // Notificamos al flag de la clase base
    setDataAvailFlag();
}

void EventRepository::setData(SensorLongDataPacket &data) {
    // No utilizado por sensores de eventos estándar
}

String EventRepository::toString() {
    return String("Event Triggered! Total count: ") + String(_eventCount);
}

bool EventRepository::hasEventOccurred() const {
    return _eventReceived;
}

void EventRepository::clearEventFlag() {
    _eventReceived = false;
    clearDataAvailFlag();
}

uint32_t EventRepository::getLastEventMillis() const {
    return _lastEventMillis;
}

uint32_t EventRepository::getEventCount() const {
    return _eventCount;
}

void EventRepository::clearEventCount() {
    _eventCount = 0;
}

// ============================================================================
// Singletons Específicos para Eventos
// ============================================================================

MotionRepository::MotionRepository() : EventRepository(SENSOR_ID_MOTION_DET) {}
MotionRepository* MotionRepository::getInstance() {
    static MotionRepository instance;
    return &instance;
}

NoMotionRepository::NoMotionRepository() : EventRepository(SENSOR_ID_STATIONARY_DET) {}
NoMotionRepository* NoMotionRepository::getInstance() {
    static NoMotionRepository instance;
    return &instance;
}

StepCounterRepository::StepCounterRepository() : EventRepository(53) {} // SENSOR_ID_STEP_COUNTER_WU
StepCounterRepository* StepCounterRepository::getInstance() {
    static StepCounterRepository instance;
    return &instance;
}
