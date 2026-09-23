#pragma once
#include <Arduino.h>
#include "rtos.h"
#include <stdint.h>

// =============================================================================
// Estructuras de Protocolo (Identificadores de Trama)
// =============================================================================

enum MsgType : uint8_t {
    MSG_ACK             = 0x00,
    MSG_IMU_BUFFER      = 0x01, // ENVIAR_BUFF_IMU
    MSG_PRESSURE_BUFFER = 0x02, // ENVIAR_BUFF_PREASSURE
    MSG_TEMPERATURE     = 0x03, // ENVIAR_TEMP
    MSG_ALARM           = 0x04, // ENVIAR_ALARMA
    MSG_METRICS         = 0x05  // ENVIAR_METRICAS
};

// =============================================================================
// Estructura de Salida para la Aplicación
// =============================================================================
struct AppMessage {
    MsgType type;
    uint8_t payload[128]; // Tamaño máximo estimado para un paquete
    uint16_t length;
};

// =============================================================================
// Clase Base Abstracta (CommDriver)
// =============================================================================

class CommDriver {
protected:
    // Puntero a la función inyectada por la Capa de Aplicación (RTOS)
    void (*_rxNotificationCallback)() = nullptr;

    // Función que será llamada por las clases hijas desde sus pseudo-ISR
    void notifyRxFromISR() {
        if (_rxNotificationCallback != nullptr) {
            _rxNotificationCallback();
        }
    }

    // -------------------------------------------------------------------------
    // Utilidades internas (Disponibles para todos los hijos)
    // -------------------------------------------------------------------------
    
    // Dummy CRC16 (A implementar algoritmo real luego)
    uint16_t calculateCRC(const uint8_t* data, uint16_t len) {
        uint16_t crc = 0xFFFF;
        // Lógica CRC16...
        return crc; 
    }

public:
    virtual ~CommDriver() = default;

    // -------------------------------------------------------------------------
    // Métodos Virtuales Puros (Contrato estricto para los hijos)
    // -------------------------------------------------------------------------
    
    virtual void begin() = 0;
    
    // Engancha el Callback/ISR al hardware físico
    virtual void attachRxInterrupt() = 0;
    
    // Lee el buffer de hardware y decodifica la trama. 
    // Devuelve true si armó una trama completa válida.
    virtual bool processRxData(AppMessage* out_msg) = 0; 
    
    // Envía un payload construyendo la trama en formato binario
    virtual bool sendPayload(MsgType type, const uint8_t* payload, uint16_t len) = 0;

    // -------------------------------------------------------------------------
    // Inyección de Dependencias (Inter-Task)
    // -------------------------------------------------------------------------
    
    // Vincula la ISR física con el generador de eventos del RTOS
    void attachRxNotifier(void (*callback)()) {
        _rxNotificationCallback = callback;
    }
};
