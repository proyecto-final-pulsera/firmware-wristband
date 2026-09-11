#pragma once
#include <Arduino.h>

class SerialMenuDebug {
public:
    SerialMenuDebug();
    
    // Incrementa el contador de un evento, o lo registra si es nuevo
    void logEvent(const String& eventName);
    
    // Debe llamarse en un loop para leer la consola serial
    // Si recibe "EVENTOS", imprime el conteo de eventos registrados
    void processSerial();
    
    // Resetea todos los contadores
    void clear();

private:
    struct EventRecord {
        String name;
        uint32_t count;
    };
    
    static const int MAX_EVENTS = 10;
    EventRecord _events[MAX_EVENTS];
    int _eventCount;
    
    void printEvents();
};
