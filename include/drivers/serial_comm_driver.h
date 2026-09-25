#pragma once
#include "drivers/comm_driver.h"
#include <Arduino.h>

class SerialCommDriver : public CommDriver {
private:
    static SerialCommDriver* _instance;
    static SerialCommDriver* createInstance();
    
    SerialCommDriver() = default;

    static void onSerialRx();

    // Buffers internos para ensamblar la trama
    uint8_t _rxBuffer[256];
    uint16_t _rxIndex = 0;

public:
    static SerialCommDriver* getInstance();

    // Prevenimos copias
    SerialCommDriver(const SerialCommDriver&) = delete;
    SerialCommDriver& operator=(const SerialCommDriver&) = delete;

    // -------------------------------------------------------------------------
    // Implementación del Contrato Base
    // -------------------------------------------------------------------------
    void init() override;
    void attachRxInterrupt() override;
    bool processRxData(CommPacket* out_msg) override;
    bool sendPayload(MsgType type, const uint8_t* payload, uint16_t len) override;
};
