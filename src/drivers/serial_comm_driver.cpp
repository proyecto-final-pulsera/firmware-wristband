#include "drivers/serial_comm_driver.h"

SerialCommDriver* SerialCommDriver::_instance = nullptr;

SerialCommDriver* SerialCommDriver::getInstance() {
    if (_instance == nullptr) {
        _instance = createInstance();
    }
    return _instance;
}

SerialCommDriver* SerialCommDriver::createInstance() {
    return new SerialCommDriver();
}

void SerialCommDriver::init() {
    // Se asume que Serial.begin(115200) puede ser llamado aquí o en el main
    // para mantener la compatibilidad con el resto del sistema.
    Serial.begin(115200);
}

void SerialCommDriver::onSerialRx() {
    if (_instance != nullptr) {
        _instance->notifyRxFromISR();
    }
}

void SerialCommDriver::attachRxInterrupt() {
    /* 
     * DECISION ARQUITECTONICA:
     * Para mantener la compatibilidad con Serial.print() en el resto del sistema
     * (uso para debug), se utiliza el objeto Serial estándar de Arduino (USB CDC).
     * El objeto Serial estándar no expone un método directo para atachear ISR de RX.
     * 
     * ACCION FUTURA:
     * Cuando se requiera implementar la recepción real de comandos:
     * 1. Migrar a un puerto físico (Serial1) y usar mbed::UARTSerial con sigio().
     * 2. O bien, implementar un polling de Serial.available() desde la tarea.
     */
}

bool SerialCommDriver::processRxData(CommPacket* out_msg) {
    // La recepción está deshabilitada temporalmente por la decisión arquitectónica.
    // Se purga el buffer para evitar overflows si llegan datos basura.
    while (Serial.available() > 0) {
        Serial.read();
    }
    return false;
}



bool SerialCommDriver::sendPayload(MsgType type, const uint8_t* payload, uint16_t len) {
    const uint8_t START_BYTE = 0xAA;
    
    // 1. Calcular CRC del Header (START_BYTE + Type + Len)
    uint8_t header_crc = calculateCRC8(&START_BYTE, 1);
    header_crc = calculateCRC8((uint8_t*)&type, 1, header_crc);
    header_crc = calculateCRC8((uint8_t*)&len, 2, header_crc);
    
    // 2. Calcular CRC del Payload (solo si existe)
    uint8_t payload_crc = 0;
    if (payload != nullptr && len > 0) {
        payload_crc = calculateCRC8(payload, len);
    }
    
    // Enviar Header + Header CRC
    Serial.write(&START_BYTE, 1);
    Serial.write((uint8_t*)&type, 1);
    Serial.write((uint8_t*)&len, 2);
    Serial.write(&header_crc, 1);
    
    // Enviar Payload + Payload CRC (si hay payload)
    if (payload != nullptr && len > 0) {
        Serial.write(payload, len);    
        Serial.write(&payload_crc, 1);
    }
    
    return true;
}
