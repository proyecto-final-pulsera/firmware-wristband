#pragma once
#include <mbed.h>
#include <stdint.h>
#include "tasks/app_messages.h"
#include "utils/message_queue.h"
#include "repositories/imu_repository.h"
#include "repositories/pressure_repository.h"
#include "tasks/alarms_events_task.h"

// Struct de métricas como template para el futuro
struct MetricsData {
    uint16_t step_count;
    // Se pueden agregar más campos a futuro
};

class CommLinkTask {
    friend class SystemTask;
    friend class AlarmsEventsTask;
    friend class NotifyTelemetryTask;
private:
    CommLinkTask() {}
    ~CommLinkTask() {}

    CommLinkTask(const CommLinkTask&) = delete;
    CommLinkTask& operator=(const CommLinkTask&) = delete;

    rtos::Thread _thread;
    MessageQueue<AppMessage, 16> _comm_link_task_queue;

    // Buffers locales para resguardo antes de enviar
    DataXYZ _imu_tx_buffer[IMU_FIFO_SIZE];
    float _pressure_tx_buffer[PRESSURE_FIFO_SIZE];
    MetricsData _metrics_buffer;

    void run();

protected:
    bool sendMsg(AppMessage* msg);

    template <typename TargetTask>
    void notifyTask(uint8_t event_id, TaskId emisor_id = TASK_COMM_LINK, MsgPriority priority = PRIORITY_NORMAL) {
        AppMessage msg;
        msg.event_id = event_id;
        msg.emisor_id = emisor_id;
        msg.priority_level = priority;
        msg.flags = 0;
        msg.payload_ptr = nullptr;
        msg.payload_len = 0;
        msg.timestamp = (uint32_t)rtos::Kernel::Clock::now().time_since_epoch().count();
        TargetTask::getInstance().sendMsg(&msg);
    }

public:
    // Comandos y eventos exclusivos de esta tarea (que mapean a lo que enviaremos)
    enum EventId : uint8_t {
        CMD_TX_IMU_BUFFER      = 0x01,
        CMD_TX_PRESSURE_BUFFER = 0x02,
        CMD_TX_TEMPERATURE     = 0x03,
        CMD_TX_ALARM           = 0x04,
        CMD_TX_METRICS         = 0x05,
        CMD_TX_KEEP_ALIVE      = 0x06,
        CMD_TX_FALL_SENSORS    = 0x07,
        CMD_TX_WARNING         = 0x08,
        CMD_TX_PANIC_BTN_PRESS = 0x09,
        EVT_RX_PACKET          = 0x0A,
        CMD_TX_BATTERY_DATA    = 0x0B,
        CMD_TX_NOTIF_BTN_PRESS = 0x0C
    };

    static CommLinkTask& getInstance() {
        static CommLinkTask instance;
        return instance;
    }

    void init();
};
