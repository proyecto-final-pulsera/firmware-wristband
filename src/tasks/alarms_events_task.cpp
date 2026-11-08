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
        Serial.print("[ERROR] AlarmsEventsTask: Fallo al iniciar hilo (codigo ");
        Serial.print((int)stat);
        Serial.println(" - Memoria insuficiente)");
    }
}

bool AlarmsEventsTask::sendMsg(AppMessage* msg) {
    return _alarms_events_task_queue.send(msg);
}

#include <stdlib.h> // Para abs()
#include "repositories/imu_repository.h"


inline uint16_t AlarmsEventsTask::approx_2d_improved(uint16_t a, uint16_t b) {
    // 1. Encontrar el mayor (Max) y el menor (Min)
    uint16_t max_val = (a > b) ? a : b;
    uint16_t min_val = (a < b) ? a : b;
    
    // 2. Calcular z0: a0=1, b0=0 -> 1*Max + 0*Min
    uint16_t z0 = max_val;
    
    // 3. Calcular z1: a1=7/8, b1=17/32
    // 7/8 * Max = Max - (Max / 8)
    uint16_t z1_max = max_val - (max_val >> 3); 
    
    // 17/32 * Min = (Min / 2) + (Min / 32)
    uint16_t z1_min = (min_val >> 1) + (min_val >> 5);
    
    uint16_t z1 = z1_max + z1_min;
    
    // 4. Retornar el mayor entre z0 y z1
    return (z0 > z1) ? z0 : z1;
}

// Función principal para el acelerómetro 3D
uint16_t AlarmsEventsTask::suma_pitagorica(int16_t x, int16_t y, int16_t z) {
    // Trabajar con valores absolutos
    uint16_t abs_x = abs(x);
    uint16_t abs_y = abs(y);
    uint16_t abs_z = abs(z);

    // Componer iterativamente: primero plano XY, luego el resultado con Z
    uint16_t mag_xy = approx_2d_improved(abs_x, abs_y);
    uint16_t mag_xyz = approx_2d_improved(mag_xy, abs_z);

    return mag_xyz;
}

void AlarmsEventsTask::run() {
    AppMessage msg;
    while (true) {
        // Esperamos por siempre hasta que llegue un comando (on-demand)
        if (_alarms_events_task_queue.receive(&msg, osWaitForever)) {
            // Procesar el mensaje recibido
        
            unsigned long t_start = micros();

            switch (msg.event_id) {
                case CMD_PROCESS_IMU: {
                    bool fall = processImuWindow(false);
                    DEBUG_PRINT("[AlarmsEventsTask] Tiempo process (us): "); DEBUG_PRINTLN(micros() - t_start);
                    if (fall) {
                        DEBUG_PRINTLN("[AlarmsEventsTask] *** ALARMA: CAIDA DETECTADA CON EXITO ***");
                        notifyTask<CommLinkTask>(CommLinkTask::CMD_TX_FALL_SENSORS);
                        notifyTask<CommLinkTask>(CommLinkTask::CMD_TX_ALARM, TASK_ALARMS_EVENTS, PRIORITY_NORMAL, 0);
                    }
                    break;
                }
                case CMD_PROCESS_IMU_WAKEUP: {
                    bool fall = processImuWindow(true);
                    DEBUG_PRINT("[AlarmsEventsTask] Tiempo process Wakeup (us): "); DEBUG_PRINTLN(micros() - t_start);
                    if (fall) {
                        DEBUG_PRINTLN("[AlarmsEventsTask] *** ALARMA: CAIDA DETECTADA CON EXITO (WAKEUP) ***");
                        notifyTask<CommLinkTask>(CommLinkTask::CMD_TX_FALL_SENSORS);
                        notifyTask<CommLinkTask>(CommLinkTask::CMD_TX_ALARM, TASK_ALARMS_EVENTS, PRIORITY_NORMAL, PRE_FALL_PROCESSED_FLAG);
                    }
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

uint8_t AlarmsEventsTask::evaluateWindow(uint16_t startIndex, uint16_t len) {
    uint8_t flags = 0;
    uint16_t magnitudes[IMPACT_WINDOW_SIZE];
    
    uint16_t process_len = (len > IMPACT_WINDOW_SIZE) ? IMPACT_WINDOW_SIZE : len;

    ImuRepository* imu = ImuRepository::getInstance();
    for (uint16_t k = 0; k < process_len; k++) {
        DataXYZ sample = imu->getElementAtUnprotected(startIndex + k);
        magnitudes[k] = suma_pitagorica(
            sample.x, 
            sample.y, 
            sample.z
        );
    }

    if (process_len <= SLIDING_WINDOW_SIZE) return flags;

    for (uint16_t i = 0; i <= process_len - SLIDING_WINDOW_SIZE; i++) {
        uint32_t sum_magnitud_ventana = 0;

        for (uint16_t j = 0; j < SLIDING_WINDOW_SIZE; j++) {
            sum_magnitud_ventana += magnitudes[i + j];
        }

        uint16_t promedio_ventana = sum_magnitud_ventana >> SLIDING_WINDOW_SHIFT;

        if (promedio_ventana < THRESHOLD_FREE_FALL) {
            if (!(flags & FLAG_FREE_FALL)) {
DEBUG_PRINT("[AlarmsEventsTask] FLAG CAIDA LIBRE detectada! Mag: "); DEBUG_PRINTLN(promedio_ventana);
            }
            flags |= FLAG_FREE_FALL;
        }
        
        if (promedio_ventana > THRESHOLD_IMPACT) {
            if (!(flags & FLAG_IMPACT)) {
DEBUG_PRINT("[AlarmsEventsTask] FLAG IMPACTO detectada! Mag: "); DEBUG_PRINTLN(promedio_ventana);
            }
            flags |= FLAG_IMPACT;
        }
        
        if (flags == FLAG_FALL_DETECTED) {
            return flags;
        }
    }
    return flags;
}

bool AlarmsEventsTask::processImuWindow(bool process_preFall) {
    ImuRepository* imu = ImuRepository::getInstance();
    
    if (imu->getAvailableCount() < IMU_FIFO_SIZE) {
        return false; 
    }

    uint8_t fall_flags = 0;

    imu->lock();

    // Procesamiento del inicio del buffer en caso de WAKEUP
    if (process_preFall) {
        fall_flags |= evaluateWindow(0, IMPACT_WINDOW_SIZE);
    }

    // Segunda etapa (o única si no es wakeup): ventana del medio
    if (fall_flags != FLAG_FALL_DETECTED) {
        fall_flags |= evaluateWindow(START_IMPACT_WINDOW_INDEX, IMPACT_WINDOW_SIZE);
    }

    imu->unlock();

    return (fall_flags == FLAG_FALL_DETECTED);
}
