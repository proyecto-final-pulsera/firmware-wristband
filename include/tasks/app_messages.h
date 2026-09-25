#pragma once
#include <stdint.h>

// Identificadores unicos para cada tarea del sistema
enum TaskId : uint8_t {
    TASK_UNKNOWN        = 0x00,
    TASK_SYSTEM         = 0x01,
    TASK_COMM_LINK      = 0x02,
    TASK_NOTIF_UI       = 0x03,
    TASK_ALARMS_EVENTS  = 0x04,
    TASK_ISR_ROUTINE    = 0x05 // Para cuando un mensaje se origina en una interrupcion
};

// Niveles de prioridad sugeridos
enum MsgPriority : uint8_t {
    PRIORITY_LOW        = 0x00,
    PRIORITY_NORMAL     = 0x01,
    PRIORITY_HIGH       = 0x02,
    PRIORITY_CRITICAL   = 0x03
};

// Formato de Mensaje Universal para todas las tareas (El "Sobre")
struct AppMessage {
    MsgPriority priority_level; // Nivel de prioridad
    uint8_t event_id;           // ID del evento o comando a ejecutar
    TaskId emisor_id;           // Quien envia este mensaje
    uint32_t timestamp;         // Marca de tiempo (ej. rtos::Kernel::get_ms_count())
    
    uint32_t flags;             // Banderas de estado adicionales
    
    // Payload (Carga util)
    void* payload_ptr;          // Puntero genérico a buffer, datos, structs, etc.
    uint16_t payload_len;       // Largo del buffer apuntado
};
