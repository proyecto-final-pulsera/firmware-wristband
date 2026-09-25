#include "tasks/system_task.h"
#include "mbed.h"

void SystemTask::init() {
    // Inicializar hardware, configuraciones previas al inicio, etc.
    
    // Iniciar el hilo asociado al metodo run de esta instancia
    _thread.start(mbed::callback(this, &SystemTask::run));
}

bool SystemTask::sendMsg(AppMessage* msg) {
    return _system_task_queue.send(msg);
}

void SystemTask::run() {
    AppMessage msg;
    while (true) {
        // Esperamos un mensaje por siempre usando el Wrapper
        if (_system_task_queue.receive(&msg)) {
            // --- PROCESAR EL MENSAJE AQUI ---
            
            // --- FIN PROCESAMIENTO ---
        }
    }
}