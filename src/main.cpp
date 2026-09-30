#include <Arduino.h>
#include "app/app.h"
#include "drivers/battery_driver.h"
#include "rtos.h"
#include "../test/test_sensors_drivers.h"

// Hilo dedicado a mantener vivo el PMIC
rtos::Thread batteryThread(osPriorityNormal, 1024);

void batteryPingTask() {
    BatteryDriver* battery = BatteryDriver::getInstance();
    while (true) {
        if (battery != nullptr) {
            battery->ping();
        }
        rtos::ThisThread::sleep_for(std::chrono::milliseconds(2000));
    }
}

void setup() {
    Serial.begin(115200);
    // Esperar a que se estabilice el puerto (opcional)
    delay(2000);
    
    // 1. Inicializar toda la arquitectura de la app (Drivers, Tareas, ISRs)
    App::init();

    // 2. Iniciar el hilo del PMIC
    // BatteryDriver* battery = BatteryDriver::getInstance();
    // battery->init();
    batteryThread.start(batteryPingTask);
    // runSensorsDriversTest();
}

void loop() {
 
    rtos::ThisThread::sleep_for(std::chrono::milliseconds(50));
}
