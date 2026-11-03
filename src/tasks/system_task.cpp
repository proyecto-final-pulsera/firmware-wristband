#include "tasks/system_task.h"
#include "mbed.h"
#include "tasks/comm_link_task.h"
#include "drivers/bhi260_driver.h"
#include "repositories/event_repository.h"
#include "tasks/notify_telemetry_task.h"

#include "tasks/alarms_events_task.h"
#include "drivers/button_driver.h"
#include "utils/debug.h"
#include "Arduino_BHY2.h"
#include "drivers/battery_driver.h"
extern BoschSensortec sensortec;

void SystemTask::init() {
    osStatus stat = _thread.start(mbed::callback(this, &SystemTask::run));
    if (stat != osOK) {
        Serial.print("[ERROR] SystemTask: Fallo al iniciar hilo (codigo ");
        Serial.print((int)stat);
        Serial.println(" - Memoria insuficiente)");
    }
}

bool SystemTask::sendMsg(AppMessage* msg) {
    return _system_task_queue.send(msg);
}

void SystemTask::setKeepAliveTimer(bool enable) {
    if (enable) {
        _keepAliveTicker.attach(mbed::callback(this, &SystemTask::onKeepAliveTick), std::chrono::seconds(KEEP_ALIVE_PERIOD_SEC));
    } else {
        _keepAliveTicker.detach();
    }
}

void SystemTask::setMetricsTimer(bool enable) {
    if (enable) {
        _metricsTicker.attach(mbed::callback(this, &SystemTask::onMetricsTick), std::chrono::seconds(METRICS_PERIOD_SEC));
    } else {
        _metricsTicker.detach();
    }
}

void SystemTask::onKeepAliveTick() {
    // Desde la ISR del Ticker, metemos el evento a CommLinkTask
    notifyTask<CommLinkTask>(CommLinkTask::CMD_TX_KEEP_ALIVE, TASK_SYSTEM, PRIORITY_LOW);
}

void SystemTask::onMetricsTick() {
    // Desde la ISR del Ticker, metemos el evento a NotifyTelemetryTask
    notifyTask<NotifyTelemetryTask>(NotifyTelemetryTask::CMD_NOTIF_ACTIVITY_DATA, TASK_SYSTEM, PRIORITY_LOW);
}

enum TestState {
    WAIT_MOTION,
    WAIT_STILLgit 
};

void SystemTask::run() {
    AppMessage msg;
    
    DEBUG_PRINTLN("Tarea Sys iniciada - MODO EVENTOS Y MDE BHI");

    // Descomentar las siguientes lineas para arrancar el envio ciclico
    setKeepAliveTimer(true);
    setMetricsTimer(true);

    // Watchdog de PMIC (BQ25120): requiere un ping I2C cada < 50s.
    // Delegado desde PmicTask para evitar el consumo de un hilo independiente.
    uint32_t last_pmic_ping = millis();

    while (true) {
        // Ping periodico al PMIC cada 30 segundos
        uint32_t now = millis();
        if ((now - last_pmic_ping) >= 30000) {
            BatteryDriver* battery = BatteryDriver::getInstance();
            if (battery != nullptr) {
                battery->ping();
            }
            last_pmic_ping = now;
        }

        // Si hay rebote de boton pendiente, timeout de 10 ms.
        // Si no, dormimos hasta el proximo evento o hasta el proximo ping de PMIC (max 30s).
        uint32_t time_to_next_ping = (30000 > (now - last_pmic_ping)) ? (30000 - (now - last_pmic_ping)) : 0;
        uint32_t timeout = (_panic_mde_active || _notif_mde_active) ? 10 : time_to_next_ping;

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
                case EVT_TX_DELAYED:
                    _flag_process_full_buffer = true;
                    _delayed_tx_count++;
                    DEBUG_PRINTLN("[SystemTask] Recibido EVT_TX_DELAYED: Proximo procesamiento evaluara buffer completo");
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
                _flag_process_full_buffer = false;
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
                    // Si CommLinkTask demoró en transmitir más tiempo que la ventana de impacto
                    // (IMPACT_WINDOW_SIZE / FREQ_IMU), la FIFO acumuló muestras durante la espera.
                    // Para no omitir eventos ocurridos al inicio del buffer durante ese retardo,
                    // se despacha CMD_PROCESS_IMU_WAKEUP (evalúa ventana inicial + central) en vez de solo la central.
                    if (_flag_process_full_buffer) {
                        DEBUG_PRINTLN("[BHI_MDE] Procesando buffer completo debido a retardo en TX");
                        notifyTask<AlarmsEventsTask>(AlarmsEventsTask::CMD_PROCESS_IMU_WAKEUP);
                        _flag_process_full_buffer = false;
                    } else {
                        notifyTask<AlarmsEventsTask>(AlarmsEventsTask::CMD_PROCESS_IMU);
                    }
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
                // Validar siempre si hubo caída durante la desconexión. Si hubo retardo en TX previo,
                // se procesa el buffer completo (pre-fall + central) para no perder eventos rezagados.
                if (_flag_process_full_buffer) {
                    DEBUG_PRINTLN("[BHI_MDE] Procesando buffer completo en desconexion debido a retardo en TX");
                    notifyTask<AlarmsEventsTask>(AlarmsEventsTask::CMD_PROCESS_IMU_WAKEUP);
                    _flag_process_full_buffer = false;
                } else {
                    notifyTask<AlarmsEventsTask>(AlarmsEventsTask::CMD_PROCESS_IMU);
                }
                
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
            notifyTask<NotifyTelemetryTask>(NotifyTelemetryTask::CMD_VIBRATOR_INTERMITTENT, TASK_SYSTEM, PRIORITY_NORMAL, VIB_DURATION_4S);
            // TODO notificar boton Presionado y no envio de sensores. Nota Quiza es una buena idea pasar los datos para procesarlos.
            notifyTask<CommLinkTask>(CommLinkTask::CMD_TX_FALL_SENSORS);
            notifyTask<CommLinkTask>(CommLinkTask::CMD_TX_ALARM, TASK_SYSTEM, PRIORITY_NORMAL, 0);
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
            notifyTask<NotifyTelemetryTask>(NotifyTelemetryTask::CMD_VIBRATOR_CONTINUOUS, TASK_SYSTEM, PRIORITY_NORMAL, VIB_DURATION_2S);
            notifyTask<CommLinkTask>(CommLinkTask::CMD_TX_FALL_SENSORS);
            notifyTask<CommLinkTask>(CommLinkTask::CMD_TX_ALARM, TASK_SYSTEM, PRIORITY_NORMAL, 0);
            notifyTask<CommLinkTask>(CommLinkTask::CMD_TX_NOTIF_BTN_PRESS);
        }

        if (notifBtn->isIdle()) {
            _notif_mde_active = false;
        }
    }
}


