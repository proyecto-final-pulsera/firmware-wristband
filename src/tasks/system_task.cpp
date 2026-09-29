#include "tasks/system_task.h"
#include "mbed.h"
#include "tasks/comm_link_task.h"
#include "drivers/bhi260_driver.h"
#include "repositories/event_repository.h"
#include "tasks/alarms_events_task.h"
#include "drivers/button_driver.h"

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
    // BHI260Driver* bhi = BHI260Driver::getInstance();
    
#ifdef DEBUG
    Serial.println("Tarea Sys iniciada - MODO TEST ALARMS EVENTS (Con Botones)");
#endif

    // /* TODO: A usar en el futuro próximo
    // ImuRepository* imu = ImuRepository::getInstance();
    // while(imu->getAvailableCount() < IMU_FIFO_SIZE){
    //      rtos::ThisThread::sleep_for(std::chrono::milliseconds(3000));
    //      bhi->updateFifoData();
    // }
    // 
    // msg.event_id = AlarmsEventsTask::CMD_PROCESS_IMU_WAKEUP;
    // msg.emisor_id = TASK_SYSTEM;
    // msg.priority_level = PRIORITY_NORMAL;
    // AlarmsEventsTask::getInstance().sendMsg(&msg);
    // */

    bool panicMdeActive = false;
    bool notifMdeActive = false;

    while (true) {
        // Si hay alguna evaluación de MDE pendiente, usamos timeout de 10ms
        uint32_t timeout = (panicMdeActive || notifMdeActive) ? 10 : osWaitForever;

        if (_system_task_queue.receive(&msg, timeout)) {
            // Procesar mensajes que llegan a la SystemTask
            switch (msg.event_id) {
                case CMD_EVALUATE_PANIC_BUTTON:
                    panicMdeActive = true;
                    break;
                case CMD_EVALUATE_NOTIF_BUTTON:
                    notifMdeActive = true;
                    break;
            }
        }

        // --- Evaluar Máquina de Estado del Botón de Pánico ---
        if (panicMdeActive) {
            PanicButton* panicBtn = PanicButton::getInstance();
            panicBtn->updateMDE();

            if (panicBtn->getPressed()) {
#ifdef DEBUG
                Serial.println("[SystemTask] PANIC BUTTON - Flanco Detectado!");
#endif
            }

            if (panicBtn->isIdle()) {
                panicMdeActive = false;
            }
        }

        // --- Evaluar Máquina de Estado del Botón de Notificación ---
        if (notifMdeActive) {
            NotifButton* notifBtn = NotifButton::getInstance();
            notifBtn->updateMDE();

            if (notifBtn->getPressed()) {
#ifdef DEBUG
                Serial.println("[SystemTask] NOTIF BUTTON - Flanco Detectado!");
#endif
            }

            if (notifBtn->isIdle()) {
                notifMdeActive = false;
            }
        }

        // --- Código de Testing para AlarmsEventsTask (Temporal) ---
        // Descomentar si se quiere probar de vuelta el acelerómetro
        /*
        static unsigned long lastImuTime = millis();
        if (millis() - lastImuTime >= 3000) {
            lastImuTime = millis();
            bhi->updateFifoData();
            msg.event_id = AlarmsEventsTask::CMD_PROCESS_IMU;        
            msg.emisor_id = TASK_SYSTEM;
            msg.priority_level = PRIORITY_NORMAL;
            AlarmsEventsTask::getInstance().sendMsg(&msg);
        }
        */
    }
}

