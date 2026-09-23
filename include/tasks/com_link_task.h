#pragma once
#include <stdint.h>
#include "rtos.h"
#include "drivers/comm_driver.h" // Para conocer los MsgType y la clase

// =============================================================================
// Estructuras de la Cola Inter-Task (Buzón de Mensajes)
// =============================================================================

// Tipos de Eventos Internos del RTOS para la Tarea de Comunicación
enum CommTaskEvent : uint8_t {
    EVENT_RX_DATA_READY      = 0x00, // Disparado por la ISR (vía callback)
    EVENT_TX_IMU_BUFFER      = 0x01, // Disparado por Processing Task
    EVENT_TX_PRESSURE_BUFFER = 0x02, // Disparado por Processing Task
    EVENT_TX_TEMPERATURE     = 0x03, // Disparado por System Task
    EVENT_TX_ALARM           = 0x04, // Disparado por Processing Task
    EVENT_TX_METRICS         = 0x05  // Disparado por System/App
};

// El "Sobre" que viaja por la Queue de Mbed OS
struct CommMessage {
    CommTaskEvent event_type; // Qué hay que hacer (TX o RX)
    
    // Si es un evento TX, estos campos llevan los datos a enviar
    const uint8_t* payload_ptr;
    uint16_t payload_len;
};

// =============================================================================
// Funciones de la Tarea de Comunicación
// =============================================================================

/**
 * @brief Función principal del hilo (Thread) de comunicación.
 *        Se bloquea esperando mensajes en la Queue.
 */
void com_link_task_main();

/**
 * @brief Inicializa los recursos de la tarea de comunicación (Queue, Driver)
 */
void com_link_task_init();
