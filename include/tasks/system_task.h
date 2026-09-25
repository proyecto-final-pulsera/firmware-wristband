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

    void run();

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
    };

public:
    static SystemTask& getInstance() {
        static SystemTask instance;
        return instance;
    }

    void init();
};
