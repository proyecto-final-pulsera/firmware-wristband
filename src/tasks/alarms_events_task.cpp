#include "tasks/alarms_events_task.h"
#include "mbed.h"

void AlarmsEventsTask::init() {
    // Inicializar hardware, configuraciones previas al inicio, etc.
    
    // Iniciar el hilo asociado al metodo run de esta instancia
    _thread.start(mbed::callback(this, &AlarmsEventsTask::run));
}

bool AlarmsEventsTask::sendMsg(AppMessage* msg) {
    return _alarms_events_task_queue.send(msg);
}

void AlarmsEventsTask::run() {
    AppMessage msg;
    while (true) {
        // Esperamos un mensaje por siempre usando el Wrapper
        if (_alarms_events_task_queue.receive(&msg)) {
            // --- PROCESAR EL MENSAJE AQUI ---
            
            // --- FIN PROCESAMIENTO ---
        }
    }
}
