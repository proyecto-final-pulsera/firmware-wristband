#pragma once
#include <mbed.h>
#include <stdint.h>
#include "tasks/app_messages.h"
#include "utils/message_queue.h"
#include "tasks/comm_link_task.h" // Para MetricsData
#include "repositories/event_repository.h"

// Estructuras de datos a compartir 
struct BatteryData {
    uint8_t charge_percent;
    bool is_charging;
};

class NotifyTelemetryTask {
    friend class SystemTask;

private:
    NotifyTelemetryTask() {}
    ~NotifyTelemetryTask() {}
    NotifyTelemetryTask(const NotifyTelemetryTask&) = delete;
    NotifyTelemetryTask& operator=(const NotifyTelemetryTask&) = delete;

    rtos::Thread _thread;
    MessageQueue<AppMessage, 16> _mailbox;
    
    // Instancias estáticas (Compartidas mediante punteros y payload_len)
    static BatteryData _shared_battery_data;
    static MetricsData _shared_metrics_data;

    void run();
    void updateMetrics();

protected:
    bool sendMsg(AppMessage* msg);

    template <typename TargetTask>
    void notifyTask(uint8_t event_id, TaskId emisor_id = TASK_NOTIFY_TELEMETRY, MsgPriority priority = PRIORITY_NORMAL, void* payload_ptr = nullptr, uint16_t payload_len = 0) {
        AppMessage msg;
        msg.event_id = event_id;
        msg.emisor_id = emisor_id;
        msg.priority_level = priority;
        msg.flags = 0;
        msg.payload_ptr = payload_ptr;
        msg.payload_len = payload_len;
        msg.timestamp = rtos::Kernel::get_ms_count();
        TargetTask::getInstance().sendMsg(&msg);
    }

public:
    enum EventId : uint8_t {
        CMD_VIBRATOR_GESTURE,  // flag = id del gesto
        CMD_LED_NOTIFY,        // flag = color / modo (bitmask)
        CMD_KEEP_ALIVE_ENA,
        CMD_KEEP_ALIVE_DES,
        CMD_FORCE_KEEP_ALIVE,
        CMD_NOTIF_BATTERY_DATA,
        CMD_NOTIF_ACTIVITY_DATA
    };

    static NotifyTelemetryTask& getInstance() {
        static NotifyTelemetryTask instance;
        return instance;
    }

    void init();
};
