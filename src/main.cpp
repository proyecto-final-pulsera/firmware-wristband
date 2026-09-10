#include <Arduino.h>
#include "drivers/battery_driver.h"

// Para acceder a algunos métodos extra en el test de manera temporal
#include "Nicla_System.h"

BatteryDriver* battery;

// Variables para manejar el buffer de mensajes cuando no hay puerto serie
String logBuffer = "";
bool wasSerialConnected = false;

void setup() {
  Serial.begin(115200);

  // Instanciar e inicializar el driver de batería
  battery = BatteryDriver::createInstance();

}

String getOperatingStatusStr(OperatingStatus status) {
  switch (status) {
    case OperatingStatus::Ready: return "Listo (No cargando)";
    case OperatingStatus::Charging: return "Cargando...";
    case OperatingStatus::ChargingComplete: return "Carga Completa";
    case OperatingStatus::Error: return "¡Error en bateria!";
    default: return "Desconocido";
  }
}

void loop() {


  // Testeo de Batería cada 2 segundos
  static unsigned long lastTest = 0;
  if (millis() - lastTest >= 2000) {
    lastTest = millis();
    battery->ping();
    Serial.println("sigo vivo");
  }
}