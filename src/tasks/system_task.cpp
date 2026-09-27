#include "tasks/system_task.h"
#include "mbed.h"
#include "tasks/comm_link_task.h"
#include "drivers/bhi260_driver.h"
#include "repositories/event_repository.h"
#include "tasks/alarms_events_task.h"

void SystemTask::init() {
    // Inicializar hardware, configuraciones previas al inicio, etc.
    
    // Iniciar el hilo asociado al metodo run de esta instancia
    _thread.start(mbed::callback(this, &SystemTask::run));
}

bool SystemTask::sendMsg(AppMessage* msg) {
    return _system_task_queue.send(msg);
}

enum TestState {
    WAIT_MOTION,
    WAIT_STILL
};

void SystemTask::run() {
    AppMessage msg;
    
    // /* TODO: A usar en el futuro próximo
    // while (true) {
    //     // Esperamos un mensaje por siempre usando el Wrapper
    //     if (_system_task_queue.receive(&msg)) {
    //         // --- PROCESAR EL MENSAJE AQUI ---
    //         
    //         // --- FIN PROCESAMIENTO ---
    //     }
    // }
    // */

    BHI260Driver* bhi = BHI260Driver::getInstance();
    
    Serial.println("Tarea Sys iniciada - MODO TEST ALARMS EVENTS");
    while (true) {
        // Dormir la aplicacion por 3 segundos
        rtos::ThisThread::sleep_for(std::chrono::milliseconds(3000));

        // Pedir los datos de la FIFO del sensor
        Serial.println("[SystemTask] Drenando FIFO del sensor BHI260...");
        bhi->updateFifoData();

        // Notificar a la aplicacion de procesamiento
        Serial.println("[SystemTask] Enviando CMD_PROCESS_IMU a AlarmsEventsTask...");
        msg.event_id = AlarmsEventsTask::CMD_PROCESS_IMU;
        msg.emisor_id = TASK_SYSTEM;
        msg.priority_level = PRIORITY_NORMAL;
        AlarmsEventsTask::getInstance().sendMsg(&msg);
    }
}

