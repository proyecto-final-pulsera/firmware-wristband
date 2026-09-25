#include "tasks/comm_link_task.h"
#include "mbed.h"

void CommLinkTask::init() {
    // Inicializar hardware, configuraciones previas al inicio, etc.
    
    // Iniciar el hilo asociado al metodo run de esta instancia
    _thread.start(mbed::callback(this, &CommLinkTask::run));
}

bool CommLinkTask::sendMsg(AppMessage* msg) {
    return _comm_link_task_queue.send(msg);
}

void CommLinkTask::run() {
    AppMessage msg;
    while (true) {
        // Esperamos un mensaje por siempre usando el Wrapper
        if (_comm_link_task_queue.receive(&msg)) {
            // --- PROCESAR EL MENSAJE AQUI ---
            
            // --- FIN PROCESAMIENTO ---
        }
    }
}
