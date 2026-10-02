#pragma once
#include <mbed.h>
#include <stdint.h>
#include "tasks/app_messages.h"
#include "utils/message_queue.h"


class SystemTask {
    // Permitimos que el SystemTask orqueste y envie mensajes a esta tarea

private:
    // Constructor privado (Patron Singleton)
    SystemTask() {}
    ~SystemTask() {}

    // Evitar copias
    SystemTask(const SystemTask&) = delete;
    SystemTask& operator=(const SystemTask&) = delete;

    rtos::Thread _thread;
    MessageQueue<AppMessage, 16> _system_task_queue;

    // --- Timers Cíclicos (ISRs) ---
    mbed::Ticker _keepAliveTicker;
    mbed::Ticker _metricsTicker;

    void onKeepAliveTick();
    void onMetricsTick();
    
    void setKeepAliveTimer(bool enable);
    void setMetricsTimer(bool enable);
    
    // --- Variables para la MDE del BHI260 ---
    enum BhiMdeState {
        BHI_MDE_IDLE,
        BHI_MDE_WAIT_FIRST_UPDATE,
        BHI_MDE_WAIT_SECOND_UPDATE,
        BHI_MDE_MONITORING_FALL,
        BHI_MDE_WAIT_MONITOR_UPDATE,
        BHI_MDE_DISCONNECTING_WAIT_IRQ,
        BHI_MDE_DISCONNECTING_WAIT_UPDATE
    };
    BhiMdeState _bhi_mde_state = BHI_MDE_IDLE;
    uint8_t _bhi_disconnect_counter = 0;
    bool _flag_bhi_irq = false;
    bool _flag_bhi_updated = false;

    bool _panic_mde_active = false;
    bool _notif_mde_active = false;

    void run();
    void updateBhiMde();
    void updateButtonsMde();

    template <typename TargetTask>
    void notifyTask(uint8_t event_id, TaskId emisor_id = TASK_SYSTEM, MsgPriority priority = PRIORITY_NORMAL) {
        AppMessage msg;
        // Se pueden inicializar el resto en 0 o dejarlos como están si no se usan
        msg.event_id = event_id;
        msg.emisor_id = emisor_id;
        msg.priority_level = priority;
        msg.flags = 0;
        msg.payload_ptr = nullptr;
        msg.payload_len = 0;
        msg.timestamp = rtos::Kernel::get_ms_count(); // Buena practica
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
    };

    static SystemTask& getInstance() {
        static SystemTask instance;
        return instance;
    }

    void init();
};
