#include "tasks/system_task.h"
#include "mbed.h"
#include "tasks/comm_link_task.h"
#include "drivers/bhi260_driver.h"
#include "repositories/event_repository.h"

#include "tasks/alarms_events_task.h"
#include "drivers/button_driver.h"
#include "utils/debug.h"
#include "Arduino_BHY2.h"
extern BoschSensortec sensortec;

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
    WAIT_STILLgit 
};

void SystemTask::run() {
    AppMessage msg;
    BHI260Driver* bhi = BHI260Driver::getInstance();
    
    DEBUG_PRINTLN("Tarea Sys iniciada - MODO EVENTOS Y MDE BHI");
    

    while (true) {
        // Timeout condicional: si hay rebote de boton pendiente, iteramos rapido. 
        // Si no hay nada, dormimos el hilo hasta que caiga un evento RTOS.
        uint32_t timeout = (_panic_mde_active || _notif_mde_active) ? 10 : osWaitForever;

        if (_system_task_queue.receive(&msg, timeout)) {
            switch (msg.event_id) {
                case CMD_EVALUATE_PANIC_BUTTON:
                    _panic_mde_active = true;
                    break;
                case CMD_EVALUATE_NOTIF_BUTTON:
                    _notif_mde_active = true;
                    break;
                case EVT_BHI_INTERRUPT:
                    _flag_bhi_irq = true;
                    break;
                case BHI_UPDATED:
                    _flag_bhi_updated = true;
                    break;
            }
        }

        // --- Evaluar Maquina de Estado del BHI ---
        if (_flag_bhi_irq || _flag_bhi_updated) {
            updateBhiMde();
        }

        // --- Evaluar Maquinas de Estado de Botones ---
        updateButtonsMde();
    }
}

void SystemTask::updateBhiMde() {
    switch (_bhi_mde_state) {
        case BHI_MDE_IDLE:
            if (_flag_bhi_irq) {
                DEBUG_PRINTLN("[BHI_MDE] IRQ recibida. IDLE -> WAIT_FIRST_UPDATE");
                notifyTask<AlarmsEventsTask>(AlarmsEventsTask::UPDATE_BUFFER_BHI);
                _bhi_mde_state = BHI_MDE_WAIT_FIRST_UPDATE;
            }
            break;

        case BHI_MDE_WAIT_FIRST_UPDATE:
            if (_flag_bhi_updated) {
                if (MotionRepository::getInstance()->hasEventOccurred()) {
                    DEBUG_PRINTLN("[BHI_MDE] Movimiento detectado! Activating sensors...");
                    // Activar non wakeup fifo
                    BHI260Driver::getInstance()->enableNonWakeupFIFO();
                    
                    // Limpiar flags
                    MotionRepository::getInstance()->clearEventFlag();
                    NoMotionRepository::getInstance()->clearEventFlag();
                    
                    // Reactivar gesto de no movimiento
                    NoMotionRepository::getInstance()->begin(1.0f, 0);
                    
                    // Enviar update de nuevo inmediatamente
                    notifyTask<AlarmsEventsTask>(AlarmsEventsTask::UPDATE_BUFFER_BHI);
                    
                    _bhi_mde_state = BHI_MDE_WAIT_SECOND_UPDATE;
                } else {
                    _bhi_mde_state = BHI_MDE_IDLE;
                }
            }
            break;

        case BHI_MDE_WAIT_SECOND_UPDATE:
            if (_flag_bhi_updated) {
                DEBUG_PRINTLN("[BHI_MDE] Enviando WAKEUP_PROCESS -> MONITORING_FALL");
                notifyTask<AlarmsEventsTask>(AlarmsEventsTask::CMD_PROCESS_IMU_WAKEUP);
                _bhi_mde_state = BHI_MDE_MONITORING_FALL;
            }
            break;

        case BHI_MDE_MONITORING_FALL:
            if (_flag_bhi_irq) {
                notifyTask<AlarmsEventsTask>(AlarmsEventsTask::UPDATE_BUFFER_BHI);
                _bhi_mde_state = BHI_MDE_WAIT_MONITOR_UPDATE;
            }
            break;

        case BHI_MDE_WAIT_MONITOR_UPDATE:
            if (_flag_bhi_updated) {
                if (NoMotionRepository::getInstance()->hasEventOccurred()) {
                    DEBUG_PRINTLN("[BHI_MDE] No-Movimiento detectado. Iniciando desconexion...");
                    // Reactivar gesto de movimiento
                    MotionRepository::getInstance()->clearEventFlag();
                    MotionRepository::getInstance()->begin(1.0f, 0);

                    NoMotionRepository::getInstance()->clearEventFlag();
                    _bhi_disconnect_counter = 0;
                    _bhi_mde_state = BHI_MDE_DISCONNECTING_WAIT_IRQ;
                } else {
                    notifyTask<AlarmsEventsTask>(AlarmsEventsTask::CMD_PROCESS_IMU);
                    _bhi_mde_state = BHI_MDE_MONITORING_FALL;
                }
            }
            break;

        case BHI_MDE_DISCONNECTING_WAIT_IRQ:
            if (_flag_bhi_irq) {
                notifyTask<AlarmsEventsTask>(AlarmsEventsTask::UPDATE_BUFFER_BHI);
                _bhi_mde_state = BHI_MDE_DISCONNECTING_WAIT_UPDATE;
            }
            break;

        case BHI_MDE_DISCONNECTING_WAIT_UPDATE:
            if (_flag_bhi_updated) {
                // Validar siempre si hubo caida durante la desconexion
                notifyTask<AlarmsEventsTask>(AlarmsEventsTask::CMD_PROCESS_IMU);
                
                if (MotionRepository::getInstance()->hasEventOccurred()) {
                    DEBUG_PRINTLN("[BHI_MDE] Movimiento detectado! Abortando desconexion -> MONITORING_FALL");
                    MotionRepository::getInstance()->clearEventFlag();
                    NoMotionRepository::getInstance()->clearEventFlag();
                    NoMotionRepository::getInstance()->begin(1.0f, 0);
                    
                    _bhi_mde_state = BHI_MDE_MONITORING_FALL;
                } else {
                    _bhi_disconnect_counter++;
                    if (_bhi_disconnect_counter >= NUM_PROCESS_POST_NOMOTION) {
                        DEBUG_PRINTLN("[BHI_MDE] Desconexion completada. Entrando en Hibernacion (IDLE)");
                        // Entrar en hibernacion
                        BHI260Driver::getInstance()->disableNonWakeupFIFO();
                        _bhi_mde_state = BHI_MDE_IDLE;
                    } else {
                        _bhi_mde_state = BHI_MDE_DISCONNECTING_WAIT_IRQ;
                    }
                }
            }
            break;
    }

    // Limpiar flags para la proxima corrida
    _flag_bhi_irq = false;
    _flag_bhi_updated = false;
}

void SystemTask::updateButtonsMde() {
    // --- Evaluar Maquina de Estado del Boton de Panico ---
    if (_panic_mde_active) {
        PanicButton* panicBtn = PanicButton::getInstance();
        panicBtn->updateMDE();

        if (panicBtn->getPressed()) {
            DEBUG_PRINTLN("[SystemTask] PANIC BUTTON - Flanco Detectado!");
            notifyTask<CommLinkTask>(CommLinkTask::CMD_TX_PANIC_BTN_PRESS);
        }

        if (panicBtn->isIdle()) {
            _panic_mde_active = false;
        }
    }

    // --- Evaluar Maquina de Estado del Boton de Notificacion ---
    if (_notif_mde_active) {
        NotifButton* notifBtn = NotifButton::getInstance();
        notifBtn->updateMDE();

        if (notifBtn->getPressed()) {
            DEBUG_PRINTLN("[SystemTask] NOTIF BUTTON - Flanco Detectado!");
        }

        if (notifBtn->isIdle()) {
            _notif_mde_active = false;
        }
    }
}

