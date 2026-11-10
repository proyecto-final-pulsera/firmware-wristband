#include "SerialMenuDebug.h"
#include "drivers/vibrator_driver.h"
#include "utils/debug.h"

SerialMenuDebug::SerialMenuDebug() {

    clear();
}

void SerialMenuDebug::logEvent(const String& eventName) {
    for (int i = 0; i < _eventCount; i++) {
        if (_events[i].name == eventName) {
            _events[i].count++;
            return;
        }
    }
    
    if (_eventCount < MAX_EVENTS) {
        _events[_eventCount].name = eventName;
        _events[_eventCount].count = 1;
        _eventCount++;
    } else {
        DEBUG_PRINTLN("[DEBUG] SerialMenuDebug: Buffer de eventos lleno!");
    }
}

void SerialMenuDebug::processSerial() {
    if (Serial.available() > 0) {
        String cmd = Serial.readStringUntil('\n');
        cmd.trim();
        cmd.toUpperCase();
        
        if (cmd == "EVENTOS") {
            printEvents();
        } else if (cmd == "VIBRATOR_ENA") {
            VibratorDriver::getInstance()->enable();
            DEBUG_PRINTLN("[DEBUG] Vibrador habilitado.");
        } else if (cmd == "VIBRATOR_DES") {
            VibratorDriver::getInstance()->disable();
            DEBUG_PRINTLN("[DEBUG] Vibrador deshabilitado.");
        } else if (cmd == "VIBRATOR_BREAK") {
            VibratorDriver::getInstance()->brake();
            DEBUG_PRINTLN("[DEBUG] Vibrador deshabilitado.");
        
        } else if (cmd.startsWith("VIBRATOR_DUTY_")) {
            String valueStr = cmd.substring(14);
            int value = valueStr.toInt();
            VibratorDriver::getInstance()->setStrength((uint8_t)value);
            DEBUG_PRINT("[DEBUG] VEVibrador strength seteado a: ");
            DEBUG_PRINTLN(value);
        } else {
            DEBUG_PRINT("[DEBUG] Comando desconocido: '");
            DEBUG_PRINT(cmd);
            DEBUG_PRINTLN("'");
            DEBUG_PRINTLN("[DEBUG] Escriba EVENTOS para ver las metricas.");
        }

        // Vaciamos el buffer de recepción (RX) descartando cualquier caracter sobrante (basura o múltiples enters)
        while (Serial.available() > 0) {
            Serial.read();
        }
    }
}

void SerialMenuDebug::printEvents() {
    DEBUG_PRINTLN("\n=== METRICAS DE EVENTOS ===");
    if (_eventCount == 0) {
        DEBUG_PRINTLN("No se registraron eventos aun.");
    } else {
        for (int i = 0; i < _eventCount; i++) {
            DEBUG_PRINT("- ");
            DEBUG_PRINT(_events[i].name);
            DEBUG_PRINT(": ");
            DEBUG_PRINTLN(_events[i].count);
        }
    }
    DEBUG_PRINTLN("===========================\n");
}

void SerialMenuDebug::clear() {
    _eventCount = 0;
    for (int i = 0; i < MAX_EVENTS; i++) {
        _events[i].name = "";
        _events[i].count = 0;
    }
}
