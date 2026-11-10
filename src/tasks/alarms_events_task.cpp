#include "tasks/alarms_events_task.h"
#include "tasks/notify_telemetry_task.h"
#include "tasks/app_messages.h"
#include "tasks/comm_link_task.h"
#include "mbed.h"
#include "drivers/bhi260_driver.h"
#include "tasks/system_task.h"
#include "utils/debug.h"

void AlarmsEventsTask::init() {
    osStatus stat = _thread.start(mbed::callback(this, &AlarmsEventsTask::run));
    if (stat != osOK) {
        DEBUG_PRINT("[ERROR] AlarmsEventsTask: Fallo al iniciar hilo (codigo ");
        DEBUG_PRINT((int)stat);
        DEBUG_PRINTLN(" - Memoria insuficiente)");
    }
}

bool AlarmsEventsTask::sendMsg(AppMessage* msg) {
    return _alarms_events_task_queue.send(msg);
}

#include <stdlib.h> // Para abs()
#include <cmath>    // Para fabsf()
#include "repositories/imu_repository.h"


void AlarmsEventsTask::run() {
    AppMessage msg;
    while (true) {
        // Esperamos por siempre hasta que llegue un comando (on-demand)
        if (_alarms_events_task_queue.receive(&msg, osWaitForever)) {
            // Procesar el mensaje recibido
        
            unsigned long t_start = micros();

            switch (msg.event_id) {
                case CMD_PROCESS_IMU: {
                    processImuWindow(false);
                    DEBUG_PRINT("[AlarmsEventsTask] Tiempo process (us): "); DEBUG_PRINTLN(micros() - t_start);
                    notifyTask<SystemTask>(SystemTask::EVT_IMU_PROCESSED, TASK_ALARMS_EVENTS, PRIORITY_NORMAL, _lastEvaluatedFlags);
                    break;
                }
                case CMD_PROCESS_IMU_WAKEUP: {
                    processImuWindow(true);
                    DEBUG_PRINT("[AlarmsEventsTask] Tiempo process Wakeup (us): "); DEBUG_PRINTLN(micros() - t_start);
                    notifyTask<SystemTask>(SystemTask::EVT_IMU_PROCESSED, TASK_ALARMS_EVENTS, PRIORITY_NORMAL, _lastEvaluatedFlags | FLAG_IS_WAKEUP);
                    break;
                }
                case CMD_STOP_PROCESS:
                    // Actualmente no tiene efecto en procesamiento on-demand
                    break;
                case UPDATE_BUFFER_BHI:
                {
                    // Actualizar datos del sensor
                    BHI260Driver::getInstance()->updateFifoData();
                    DEBUG_PRINT("[AlarmsEventsTask] Tiempo update sensor (us): "); DEBUG_PRINTLN(micros() - t_start);
                    
                    // Validar emisor y responder
                    if (msg.emisor_id == TASK_SYSTEM) {
                        notifyTask<SystemTask>(SystemTask::BHI_UPDATED);
                    }
                    // Si hay otras tareas que puedan emitir este mensaje, 
                    // se agregarian sus validaciones aqui.
                    break;
                }
                default:
                    break;
            }
        }
    }
}

uint8_t AlarmsEventsTask::evaluateWindow(const uint16_t* magnitudes, uint16_t len) {
    uint8_t flags = 0;
    
    if (magnitudes == nullptr || len == 0) {
        return flags;
    }

    uint16_t process_len = (len > IMPACT_WINDOW_SIZE) ? IMPACT_WINDOW_SIZE : len;

    // Inicialización del filtro solo en el primer uso (cuando arranca el sistema)
    if (media_ema == 0) {
        media_ema = magnitudes[0];
        mad_ema = 0;
    }

    for (uint16_t i = 0; i < process_len; i++) {
        int32_t x = magnitudes[i];
        
        // 1. Filtros EMA en aritmética entera (alpha = 1/8)
        // Se asume que media_ema y mad_ema son atributos (int32_t) de la clase.
        media_ema += (x - media_ema) >> 3;
        
        int32_t diff = abs(x - media_ema);
        mad_ema += (diff - mad_ema) >> 3;

        // 2. Evaluar Caída Libre (Baja dinámica -> evaluado sobre la media)
        if (media_ema < THRESHOLD_FREE_FALL) {
            flags |= FLAG_FREE_FALL;
        }
        
        // 3. Evaluar Impacto (Alta dinámica -> evaluado sobre valor instantáneo)
        if (media_ema > THRESHOLD_IMPACT) {
            flags |= FLAG_IMPACT;
        }
        
        // Retorno prioritario si ya se detectó la secuencia completa de caída
        if ((flags & (FLAG_FREE_FALL | FLAG_IMPACT)) == (FLAG_FREE_FALL | FLAG_IMPACT)) {
            flags |= FLAG_FALL_DETECTED;
            return flags;
        }
    }

    // 4. Evaluar inmovilidad tras actualizar el MAD con toda la ventana
    if (mad_ema < THRESHOLD_NO_MOVEMENT_HIGH) { 
        flags |= FLAG_NO_MOVEMENT_HIGHT;
        flags |= FLAG_NO_MOVEMENT_LOW;
    } else if (mad_ema < THRESHOLD_NO_MOVEMENT_LOW) {
        flags |= FLAG_NO_MOVEMENT_LOW;
    }

    if ((flags & (FLAG_FREE_FALL | FLAG_IMPACT)) == (FLAG_FREE_FALL | FLAG_IMPACT)) {
        flags |= FLAG_FALL_DETECTED;
    }

    return flags;
}

bool AlarmsEventsTask::processImuWindow(bool process_preFall) {
    _lastEvaluatedFlags = 0;
    ImuRepository* imu = ImuRepository::getInstance();
    
    if (imu->getAvailableCount() < IMU_FIFO_SIZE) {
        return false; 
    }

    uint8_t fall_flags = 0;
    // Procesamiento de las ventanas inicial e intermedia en caso de WAKEUP
    if (process_preFall) {
        // Etapa 1: Ventana inicial desde el inicio del buffer (muestras 0 a IMPACT_WINDOW_SIZE - 1)
        uint16_t count = imu->copyFifoMagnitudes(_magnitudes, IMPACT_WINDOW_SIZE, 0);
        fall_flags |= evaluateWindow(_magnitudes, count);

        // Etapa 2: Ventana intermedia (toma la mitad de muestras de la ventana inicial y mitad de la central)
        if (!(fall_flags & FLAG_FALL_DETECTED)) {
            uint16_t count = imu->copyFifoMagnitudes(_magnitudes, IMPACT_WINDOW_SIZE, INTERMEDIATE_IMPACT_WINDOW_INDEX);
            if (count != 0) {
                fall_flags |= evaluateWindow(_magnitudes, count);
            }
        }
    }

    // Etapa 3 (o única si no es wakeup): Ventana central
    if (!(fall_flags & FLAG_FALL_DETECTED)) {
        uint16_t count = imu->copyFifoMagnitudes(_magnitudes, IMPACT_WINDOW_SIZE, START_IMPACT_WINDOW_INDEX);
        fall_flags |= evaluateWindow(_magnitudes, count);
    }

    _lastEvaluatedFlags = fall_flags;
    
    // --- Debug de Flags Detectados ---
    DEBUG_PRINT("[AlarmsEventsTask] Flags (0x");
    DEBUG_PRINT(fall_flags, HEX);
    DEBUG_PRINT("): ");
    if (fall_flags == 0) {
        DEBUG_PRINT("[NINGUNO]");
    } else {
        if (fall_flags & FLAG_FREE_FALL)         DEBUG_PRINT("[CAIDA_LIBRE] ");
        if (fall_flags & FLAG_IMPACT)            DEBUG_PRINT("[IMPACTO] ");
        if (fall_flags & FLAG_FALL_DETECTED)     DEBUG_PRINT("[CAIDA_DETECTADA] ");
        if (fall_flags & FLAG_NO_MOVEMENT_LOW)   DEBUG_PRINT("[REPOSO_LOW] ");
        if (fall_flags & FLAG_NO_MOVEMENT_HIGHT) DEBUG_PRINT("[REPOSO_HIGHT] ");
    }
    DEBUG_PRINTLN();

    return ((fall_flags & FLAG_FALL_DETECTED) != 0);
}
