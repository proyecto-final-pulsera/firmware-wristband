#include "drivers/pressure_sensor_driver.h"
#include "sensors/DataParser.h"

PressureSensorDriver::PressureSensorDriver() 
    : SensorClass(SENSOR_ID_BARO), 
      _head(0), _tail(0), _count(0), _overflow(false), _lastData(0.0f), _totalPushed(0)
{
}

PressureSensorDriver::~PressureSensorDriver() {
}

void PressureSensorDriver::setData(SensorDataPacket &data) {
    float parsedData;
    // SENSOR_ID_BARO uses P24BITUNSIGNED and scaleFactor 0.0078
    DataParser::parseData(data, parsedData, 0.0078f, P24BITUNSIGNED);
    
    _lastData = parsedData;
    push(parsedData);
    setDataAvailFlag();
}

void PressureSensorDriver::setData(SensorLongDataPacket &data) {
    // No utilizado
}

String PressureSensorDriver::toString() {
    return String("Pressure: ") + String(_lastData, 2) + " hPa";
}

void PressureSensorDriver::fifoFlush() {
    _head = 0;
    _tail = 0;
    _count = 0;
    _overflow = false;
    //_totalPushed = 0;
    clearDataAvailFlag();
}

bool PressureSensorDriver::push(const float& data) {
    bool overwritten = false;
    _totalPushed++;

    _fifo[_head] = data;
    _head = (_head + 1) % PRESSURE_FIFO_SIZE;

    if (_count < PRESSURE_FIFO_SIZE) {
        _count++;
    } else {
        _tail = (_tail + 1) % PRESSURE_FIFO_SIZE;
        overwritten = true;
        _overflow = true;
    }
    
    return !overwritten;
}

bool PressureSensorDriver::pop(float& data) {
    if (_count == 0) {
        return false;
    }

    data = _fifo[_tail];
    _tail = (_tail + 1) % PRESSURE_FIFO_SIZE;
    _count--;

    return true;
}

uint16_t PressureSensorDriver::getAvailableCount() const {
    return _count;
}

bool PressureSensorDriver::isFull() const {
    return _count == PRESSURE_FIFO_SIZE;
}

bool PressureSensorDriver::hasOverflowed() const {
    return _overflow;
}

void PressureSensorDriver::clearOverflow() {
    _overflow = false;
}

uint16_t PressureSensorDriver::rewind(uint16_t steps) {
    uint32_t totalValid = (_totalPushed < PRESSURE_FIFO_SIZE) ? _totalPushed : PRESSURE_FIFO_SIZE;
    uint16_t maxRewind = totalValid - _count;
    
    uint16_t actualRewind = (steps > maxRewind) ? maxRewind : steps;
    
    if (actualRewind > 0) {
        _tail = (_tail + PRESSURE_FIFO_SIZE - actualRewind) % PRESSURE_FIFO_SIZE;
        _count += actualRewind;
    }
    
    return actualRewind;
}

const float* PressureSensorDriver::getElementAt(uint16_t index) const {
    if (index >= _count) {
        return nullptr;
    }
    return &_fifo[(_tail + index) % PRESSURE_FIFO_SIZE];
}

uint16_t PressureSensorDriver::getFifoValues(float* buffer, uint16_t maxLen) {
    uint16_t extracted = 0;
    
    while (_count > 0 && extracted < maxLen) {
        pop(buffer[extracted]);
        extracted++;
    }
    
    return extracted;
}

uint32_t PressureSensorDriver::getTotalPushed() const {
    return _totalPushed;
}

void PressureSensorDriver::resetTotalPushed() {
    _totalPushed = 0;
}
