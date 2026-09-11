#include "drivers/imu_sensor_driver.h"

ImuSensorDriver::ImuSensorDriver() 
    : SensorClass(SENSOR_ID_ACCEL), 
      _head(0), _tail(0), _count(0), _totalPushed(0)
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
