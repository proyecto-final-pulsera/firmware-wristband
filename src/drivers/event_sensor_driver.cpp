#include "drivers/event_sensor_driver.h"

EventSensorDriver::EventSensorDriver(uint8_t sensorId)
    : SensorClass(sensorId), 
      _eventReceived(false), 
      _lastEventMillis(0), 
      _eventCount(0)
{
}

EventSensorDriver::~EventSensorDriver() {
}

void EventSensorDriver::setData(SensorDataPacket &data) {
    // Al ser un sensor de evento puro, no se parsea ningún valor interno (payload),
    // sino que la sola llegada del paquete ya implica que el evento ocurrió.
    
    _eventReceived = true;
    _lastEventMillis = millis();
    _eventCount++;
    
    // Notificamos al flag de la clase base
    setDataAvailFlag();
}

void EventSensorDriver::setData(SensorLongDataPacket &data) {
    // No utilizado por sensores de eventos estándar
}

String EventSensorDriver::toString() {
    return String("Event Triggered! Total count: ") + String(_eventCount);
}

bool EventSensorDriver::hasEventOccurred() const {
    return _eventReceived;
}

void EventSensorDriver::clearEventFlag() {
    _eventReceived = false;
    clearDataAvailFlag();
}

uint32_t EventSensorDriver::getLastEventMillis() const {
    return _lastEventMillis;
}

uint32_t EventSensorDriver::getEventCount() const {
    return _eventCount;
}

void EventSensorDriver::clearEventCount() {
    _eventCount = 0;
}
