#include "drivers/bhi260_driver.h"
#include "Nicla_System.h"

BHI260Driver* BHI260Driver::_instance = nullptr;

BHI260Driver* BHI260Driver::getInstance() {
    if (_instance == nullptr) {
        _instance = new BHI260Driver();
    }
    return _instance;
}

BHI260Driver::BHI260Driver() {

    // We will initialize it here or leave it for later as requested.
    
    _interruptPin = INT_BHI260; // Use Nicla's default BHI260 interrupt pin
    _isr_handler = nullptr;
    _interruptTriggered = false;
}

BHI260Driver::~BHI260Driver() {
    // Destructor implementation
}

extern BoschSensortec sensortec;

bool BHI260Driver::init() {
    // Initialize Nicla I2C/power if not already
    nicla::begin();
    nicla::enable3V3LDO();

    // Call the global sensortec to initialize the BHI260 hardware
    // This allows native Arduino_BHY2 classes like SensorXYZ to work
    return sensortec.begin();
}

void BHI260Driver::updateFifoData() {
    bhy2_get_and_process_fifo(sensortec._workBuffer, WORK_BUFFER_SIZE, &sensortec._bhy2);
}

void BHI260Driver::configureSensor(SensorConfigurationPacket& config) {
    sensortec.configureSensor(config);
}

void BHI260Driver::flushFIFOs() {
    // Lee la FIFO internamente
    updateFifoData();
    // Descarta todos los paquetes decodificados
    while (availableSensorData()) {
        SensorDataPacket dummy;
        readSensorData(dummy);
    }
    // Aseguramos que la flag quede limpia para el próximo evento real
    clearInterruptFlag();
}

void BHI260Driver::suspendHost() {
    uint8_t stat;
    // Leer el estado actual del registro
    bhy2_get_host_intf_ctrl(&stat, &sensortec._bhy2);
    // Setear el bit de AP Suspended (bloquea la FIFO Non-Wakeup del pin físico)
    stat |= BHY2_HIF_CTRL_AP_SUSPENDED;
    bhy2_set_host_intf_ctrl(stat, &sensortec._bhy2);
}

void BHI260Driver::resumeHost() {
    uint8_t stat;
    // Leer el estado actual del registro
    bhy2_get_host_intf_ctrl(&stat, &sensortec._bhy2);
    // Limpiar el bit de AP Suspended (desbloquea la FIFO Non-Wakeup)
    stat &= ~BHY2_HIF_CTRL_AP_SUSPENDED;
    bhy2_set_host_intf_ctrl(stat, &sensortec._bhy2);
    // IMPORTANTE: El BHI260 requiere de unos 10ms aprox. para reconfigurar 
    // sus flags internos y exponer de vuelta la Non-Wakeup FIFO luego de despertar.
    delay(10);
}

bool BHI260Driver::hasSensor(uint8_t sensorId) {
    return sensortec.hasSensor(sensorId);
}

uint8_t BHI260Driver::availableSensorData() {
    return sensortec.availableSensorData();
}

uint8_t BHI260Driver::availableLongSensorData() {
    return sensortec.availableLongSensorData();
}

bool BHI260Driver::readSensorData(SensorDataPacket &data) {
    return sensortec.readSensorData(data);
}

bool BHI260Driver::readLongSensorData(SensorLongDataPacket &data) {
    return sensortec.readLongSensorData(data);
}

void BHI260Driver::setInterruptPin(uint32_t pin) {
    _interruptPin = pin;
}

uint32_t BHI260Driver::getInterruptPin() const {
    return _interruptPin;
}

void BHI260Driver::configureInterrupt(void (*isr_handler)(void)) {
    if (_interruptPin != 0xFFFFFFFF) {
        _isr_handler = isr_handler;
        pinMode(_interruptPin, INPUT);
        attachInterrupt(digitalPinToInterrupt(_interruptPin), _isr_handler, RISING);
    }
}

void BHI260Driver::enableInterrupt() {
    if (_interruptPin != 0xFFFFFFFF && _isr_handler != nullptr) {
        attachInterrupt(digitalPinToInterrupt(_interruptPin), _isr_handler, RISING);
    }
}

void BHI260Driver::disableInterrupt() {
    if (_interruptPin != 0xFFFFFFFF) {
        detachInterrupt(digitalPinToInterrupt(_interruptPin));
    }
}

void BHI260Driver::handleInterrupt() {
    _interruptTriggered = true;
}

void BHI260Driver::updateWakeupFIFO() {
    // To be implemented: process only the wakeup FIFO
}

void BHI260Driver::updateNonWakeupFIFO() {
    // To be implemented: process only the non-wakeup FIFO
}

bool BHI260Driver::isInterruptTriggered() const {
    return _interruptTriggered;
}

void BHI260Driver::clearInterruptFlag() {
    _interruptTriggered = false;
}
