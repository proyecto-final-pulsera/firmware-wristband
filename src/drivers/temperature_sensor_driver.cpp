#include "drivers/temperature_sensor_driver.h"
#include "sensors/DataParser.h"

TemperatureSensorDriver::TemperatureSensorDriver()
    : SensorClass(SENSOR_ID_TEMP), 
      _lastTemp(0.0f), 
      _lastUpdateMillis(0), 
      _isUpdated(false)
{
}

TemperatureSensorDriver::~TemperatureSensorDriver() {
}

void TemperatureSensorDriver::setData(SensorDataPacket &data) {
    float parsedData;
    // SENSOR_ID_TEMP utiliza P16BITSIGNED y factor de escala 0.01
    DataParser::parseData(data, parsedData, 0.01f, P16BITSIGNED);
    
    _lastTemp = parsedData; // El valor ya resulta en Celsius
    _lastUpdateMillis = millis();
    _isUpdated = true;
    
    // Indicamos al sistema base que hay un dato nuevo
    setDataAvailFlag();
}

void TemperatureSensorDriver::setData(SensorLongDataPacket &data) {
    // No utilizado por el sensor de temperatura
}

String TemperatureSensorDriver::toString() {
    return String("Temperature: ") + String(_lastTemp, 2) + " C";
}

bool TemperatureSensorDriver::isUpdated() const {
    return _isUpdated;
}

float TemperatureSensorDriver::getTemp() {
    _isUpdated = false;
    clearDataAvailFlag(); // Limpiamos también el flag heredado
    return _lastTemp;
}

uint32_t TemperatureSensorDriver::getLastUpdateMillis() const {
    return _lastUpdateMillis;
}
