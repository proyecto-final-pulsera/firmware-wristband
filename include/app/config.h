#pragma once

// Descomentar para utilizar comunicación Serial en lugar de BLE
// #define USE_SERIAL_COMM

// Frecuencia de muestreo del IMU (en Hz)
#ifndef FREQ_IMU
#define FREQ_IMU 50
#endif

// Latencia del buffer de hardware del IMU (en milisegundos, 2 segundos)
#define LATENCIA_IMU 2000

// Longitud de los buffers de sensores (en segundos)
#ifndef LEN_BUFFER_IMU_SEG
#define LEN_BUFFER_IMU_SEG 12
#endif

#ifndef LEN_BUFFER_PRESSURE_SEG
#define LEN_BUFFER_PRESSURE_SEG 12
#endif

// Frecuencia de muestreo del Barómetro / Presión (en Hz)
#ifndef FREQ_PRESSURE
#define FREQ_PRESSURE 16
#endif

// Tamaño de los buffers circulares (FIFOs)
#define IMU_FIFO_SIZE (FREQ_IMU * LEN_BUFFER_IMU_SEG)
#define PRESSURE_FIFO_SIZE (FREQ_PRESSURE * LEN_BUFFER_PRESSURE_SEG)

// Tamaño de la ventana donde se procesa la posible caida.
#define IMPACT_WINDOW_SIZE 200 // 4 segundos a 50 Hz

// Duracion de la ventana de impacto (en segundos) para umbral de retardo en envios
#define IMPACT_WINDOW_TIME_SEC ((float)IMPACT_WINDOW_SIZE / (float)FREQ_IMU)

// Posicion a partir de la cual se obtiene la ventana de procesamiento.
#define START_IMPACT_WINDOW_INDEX ((IMU_FIFO_SIZE / 2) - (IMPACT_WINDOW_SIZE / 2))

// Posicion de la ventana intermedia para procesamiento en WAKEUP (50% de solapamiento entre ventana inicial y central)
#define INTERMEDIATE_IMPACT_WINDOW_INDEX (START_IMPACT_WINDOW_INDEX / 2)

// Tamaño de la media movil que recorre la ventana de impacto (se calcula a partir del shift).
#define SLIDING_WINDOW_SHIFT 3 // Shift para promediar con bitshift
#define SLIDING_WINDOW_SIZE (1 << SLIDING_WINDOW_SHIFT) // 2^3 = 8

// Valor umbral maximo a partir del cual se consigera que hubo caida libre.
#define THRESHOLD_FREE_FALL  2600// 2300 // 2000 //1500  // Valor provisorio

// Valor umbral minimo a partir del cual se considera que hubo un impacto.
#define THRESHOLD_IMPACT 8000// 6500 // 7000     // Valor provisorio

// --- Umbrales para Detección de Reposo e Inmovilidad (MAD sobre magnitudes) ---
#define THRESHOLD_NO_MOVEMENT_LOW   80.0f   // LSB de MAD: persona quieta (sueño / reposo relativo)
#define THRESHOLD_NO_MOVEMENT_HIGH  3.0f   // LSB de MAD: reposo estricto (mesa / dispositivo quitado)
#define ALPHA_FILTER_NO_MOVEMENT    0.1f    // Factor de suavizado para media y MAD

// numero de veces que se procesa el buffer recibido de IMU luego de recibir un evento de "No caida".
#define NUM_PROCESS_POST_NOMOTION 20

// =============================================================================
// --- Configuración de Autenticación BLE (Zero Flash Writes) ---
// =============================================================================
inline const char* getBraceletUid() {
    static char uid[16] = {0};
    if (uid[0] == '\0') {
        // Se obtiene el número de serie único de 64 bits del silicio nRF52 (FICR)
        uint32_t combined = NRF_FICR->DEVICEID[0] ^ NRF_FICR->DEVICEID[1];
        uint16_t short_id = (uint16_t)((combined & 0xFFFF) ^ ((combined >> 16) & 0xFFFF));
        snprintf(uid, sizeof(uid), "SB-%04X", short_id);
    }
    return uid;
}

#ifndef BRACELET_UID
#define BRACELET_UID (getBraceletUid())
#endif

#define BLE_AUTH_TIMEOUT_MS   10000 // 10 segundos para completar autenticacion
#define BLE_AUTH_MAX_ATTEMPTS 3     // Maximo de intentos fallidos antes de desconectar

// =============================================================================
// --- Configuración de Control de Dispositivo Quitado (Off-Wrist) ---
// =============================================================================
#define OFF_WRIST_PROBE_INTERVAL_SEC  60
#define OFF_WRIST_CONSECUTIVE_TARGET  5

enum WarningId : uint8_t {
    WARN_DEVICE_OFF_WRIST   = 0x01,
    WARN_DEVICE_ON_WRIST    = 0x02
};

// =============================================================================
// --- Configuración de Colas RTOS (Mailbox) ---
// =============================================================================
#ifndef TASK_QUEUE_SIZE
#define TASK_QUEUE_SIZE 6 // 6 mensajes por cola (ahorro de RAM estática y margen holgado)
#endif

#ifndef SYSTEM_TASK_QUEUE_SIZE
#define SYSTEM_TASK_QUEUE_SIZE (TASK_QUEUE_SIZE * 2) // 12 mensajes para SystemTask (orquestador)
#endif

// =============================================================================
// --- Tamaños de Stack para Tareas RTOS (en bytes) ---
// =============================================================================
// REGLA DE DISEÑO Y PRESUPUESTO DE MEMORIA (nRF52832 - 64 KB RAM):
// 1. Contexto: Mbed OS asigna por defecto 4096 B por hilo (20.480 B para 5 tareas).
//    Junto con la memoria estática (~45.7 KB) y el buffer contiguo obligatorio de
//    Cordio BLE (13.000 B en Heap para NRFCordioHCIDriver::_stack_buffer), la RAM
//    física requiere un dimensionamiento exacto de pilas de hilos.
// 2. Granularidad: Se asigna memoria en múltiplos de 256 B (0.25 KB), respetando
//    estrictamente el alineamiento de 8 bytes de la arquitectura ARM Cortex-M (AAPCS).
// 3. Justificación por tarea:
//    - PMIC (512 B): Ejecuta un ping I2C cada 40s sin anidamiento profundo (consumo <100 B).
//    - NOTIFY_TELEMETRY (768 B): Manejo de mailbox, control de GPIO/PWM (LEDs y vibrador).
//    - ALARMS_EVENTS (1280 B): Algoritmo de caída libre e impacto. Reserva un array
//      local 'uint16_t magnitudes[300]' (600 B) en la pila dentro de evaluateWindow().
//    - SYSTEM (1024 B): Orquestador central. Maneja tickers de hardware, máquinas de
//      estado (BHI260, botones) y llamadas a repositorios.
//    - COMM_LINK (1280 B): Tarea con mayor profundidad de llamada (árbol de ejecución
//      que desciende por BleDriver, BLE.poll(), ArduinoBLE y capa HCI Cordio).
// =============================================================================
#define STACK_SIZE_PMIC_TASK             768
#define STACK_SIZE_NOTIFY_TELEMETRY_TASK 1024
#define STACK_SIZE_ALARMS_EVENTS_TASK    1280
#define STACK_SIZE_SYSTEM_TASK           1536
#define STACK_SIZE_COMM_LINK_TASK        1536
