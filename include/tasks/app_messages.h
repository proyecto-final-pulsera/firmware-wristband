#pragma once
#include <stdint.h>

// Identificadores unicos para cada tarea del sistema
enum TaskId : uint8_t {
    TASK_UNKNOWN        = 0x00,
    TASK_SYSTEM         = 0x01,
    TASK_COMM_LINK      = 0x02,
    TASK_NOTIFY_TELEMETRY = 0x03,
    TASK_ALARMS_EVENTS  = 0x04,
    TASK_ISR_ROUTINE    = 0x05, // Para cuando un mensaje se origina en una interrupcion
    TASK_PMIC           = 0x06
};

// Niveles de prioridad sugeridos
enum MsgPriority : uint8_t {
    PRIORITY_LOW        = 0x00,
    PRIORITY_NORMAL     = 0x01,
    PRIORITY_HIGH       = 0x02,
    PRIORITY_CRITICAL   = 0x03
};

// Flags específicos para el control del LED (CMD_LED_NOTIFY)
// Se configuran usando operaciones bit a bit. Ej: LED_COLOR_RED | LED_MODE_BLINK
enum LedNotifyFlags : uint32_t {
    LED_COLOR_OFF   = 0x0000,
    LED_COLOR_RED   = 0x0001,
    LED_COLOR_GREEN = 0x0002,
    LED_COLOR_BLUE  = 0x0004,
    LED_COLOR_YELLOW= 0x0003, // RED | GREEN
    
    LED_MODE_SOLID  = 0x0100,
    LED_MODE_BLINK  = 0x0200,
    
    // Perfiles pre-armados (como los viejos setLedWarn, setLedNotif)
    LED_PROFILE_WARN  = (LED_COLOR_YELLOW | LED_MODE_SOLID),
    LED_PROFILE_NOTIF = (LED_COLOR_BLUE | LED_MODE_SOLID),
    LED_PROFILE_ALARM = (LED_COLOR_RED | LED_MODE_SOLID)
};

enum VibratorDurationFlags : uint32_t {
    VIB_DURATION_2S = 0x01,
    VIB_DURATION_4S = 0x02,
    VIB_DURATION_7S = 0x04
};

// Formato de Mensaje Universal para todas las tareas (El "Sobre")
struct AppMessage {
    MsgPriority priority_level; // Nivel de prioridad
    uint8_t event_id;           // ID del evento o comando a ejecutar
    TaskId emisor_id;           // Quien envia este mensaje
    uint32_t timestamp;         // Marca de tiempo (ej. rtos::Kernel::Clock::now().time_since_epoch().count())
    
    uint32_t flags;             // Banderas de estado adicionales
    
    // Payload (Carga util)
    void* payload_ptr;          // Puntero genérico a buffer, datos, structs, etc.
    uint16_t payload_len;       // Largo del buffer apuntado
};

#define PRE_FALL_PROCESSED_FLAG 0x1 