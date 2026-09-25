#pragma once
#include <mbed.h>
#include <stdint.h>
#include "tasks/app_messages.h"
#include "utils/message_queue.h"


class CommLinkTask {
    // Permitimos que el SystemTask orqueste y envie mensajes a esta tarea
    friend class SystemTask;

private:
    // Constructor privado (Patron Singleton)
    CommLinkTask() {}
    ~CommLinkTask() {}

    // Evitar copias
    CommLinkTask(const CommLinkTask&) = delete;
    CommLinkTask& operator=(const CommLinkTask&) = delete;

    rtos::Thread _thread;
    MessageQueue<AppMessage, 16> _comm_link_task_queue;

    void run();

protected:
    // Protegido: Solo los 'friend' (como SystemTask) pueden encolar trabajos aca.
    // Garantiza que nadie salte el esquema arquitectonico por error.
    bool sendMsg(AppMessage* msg);

protected:
    // Comandos y eventos exclusivos de esta tarea
    enum EventId : uint8_t {
        CMD_TX_TELEMETRY,
        CMD_TX_ALARM,
        EVT_RX_PACKET,
    };

public:
    static CommLinkTask& getInstance() {
        static CommLinkTask instance;
        return instance;
    }

    void init();
};
