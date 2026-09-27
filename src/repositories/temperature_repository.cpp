#include "repositories/temperature_repository.h"
#include "sensors/DataParser.h"

TemperatureRepository* TemperatureRepository::_instance = nullptr;

TemperatureRepository* TemperatureRepository::createInstance() {
    if (_instance == nullptr) {
        _instance = new TemperatureRepository();
    }
    return _instance;
}

TemperatureRepository* TemperatureRepository::getInstance() {
    return createInstance();
}

TemperatureRepository::TemperatureRepository()
    : SensorClass(SENSOR_ID_TEMP), 
      _lastTemp(0.0f), 
      _lastUpdateMillis(0), 
      _isUpdated(false)
{
}

TemperatureRepository::~TemperatureRepository() {
}

void TemperatureRepository::setData(SensorDataPacket &data) {
    float parsedData;
    // SENSOR_ID_TEMP utiliza P16BITSIGNED y factor de escala 0.01
    DataParser::parseData(data, parsedData, 0.01f, P16BITSIGNED);
    
    _lastTemp = parsedData; // El valor ya resulta en Celsius
    _lastUpdateMillis = millis();
    _isUpdated = true;
    
    // Indicamos al sistema base que hay un dato nuevo
    setDataAvailFlag();
}

void TemperatureRepository::setData(SensorLongDataPacket &data) {
    // No utilizado por el sensor de temperatura
}

String TemperatureRepository::toString() {
    return String("Temperature: ") + String(_lastTemp, 2) + " C";
}

bool TemperatureRepository::isUpdated() const {
    return _isUpdated;
}

float TemperatureRepository::getTemp() {
    _isUpdated = false;
    clearDataAvailFlag(); // Limpiamos también el flag heredado
    return _lastTemp;
}

uint32_t TemperatureRepository::getLastUpdateMillis() const {
    return _lastUpdateMillis;
}
