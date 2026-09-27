#pragma once
#include <mbed.h>
#include <stdint.h>
#include "tasks/app_messages.h"
#include "utils/message_queue.h"
#include "tasks/comm_link_task.h"
#include "repositories/imu_repository.h"

#include "app/config.h"
class AlarmsEventsTask {
    // Permitimos que el SystemTask orqueste y envie mensajes a esta tarea
    friend class SystemTask;

private:
    // Constructor privado (Patron Singleton)
    AlarmsEventsTask() {}
    ~AlarmsEventsTask() {}

    // Evitar copias
    AlarmsEventsTask(const AlarmsEventsTask&) = delete;
    AlarmsEventsTask& operator=(const AlarmsEventsTask&) = delete;

    rtos::Thread _thread;
    MessageQueue<AppMessage, 16> _alarms_events_task_queue;
    
    // Buffer para la ventana de impacto (en memoria estatica para evitar Stack Overflow)
    DataXYZ _imu_window_buffer[IMPACT_WINDOW_SIZE];

    void run();

protected:
    // Protegido: Solo los 'friend' (como SystemTask) pueden encolar trabajos aca.
    // Garantiza que nadie salte el esquema arquitectonico por error.
    bool sendMsg(AppMessage* msg);

protected:
    // Comandos y eventos exclusivos de esta tarea
    enum EventId : uint8_t {
        CMD_PROCESS_IMU,
        CMD_PROCESS_IMU_WAKEUP,
        CMD_STOP_PROCESS,
        EVT_PANIC_BUTTON,
    };



    // Funciones matematicas de aproximacion 3D
    static inline uint16_t approx_2d_improved(uint16_t a, uint16_t b);
    uint16_t suma_pitagorica(int16_t x, int16_t y, int16_t z);

    #define FLAG_FREE_FALL 0x01
    #define FLAG_IMPACT    0x02
    #define FLAG_FALL_DETECTED (FLAG_FREE_FALL | FLAG_IMPACT)

    // Funcion auxiliar para evaluar la ventana
    uint8_t evaluateWindow(DataXYZ* buffer, uint16_t len);
    
    // Funcion de procesamiento
    void processImuWindow(bool process_preFall);

public:
    static AlarmsEventsTask& getInstance() {
        static AlarmsEventsTask instance;
        return instance;
    }

    void init();
};
