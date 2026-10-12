#include "repositories/pressure_repository.h"
#include "sensors/DataParser.h"
#include <algorithm>
#include <cstring>

PressureRepository* PressureRepository::getInstance() {
    static PressureRepository instance;
    return &instance;
}

PressureRepository::PressureRepository() 
    : SensorClass(SENSOR_ID_BARO), 
      _head(0), _tail(0), _count(0), _overflow(false), _lastData(0.0f), _totalPushed(0)
{
}

PressureRepository::~PressureRepository() {
}

void PressureRepository::setData(SensorDataPacket &data) {
    float parsedData;
    // SENSOR_ID_BARO uses P24BITUNSIGNED and scaleFactor 0.0078
    DataParser::parseData(data, parsedData, 0.0078f, P24BITUNSIGNED);
    
    _lastData = parsedData;
    push(parsedData);
    setDataAvailFlag();
}

void PressureRepository::setData(SensorLongDataPacket &data) {
    // No utilizado
}

String PressureRepository::toString() {
    return String("Pressure: ") + String(_lastData, 2) + " hPa";
}

void PressureRepository::fifoFlush() {
    rtos::ScopedMutexLock lock(_mutex);
    _head = 0;
    _tail = 0;
    _count = 0;
    _overflow = false;
    //_totalPushed = 0;
    clearDataAvailFlag();
}

bool PressureRepository::push(const float& data) {
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

bool PressureRepository::pop(float& data) {
    if (_count == 0) {
        return false;
    }

    data = _fifo[_tail];
    _tail = (_tail + 1) % PRESSURE_FIFO_SIZE;
    _count--;

    return true;
}

uint16_t PressureRepository::getAvailableCount() const {
    return _count;
}

bool PressureRepository::isFull() const {
    return _count == PRESSURE_FIFO_SIZE;
}

bool PressureRepository::hasOverflowed() const {
    return _overflow;
}

void PressureRepository::clearOverflow() {
    _overflow = false;
}

uint16_t PressureRepository::rewind(uint16_t steps) {
    uint32_t totalValid = (_totalPushed < PRESSURE_FIFO_SIZE) ? _totalPushed : PRESSURE_FIFO_SIZE;
    uint16_t maxRewind = totalValid - _count;
    
    uint16_t actualRewind = (steps > maxRewind) ? maxRewind : steps;
    
    if (actualRewind > 0) {
        _tail = (_tail + PRESSURE_FIFO_SIZE - actualRewind) % PRESSURE_FIFO_SIZE;
        _count += actualRewind;
    }
    
    return actualRewind;
}

float PressureRepository::getElementAt(uint16_t index) {
    rtos::ScopedMutexLock lock(_mutex);
    return getElementAtUnprotected(index);
}

float PressureRepository::getElementAtUnprotected(uint16_t index) const {
    if (index >= _count) {
        return 0.0f;
    }
    return _fifo[(_tail + index) % PRESSURE_FIFO_SIZE];
}

uint16_t PressureRepository::getFifoValues(float* buffer, uint16_t maxLen) {
    rtos::ScopedMutexLock lock(_mutex);
    uint16_t extracted = 0;
    
    while (_count > 0 && extracted < maxLen) {
        pop(buffer[extracted]);
        extracted++;
    }
    
    return extracted;
}

const float* PressureRepository::getLinearBuffer(uint16_t* outCount) {
    rtos::ScopedMutexLock lock(_mutex);
    if (_count == 0) {
        if (outCount) *outCount = 0;
        return nullptr;
    }
    if (_tail != 0) {
        if (_count == PRESSURE_FIFO_SIZE) {
            std::rotate(&_fifo[0], &_fifo[_tail], &_fifo[PRESSURE_FIFO_SIZE]);
            _tail = 0;
            _head = 0;
        } else if (_head > _tail) {
            std::memmove(&_fifo[0], &_fifo[_tail], _count * sizeof(float));
            _tail = 0;
            _head = _count;
        } else {
            std::rotate(&_fifo[0], &_fifo[_tail], &_fifo[PRESSURE_FIFO_SIZE]);
            _tail = 0;
            _head = _count % PRESSURE_FIFO_SIZE;
        }
    }
    if (outCount) *outCount = _count;
    return _fifo;
}

uint16_t PressureRepository::copyFifoValues(float* buffer, uint16_t maxLen) {
    rtos::ScopedMutexLock lock(_mutex);
    return copyFifoValuesUnprotected(buffer, maxLen, 0);
}

uint16_t PressureRepository::copyFifoValues(float* buffer, uint16_t maxLen, uint16_t index) {
    rtos::ScopedMutexLock lock(_mutex);
    return copyFifoValuesUnprotected(buffer, maxLen, index);
}

uint16_t PressureRepository::copyFifoValuesUnprotected(float* buffer, uint16_t maxLen, uint16_t index) const {
    if (index >= _count) {
        return 0;
    }

    uint16_t elementsToCopy = _count - index;
    if (elementsToCopy > maxLen) {
        elementsToCopy = maxLen;
    }
    
    for (uint16_t i = 0; i < elementsToCopy; i++) {
        buffer[i] = _fifo[(_tail + index + i) % PRESSURE_FIFO_SIZE];
    }
    
    return elementsToCopy;
}

uint32_t PressureRepository::getTotalPushed() const {
    return _totalPushed;
}

void PressureRepository::resetTotalPushed() {
    _totalPushed = 0;
}
