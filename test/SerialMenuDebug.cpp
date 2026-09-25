#include "SerialMenuDebug.h"
#include "drivers/vibrator_driver.h"

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
        Serial.println("[DEBUG] SerialMenuDebug: Buffer de eventos lleno!");
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
            Serial.println("[DEBUG] Vibrador habilitado.");
        } else if (cmd == "VIBRATOR_DES") {
            VibratorDriver::getInstance()->disable();
            Serial.println("[DEBUG] Vibrador deshabilitado.");
        } else if (cmd == "VIBRATOR_BREAK") {
            VibratorDriver::getInstance()->brake();
            Serial.println("[DEBUG] Vibrador deshabilitado.");
        
        } else if (cmd.startsWith("VIBRATOR_DUTY_")) {
            String valueStr = cmd.substring(14);
            int value = valueStr.toInt();
            VibratorDriver::getInstance()->setStrength((uint8_t)value);
            Serial.print("[DEBUG] VEVibrador strength seteado a: ");
            Serial.println(value);
        } else {
            Serial.print("[DEBUG] Comando desconocido: '");
            Serial.print(cmd);
            Serial.println("'");
            Serial.println("[DEBUG] Escriba EVENTOS para ver las metricas.");
        }

        // Vaciamos el buffer de recepción (RX) descartando cualquier caracter sobrante (basura o múltiples enters)
        while (Serial.available() > 0) {
            Serial.read();
        }
    }
}

void SerialMenuDebug::printEvents() {
    Serial.println("\n=== METRICAS DE EVENTOS ===");
    if (_eventCount == 0) {
        Serial.println("No se registraron eventos aun.");
    } else {
        for (int i = 0; i < _eventCount; i++) {
            Serial.print("- ");
            Serial.print(_events[i].name);
            Serial.print(": ");
            Serial.println(_events[i].count);
        }
    }
    Serial.println("===========================\n");
}

void SerialMenuDebug::clear() {
    _eventCount = 0;
    for (int i = 0; i < MAX_EVENTS; i++) {
        _events[i].name = "";
        _events[i].count = 0;
    }
}
