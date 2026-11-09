#pragma once
#include <mbed.h>
#include <stdint.h>
#include "tasks/app_messages.h"
#include "utils/message_queue.h"
#include "repositories/imu_repository.h"
#include "repositories/pressure_repository.h"
#include "tasks/alarms_events_task.h"
#include "app/config.h"
#include "drivers/comm_driver.h"
#include "drivers/led_driver.h"

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
    CommLinkTask() : _thread(osPriorityNormal, STACK_SIZE_COMM_LINK_TASK) {}
    ~CommLinkTask() {}

    CommLinkTask(const CommLinkTask&) = delete;
    CommLinkTask& operator=(const CommLinkTask&) = delete;

    rtos::Thread _thread;
    MessageQueue<AppMessage, TASK_QUEUE_SIZE> _comm_link_task_queue;

    MetricsData _metrics_buffer;
    CommPacket _rx_packet;
    uint8_t _retry_counts[16] = {0};

    bool _is_authenticated = true;
    uint8_t _auth_attempts = 0;
    uint32_t _auth_timer_start_ms = 0;
    bool _was_connected = true;

    // Métodos privados de procesamiento de recepción (RX)
    void handleRxPacket(CommDriver* comm);
    void handleAuthPacket(const uint8_t* payload, uint16_t length, CommDriver* comm);
    void handleTxCommand(const AppMessage& msg, CommDriver* comm);
    void handleAck(MsgType ackedMsg, uint8_t status);
    void dispatchRetry(MsgType type);

    void run();

protected:
    template <typename TargetTask>
    void notifyTask(uint8_t event_id, TaskId emisor_id = TASK_COMM_LINK, MsgPriority priority = PRIORITY_NORMAL) {
        AppMessage msg;
        msg.event_id = event_id;
        msg.emisor_id = emisor_id;
        msg.priority_level = priority;
        msg.flags = 0;
        msg.payload_ptr = nullptr;
        msg.payload_len = 0;
        TargetTask::getInstance().sendMsg(&msg);
    }

public:
    bool sendMsg(AppMessage* msg);      
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
        CMD_TX_BATTERY_DATA    = 0x0A,
        CMD_TX_NOTIF_BTN_PRESS = 0x0C,
        EVT_RX_PACKET          = 0x20
    };

    bool isAuthenticated() const { return _is_authenticated; }

    static CommLinkTask& getInstance() {
        static CommLinkTask instance;
        return instance;
    }

    void init();
};
