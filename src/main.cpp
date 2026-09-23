#include <Arduino.h>
#include "drivers/battery_driver.h"
#include "drivers/bhi260_driver.h"
#include "drivers/vibrator_driver.h"
#include "Nicla_System.h"
#include "rtos.h"
#include "test_eventos_imu.h"
#include "test_virtual_sensors.h"
#include "../test/test_sensors_drivers.h"
BatteryDriver* battery;
BHI260Driver* imu;

// Hilo dedicado para hacer ping a la batería
rtos::Thread batteryThread(osPriorityNormal, 2048);

void batteryPingTask() {
  while (true) {
    if (battery != nullptr) {
      battery->ping();
    }
    rtos::ThisThread::sleep_for(2000);
  }
}

void setup() {
  Serial.begin(115200);

  // Damos tiempo a que se estabilice la conexión
  delay(2000);
  
  // Limpiamos cualquier basura (ruido eléctrico o mensajes de booteo)
  // que haya quedado atascada en el buffer antes de arrancar.
  while(Serial.available() > 0) {
      Serial.read();
  }

  // Instanciar e inicializar el driver de batería
  battery = BatteryDriver::createInstance();
  batteryThread.start(batteryPingTask);

  // Inicializar nuestro driver IMU
  imu = BHI260Driver::getInstance();
  
  imu->init();
  imu->disableNonWakeupFIFO();
  // Vaciamos FIFOs iniciales para garantizar RISING pin edge
  imu->flushFIFOs();

  // Instanciamos el driver del vibrador para que el pin Enable arranque en LOW
  VibratorDriver::createInstance();
  
  // Ejecutamos el test multi-sensor
  runSensorsDriversTest();
}

void loop() {
  // Lógica principal de tu aplicación (vacía por el momento mientras estamos en test)
  rtos::ThisThread::sleep_for(100);
}