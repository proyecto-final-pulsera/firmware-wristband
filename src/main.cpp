#include <Arduino.h>
#include "app/app.h"
#include "drivers/battery_driver.h"
#include "rtos.h"

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
    // BatteryDriver* battery = BatteryDriver::getInstance();
    // battery->init();
    
    // 1. Inicializar toda la arquitectura de la app (Drivers, Tareas, ISRs)
    App::init();


    // 2. Iniciar el hilo del PMIC
    batteryThread.start(batteryPingTask);
}


void loop() {
    // El RTOS ya tomo el control mediante los threads.
    // Dejamos el loop vacio cediendo el tiempo al Scheduler.
    
    rtos::ThisThread::sleep_for(std::chrono::milliseconds(100));
}
