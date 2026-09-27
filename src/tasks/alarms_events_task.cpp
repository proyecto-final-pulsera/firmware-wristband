#include "tasks/alarms_events_task.h"

#include "mbed.h"

void AlarmsEventsTask::init() {
    // Inicializar hardware, configuraciones previas al inicio, etc.
    
    // Iniciar el hilo asociado al metodo run de esta instancia
    _thread.start(mbed::callback(this, &AlarmsEventsTask::run));
}

bool AlarmsEventsTask::sendMsg(AppMessage* msg) {
    return _alarms_events_task_queue.send(msg);
}

#include <stdlib.h> // Para abs()
#include "repositories/imu_repository.h"

// Función mejorada para 2D usando el primer caso de la tabla
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
            switch (msg.event_id) {
                case CMD_PROCESS_IMU:
                    processImuWindow();
                    break;
                case CMD_STOP_PROCESS:
                    // Actualmente no tiene efecto en procesamiento on-demand
                    break;
                default:
                    break;
            }
        }
    }
}

void AlarmsEventsTask::processImuWindow() {
    ImuRepository* imu = ImuRepository::getInstance();
    
    // Verificamos que el buffer esté teóricamente lleno. 
    // Si no está lleno, descartamos esta tanda y no procesamos.
    if (imu->getAvailableCount() < IMU_FIFO_SIZE) {
        return; 
    }

    // Índice fijo para sacar la ventana del medio exacto del buffer
    
    // TODO: Optimizar esto, ya que no deberia ser necesario procesar toda la ventana continuamente 
    // en cada ciclo del procesador (existe un solapamiento y se puede desplazar la ventana).
    uint16_t len = imu->copyFifoValues(_imu_window_buffer, IMPACT_WINDOW_SIZE, START_IMPACT_WINDOW_INDEX);

    if (len < IMPACT_WINDOW_SIZE) {
        return; 
    }

    bool flag_caida_libre = false;
    bool flag_impacto = false;

    // Recorremos buscando la ventana pequeña (SLIDING_WINDOW_SIZE)
    for (uint16_t i = 0; i <= len - SLIDING_WINDOW_SIZE; i++) {
        uint32_t sum_magnitud_ventana = 0;

        for (uint16_t j = 0; j < SLIDING_WINDOW_SIZE; j++) {
            sum_magnitud_ventana += suma_pitagorica(
                _imu_window_buffer[i + j].x, 
                _imu_window_buffer[i + j].y, 
                _imu_window_buffer[i + j].z
            );
        }

        // Promedio usando bitshift en lugar de división (SLIDING_WINDOW_SIZE debe ser 8)
        uint16_t promedio_ventana = sum_magnitud_ventana >> SLIDING_WINDOW_SHIFT;

        if (promedio_ventana < THRESHOLD_FREE_FALL) {
            if (!flag_caida_libre) {
                Serial.print("[AlarmsEventsTask] FLAG CAIDA LIBRE detectada! Mag: ");
                Serial.println(promedio_ventana);
            }
            flag_caida_libre = true;
        }
        
        if (promedio_ventana > THRESHOLD_IMPACT) {
            if (!flag_impacto) {
                Serial.print("[AlarmsEventsTask] FLAG IMPACTO detectada! Mag: ");
                Serial.println(promedio_ventana);
            }
            flag_impacto = true;
        }

        // Si detectamos ambas condiciones en la ventana de 6 segundos...
        if (flag_caida_libre && flag_impacto) {
            // CONDICION DE CAIDA
            Serial.println("[AlarmsEventsTask] *** ALARMA: CAIDA DETECTADA CON EXITO ***");

            // (Acá se deberá enviar un mensaje de notificación de caída)
            AppMessage caidaMsg;
            caidaMsg.event_id = CommLinkTask::CMD_TX_FALL_SENSORS;
            caidaMsg.emisor_id = TASK_ALARMS_EVENTS;
            CommLinkTask::getInstance().sendMsg(&caidaMsg);
            // Rompemos el ciclo para no procesar el resto si ya determinamos que hay caída
            break; 
        }
    }
}
