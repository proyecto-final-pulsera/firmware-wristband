#pragma once
#include <mbed.h>
#include <stdint.h>
#include "tasks/app_messages.h"
#include "utils/message_queue.h"


class NotifUiTask {
    // Permitimos que el SystemTask orqueste y envie mensajes a esta tarea
    friend class SystemTask;

private:
    // Constructor privado (Patron Singleton)
    NotifUiTask() {}
    ~NotifUiTask() {}

    // Evitar copias
    NotifUiTask(const NotifUiTask&) = delete;
    NotifUiTask& operator=(const NotifUiTask&) = delete;

    rtos::Thread _thread;
    MessageQueue<AppMessage, 16> _notif_ui_task_queue;

    void run();

protected:
    // Protegido: Solo los 'friend' (como SystemTask) pueden encolar trabajos aca.
    // Garantiza que nadie salte el esquema arquitectonico por error.
    bool sendMsg(AppMessage* msg);

protected:
    // Comandos y eventos exclusivos de esta tarea
    enum EventId : uint8_t {
        CMD_LED_ALARM,
        CMD_LED_OK,
        CMD_BUZZER_BEEP,
    };

public:
    static NotifUiTask& getInstance() {
        static NotifUiTask instance;
        return instance;
    }

    void init();
};
