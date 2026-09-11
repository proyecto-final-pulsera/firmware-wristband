#include <Arduino.h>
#include "drivers/battery_driver.h"
#include "drivers/bhi260_driver.h"
#include "Nicla_System.h"
#include "rtos.h"
#include "test_eventos_imu.h"

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

  // Instanciar e inicializar el driver de batería
  battery = BatteryDriver::createInstance();
  batteryThread.start(batteryPingTask);

  // Inicializar nuestro driver IMU
  imu = BHI260Driver::getInstance();
  
  imu->init();
  
  // Vaciamos FIFOs iniciales para garantizar RISING pin edge
  imu->flushFIFOs();

  // Ejecutamos el test de los eventos
  runTestEventosImu();
}

void loop() {
  // Lógica principal de tu aplicación (vacía por el momento mientras estamos en test)
  rtos::ThisThread::sleep_for('1s');
}