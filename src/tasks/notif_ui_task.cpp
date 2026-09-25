#include "tasks/notif_ui_task.h"
#include "mbed.h"

void NotifUiTask::init() {
    // Inicializar hardware, configuraciones previas al inicio, etc.
    
    // Iniciar el hilo asociado al metodo run de esta instancia
    _thread.start(mbed::callback(this, &NotifUiTask::run));
}

bool NotifUiTask::sendMsg(AppMessage* msg) {
    return _notif_ui_task_queue.send(msg);
}

void NotifUiTask::run() {
    AppMessage msg;
    while (true) {
        // Esperamos un mensaje por siempre usando el Wrapper
        if (_notif_ui_task_queue.receive(&msg)) {
            // --- PROCESAR EL MENSAJE AQUI ---
            
            
            // --- FIN PROCESAMIENTO ---
        }
    }
}
    }
}
