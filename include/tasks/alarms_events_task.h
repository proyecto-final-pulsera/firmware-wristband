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
    AlarmsEventsTask() : _thread(osPriorityNormal, STACK_SIZE_ALARMS_EVENTS_TASK) {}
    ~AlarmsEventsTask() {}

    // Evitar copias
    AlarmsEventsTask(const AlarmsEventsTask&) = delete;
    AlarmsEventsTask& operator=(const AlarmsEventsTask&) = delete;

    rtos::Thread _thread;
    MessageQueue<AppMessage, TASK_QUEUE_SIZE> _alarms_events_task_queue;
    
    // Buffer de magnitudes precalculadas (alojado en la tarea para evitar desbordar el stack)
    uint16_t _magnitudes[IMPACT_WINDOW_SIZE];
    uint8_t _lastEvaluatedFlags = 0;
    int32_t media_ema;
    int32_t mad_ema;

    void run();

protected:
    // Protegido: Solo los 'friend' (como SystemTask) pueden encolar trabajos aca.
    // Garantiza que nadie salte el esquema arquitectonico por error.
    bool sendMsg(AppMessage* msg);

    template <typename TargetTask>
    void notifyTask(uint8_t event_id, TaskId emisor_id = TASK_ALARMS_EVENTS, MsgPriority priority = PRIORITY_NORMAL, uint8_t flags = 0, void* payload_ptr = nullptr, uint16_t payload_len = 0) {
        AppMessage msg;
        msg.event_id = event_id;
        msg.emisor_id = emisor_id;
        msg.priority_level = priority;
        msg.flags = flags;
        msg.payload_ptr = payload_ptr;
        msg.payload_len = payload_len;
        TargetTask::getInstance().sendMsg(&msg);
    }

protected:
    // Comandos y eventos exclusivos de esta tarea
    enum EventId : uint8_t {
        CMD_PROCESS_IMU,
        CMD_PROCESS_IMU_WAKEUP,
        CMD_STOP_PROCESS,
        EVT_PANIC_BUTTON,
        UPDATE_BUFFER_BHI,
    };

    // Funciones matematicas de aproximacion 3D (delegadas a ImuRepository)
    static inline uint16_t approx_2d_improved(uint16_t a, uint16_t b) {
        return ImuRepository::approx2DImproved(a, b);
    }
    static inline uint16_t suma_pitagorica(int16_t x, int16_t y, int16_t z) {
        return ImuRepository::sumaPitagorica(x, y, z);
    }

    #define FLAG_FREE_FALL          0x01
    #define FLAG_IMPACT             0x02
    #define FLAG_NO_MOVEMENT_LOW    0x04
    #define FLAG_NO_MOVEMENT_HIGHT  0x08
    #define FLAG_NO_MOVEMENT_HIGH   FLAG_NO_MOVEMENT_HIGHT
    #define FLAG_FALL_DETECTED      0x10
    #define FLAG_IS_WAKEUP          0x20

    // Funcion auxiliar para evaluar la ventana sobre el vector de magnitudes provisto
    uint8_t evaluateWindow(const uint16_t* magnitudes, uint16_t len);
    
    // Funcion de procesamiento (retorna true si hubo caida)
    bool processImuWindow(bool process_preFall);

public:
    static AlarmsEventsTask& getInstance() {
        static AlarmsEventsTask instance;
        return instance;
    }

    void init();
    uint8_t getLastEvaluatedFlags() const { return _lastEvaluatedFlags; }
};
