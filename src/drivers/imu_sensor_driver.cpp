#include "drivers/imu_sensor_driver.h"

ImuSensorDriver::ImuSensorDriver() 
    : SensorClass(SENSOR_ID_ACC_PASS), 
      _head(0), _tail(0), _count(0), _overflow(false), _totalPushed(0)
{
    // Constructor llama al padre SensorClass pasándole el ID de Acelerómetro
}

ImuSensorDriver::~ImuSensorDriver() {
    // Destructor
}

void ImuSensorDriver::setData(SensorDataPacket &data) {
    // Parseamos el paquete nativo a un DataXYZ
    DataXYZ parsedData;
    DataParser::parse3DVector(data, parsedData);
    
    // Guardamos el último dato para toString
    _lastData = parsedData;

    // Lo agregamos a la FIFO
    push(parsedData);
    
    // Seteamos el flag de SensorClass para que el sistema sepa que hay data
    setDataAvailFlag();
}

void ImuSensorDriver::setData(SensorLongDataPacket &data) {
    // No utilizado para este tipo de sensor (Acelerómetro usa paquetes cortos)
}

String ImuSensorDriver::toString() {
    return _lastData.toString();
}

void ImuSensorDriver::fifoFlush() {
    _head = 0;
    _tail = 0;
    _count = 0;
    _overflow = false;
    // _totalPushed = 0;
    clearDataAvailFlag();
}

bool ImuSensorDriver::push(const DataXYZ& data) {
    bool overwritten = false;
    
    _totalPushed++; // Contamos estadísticamente cuántos ingresaron

    _fifo[_head] = data;
    _head = (_head + 1) % IMU_FIFO_SIZE;

    if (_count < IMU_FIFO_SIZE) {
        _count++;
    } else {
        // FIFO llena: se sobrescribe el dato más viejo. Avanzamos el tail.
        _tail = (_tail + 1) % IMU_FIFO_SIZE;
        overwritten = true;
        _overflow = true;
    }
    
    return !overwritten;
}

bool ImuSensorDriver::pop(DataXYZ& data) {
    if (_count == 0) {
        return false; // FIFO vacía
    }

    data = _fifo[_tail];
    _tail = (_tail + 1) % IMU_FIFO_SIZE;
    _count--;

    return true;
}

uint16_t ImuSensorDriver::getAvailableCount() const {
    return _count;
}

bool ImuSensorDriver::isFull() const {
    return _count == IMU_FIFO_SIZE;
}

bool ImuSensorDriver::hasOverflowed() const {
    return _overflow;
}

void ImuSensorDriver::clearOverflow() {
    _overflow = false;
}

uint16_t ImuSensorDriver::rewind(uint16_t steps) {
    // Calculamos el historial máximo físicamente presente en la memoria
    uint32_t totalValid = (_totalPushed < IMU_FIFO_SIZE) ? _totalPushed : IMU_FIFO_SIZE;
    uint16_t maxRewind = totalValid - _count;
    
    // Evitamos retroceder más allá de los datos válidos existentes
    uint16_t actualRewind = (steps > maxRewind) ? maxRewind : steps;
    
    if (actualRewind > 0) {
        // Retrocedemos el tail lógicamente (sumando el max size antes de restar para evitar underflow)
        _tail = (_tail + IMU_FIFO_SIZE - actualRewind) % IMU_FIFO_SIZE;
        _count += actualRewind;
    }
    
    return actualRewind;
}

const DataXYZ* ImuSensorDriver::getElementAt(uint16_t index) const {
    if (index >= _count) {
        return nullptr;
    }
    return &_fifo[(_tail + index) % IMU_FIFO_SIZE];
}

uint16_t ImuSensorDriver::getFifoValues(DataXYZ* buffer, uint16_t maxLen) {
    uint16_t extracted = 0;
    
    // Sacamos datos hasta vaciar la FIFO o llenar el buffer provisto
    while (_count > 0 && extracted < maxLen) {
        pop(buffer[extracted]);
        extracted++;
    }
    
    return extracted;
}

uint32_t ImuSensorDriver::getTotalPushed() const {
    return _totalPushed;
}

void ImuSensorDriver::resetTotalPushed() {
    _totalPushed = 0;
}
