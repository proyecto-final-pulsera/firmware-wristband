#include "tasks/system_task.h"
#include "mbed.h"
#include "tasks/comm_link_task.h"
#include "drivers/bhi260_driver.h"
#include "repositories/event_repository.h"

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

    // --- MANUAL TEST ROUTINE FOR COMM LINK TASK ---
    BHI260Driver* bhi = BHI260Driver::getInstance();
    MotionRepository* motionEvent = MotionRepository::getInstance();
    NoMotionRepository* noMotionEvent = NoMotionRepository::getInstance();
    
    TestState currentState = WAIT_MOTION;
    uint32_t last_metrics = millis();
    Serial.println("Tarea Sys iniciada");
    while (true) {

        // Drenar FIFO para actualizar repositorios (polling continuo sin IRQ bloqueante)
        bhi->updateFifoData();

        if (currentState == WAIT_MOTION) {

            if (motionEvent->hasEventOccurred()) {
                Serial.println("Se detecto movimiento");
                motionEvent->clearEventFlag();
                noMotionEvent->clearEventFlag();
                noMotionEvent->begin(1.0f, 0);
                currentState = WAIT_STILL;
            }
        } else if (currentState == WAIT_STILL) {
            if (noMotionEvent->hasEventOccurred()) {
                Serial.println("Se detecto no movimiento");
                noMotionEvent->clearEventFlag();

                // Detectada la caída (cese de movimiento), enviamos los buffers
                
                msg.event_id = CommLinkTask::CMD_TX_FALL_SENSORS;
                msg.emisor_id = TASK_SYSTEM;
                CommLinkTask::getInstance().sendMsg(&msg);

                // Enviamos una alarma simulada de caída (ID genérico 0x12 en flags)
                
                msg.event_id = CommLinkTask::CMD_TX_ALARM;
                msg.emisor_id = TASK_SYSTEM;
                msg.flags = 0x12; 
                CommLinkTask::getInstance().sendMsg(&msg);

                // Volver a esperar movimiento
                motionEvent->clearEventFlag();
                motionEvent->begin(1.0f, 0);
                currentState = WAIT_MOTION;
            }
        }

        // Cada 5 segundos disparamos el envío de métricas de paso
        if (millis() - last_metrics >= 5000) {
            Serial.println("Envio metricas");
            msg.event_id = CommLinkTask::CMD_TX_METRICS;
            msg.emisor_id = TASK_SYSTEM;
            CommLinkTask::getInstance().sendMsg(&msg);
            last_metrics = millis();
        }

        // Pequeño retardo para no colgar el scheduler
        rtos::ThisThread::sleep_for(std::chrono::milliseconds(2000));
    }
    // ----------------------------------------------
}


void printHeapStats() {
    mbed_stats_heap_t heap_stats;
    mbed_stats_heap_get(&heap_stats);
    
    Serial.println("=== ESTADÍSTICAS DEL HEAP ===");
    Serial.print("Tamaño total reservado para Heap: ");
    Serial.print(heap_stats.reserved_size);
    Serial.println(" bytes");
    
    Serial.print("Uso actual: ");
    Serial.print(heap_stats.current_size);
    Serial.println(" bytes");
    
    Serial.print("Pico máximo histórico usado (Max): ");
    Serial.print(heap_stats.max_size);
    Serial.println(" bytes");
    Serial.println("=============================");
}