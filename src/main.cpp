#include <Arduino.h>

#include "test_serial_data.h"
#include "drivers/battery_driver.h"

BatteryDriver* battery = nullptr;
rtos::Thread batteryThread(osPriorityNormal, 2048);

void batteryPingTask() {
    while (true) {
        if (battery != nullptr) {
            battery->ping();
        }
        rtos::ThisThread::sleep_for(2000); // Ping cada 2 segundos
    }
}

void setup() {
    // Inicializar el controlador de batería y lanzar su hilo de ping
    // Esto evita que el PMIC BQ25120 apague la placa
    battery = BatteryDriver::createInstance();
    batteryThread.start(batteryPingTask);

    // Todo el código de prueba fue encapsulado en la carpeta test/
    runSerialDataTest();
}

void loop() {
    // El test corre en un while(1) infinito, por lo que nunca se llega acá.
}