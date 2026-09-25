#include <Arduino.h>
#include "app/app.h"
#include "drivers/battery_driver.h"

// Hilo dedicado a mantener vivo el PMIC enviandole Pings por I2C
rtos::Thread batteryThread(osPriorityNormal, 1024);

void batteryPingTask() {
    BatteryDriver* battery = BatteryDriver::getInstance();
    while (true) {
        if (battery != nullptr) {
            battery->ping();
        }
        // Ping cada 2 segundos (Mbed OS 6 safe)
        rtos::ThisThread::sleep_for(std::chrono::milliseconds(2000));
    }
}

void setup() {
    // Inicializar puerto serie para ver los prints de prueba
    Serial.begin(115200);
    // Esperar a que conectes el Serial Monitor, podes comentarlo si usas bateria
    // while (!Serial) { rtos::ThisThread::sleep_for(std::chrono::milliseconds(10)); }

    // 1. Inicializar toda la arquitectura de la app (Drivers, Tareas, ISRs)
    App::init();

    // 2. Iniciar el hilo del PMIC
    // Nota: Como la App ya llamo a BatteryDriver::createInstance(), 
    // el getInstance() dentro de la tarea funcionara correctamente.
    batteryThread.start(batteryPingTask);
}

void loop() {
    // El RTOS ya tomo el control mediante los threads.
    // Dejamos el loop vacio cediendo el tiempo al Scheduler.
    rtos::ThisThread::sleep_for(std::chrono::milliseconds(1000));
}