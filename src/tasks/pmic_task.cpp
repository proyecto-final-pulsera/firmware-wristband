#include "tasks/pmic_task.h"
#include "drivers/battery_driver.h"

void PmicTask::init() {
    // Tarea desactivada para ahorrar RAM (stack + TCB de Thread).
    // El ping periodico al PMIC se delega a SystemTask.
    // _thread.start(mbed::callback(this, &PmicTask::run));
}

void PmicTask::run() {
    // Preservado para referencia futura si se reactiva la tarea como hilo independiente:
    /*
    while (true) {
        // Ping al PMIC cada 40 segundos para evitar que el watchdog de 50s se dispare
        BatteryDriver* battery = BatteryDriver::getInstance();

        if (battery != nullptr) {
            battery->ping();
        }
        rtos::ThisThread::sleep_for(std::chrono::milliseconds(40000));
    }
    */
}
