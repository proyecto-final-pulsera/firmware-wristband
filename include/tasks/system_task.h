#pragma once
#include <mbed.h>
#include <stdint.h>
#include "tasks/app_messages.h"
#include "utils/message_queue.h"
#include "app/config.h"

#define KEEP_ALIVE_PERIOD_SEC   60
#define METRICS_PERIOD_SEC      300 // 5 minutos

class SystemTask {
    // Permitimos que el SystemTask orqueste y envie mensajes a esta tarea

private:
    SystemTask() : _thread(osPriorityNormal, STACK_SIZE_SYSTEM_TASK) {}
    ~SystemTask() {}

    // Evitar copias
    SystemTask(const SystemTask&) = delete;
    SystemTask& operator=(const SystemTask&) = delete;

    rtos::Thread _thread;
    MessageQueue<AppMessage, SYSTEM_TASK_QUEUE_SIZE> _system_task_queue;

    // --- Timers Cíclicos (ISRs) ---
    mbed::Ticker _keepAliveTicker;
    mbed::Ticker _metricsTicker;

    void onKeepAliveTick();
    void onMetricsTick();
    
    void setKeepAliveTimer(bool enable);
    void setMetricsTimer(bool enable);
    
    // --- Variables para la MDE del BHI260 ---
    enum BhiMdeState {
        // --- Estado de Reposo Base ---
        STATE_IDLE_SLEEP,

        // --- 1. Inicialización Post-Dormida ---
        WAKEUP_WAIT_FIRST_FIFO_UPDATE,
        WAKEUP_WAIT_SECOND_FIFO_UPDATE,
        WAKEUP_WAIT_PROCESS_RESULT,

        // --- 2. Estados de Régimen (Monitoreo Continuo) ---
        REGIME_WAIT_FIFO_IRQ,
        REGIME_WAIT_FIFO_UPDATE,
        REGIME_WAIT_PROCESS_RESULT,

        // --- 3. Estados para Poner a Dormir (Verificación de Inmovilidad) ---
        SLEEP_WAIT_VERIFICATION_IRQ,
        SLEEP_WAIT_FIFO_UPDATE,
        SLEEP_WAIT_PROCESS_RESULT,

        // --- 4. Sondeo Dispositivo Quitado ---
        OFF_WRIST_WAIT_FIFO_UPDATE,
        OFF_WRIST_WAIT_PROCESS_RESULT
    };
    BhiMdeState _bhi_mde_state = STATE_IDLE_SLEEP;
    uint8_t _bhi_disconnect_counter = 0;
    
    // Ticker para sondeo Off-Wrist
    mbed::Ticker _offWristTicker;
    void onOffWristTick();
    void setOffWristTimer(bool enable);
    uint8_t _off_wrist_consecutive_count = 0;
    bool _is_device_off_wrist = false;
    bool _flag_bhi_irq = false;
    bool _flag_bhi_updated = false;
    bool _flag_imu_processed = false;
    bool _flag_probe_off_wrist = false;
    uint8_t _last_imu_flags = 0;
    bool _flag_process_full_buffer = false;
    uint32_t _delayed_tx_count = 0;

    bool _panic_mde_active = false;
    bool _notif_mde_active = false;

    void run();
    void updateBhiMde();
    void updateButtonsMde();

    template <typename TargetTask>
    void notifyTask(uint8_t event_id, TaskId emisor_id = TASK_SYSTEM, MsgPriority priority = PRIORITY_NORMAL, uint8_t flags = 0, void* payload_ptr = nullptr, uint16_t payload_len = 0) {
        AppMessage msg;
        // Se pueden inicializar el resto en 0 o dejarlos como están si no se usan
        msg.event_id = event_id;
        msg.emisor_id = emisor_id;
        msg.priority_level = priority;
        msg.flags = flags;
        msg.payload_ptr = payload_ptr;
        msg.payload_len = payload_len;
        TargetTask::getInstance().sendMsg(&msg);
    }

public: // API Publica: Todas las tareas pueden reportar al SystemTask
    // Protegido: Solo los 'friend' (como SystemTask) pueden encolar trabajos aca.
    // Garantiza que nadie salte el esquema arquitectonico por error.
    bool sendMsg(AppMessage* msg);

public:
    // Comandos y eventos exclusivos de esta tarea
    enum EventId : uint8_t {
        CMD_PROCESS_ALARM,
        CMD_UPDATE_STATE,
        EVT_BATTERY_LOW,
        CMD_EVALUATE_PANIC_BUTTON,
        CMD_EVALUATE_NOTIF_BUTTON,
        BHI_UPDATED,
        EVT_BHI_INTERRUPT,
        EVT_TX_DELAYED,
        EVT_IMU_PROCESSED,
        CMD_PROBE_OFF_WRIST,
    };

    uint32_t getDelayedTxCount() const { return _delayed_tx_count; }

    static SystemTask& getInstance() {
        static SystemTask instance;
        return instance;
    }

    void init();
};
