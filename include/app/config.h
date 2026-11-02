#pragma once

// Activar para habilitar los mensajes por Serial (Debug)
// #define DEBUG

// Frecuencia de muestreo del IMU (en Hz)
#ifndef FREQ_IMU
#define FREQ_IMU 50
#endif

// Latencia del buffer de hardware del IMU (en milisegundos, 3 segundos)
#define LATENCIA_IMU 3000

// Longitud del buffer del IMU (en segundos)
#ifndef LEN_BUFFER_IMU_SEG
#define LEN_BUFFER_IMU_SEG 16
#endif

// Tamaño del buffer de IMU.
#define IMU_FIFO_SIZE (FREQ_IMU * LEN_BUFFER_IMU_SEG)

// Tamaño de la ventana donde se procesa la posible caida.
#define IMPACT_WINDOW_SIZE 300 // 6 segundos a 50 Hz

// Duracion de la ventana de impacto (en segundos) para umbral de retardo en envios
#define IMPACT_WINDOW_TIME_SEC ((float)IMPACT_WINDOW_SIZE / (float)FREQ_IMU)

// Posicion a partir de la cual se obtiene la ventana de procesamiento.
#define START_IMPACT_WINDOW_INDEX ((IMU_FIFO_SIZE / 2) - (IMPACT_WINDOW_SIZE / 2))

// Tamaño de la media movil que recorre la ventana de impacto (se calcula a partir del shift).
#define SLIDING_WINDOW_SHIFT 3 // Shift para promediar con bitshift
#define SLIDING_WINDOW_SIZE (1 << SLIDING_WINDOW_SHIFT) // 2^3 = 8

// Valor umbral maximo a partir del cual se consigera que hubo caida libre.
#define THRESHOLD_FREE_FALL 2300 // 2000 //1500  // Valor provisorio

// Valor umbral minimo a partir del cual se considera que hubo un impacto.
#define THRESHOLD_IMPACT 6500 // 7000     // Valor provisorio

// numero de veces que se procesa el buffer recibido de IMU luego de recibir un evento de "No caida".
#define NUM_PROCESS_POST_NOMOTION 10

// =============================================================================
// --- Configuración de Colas RTOS (Mailbox) ---
// =============================================================================
#ifndef TASK_QUEUE_SIZE
#define TASK_QUEUE_SIZE 6 // 6 mensajes por cola (ahorro de RAM estática y margen holgado)
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
#define STACK_SIZE_PMIC_TASK             384
#define STACK_SIZE_NOTIFY_TELEMETRY_TASK 448
#define STACK_SIZE_ALARMS_EVENTS_TASK    1152
#define STACK_SIZE_SYSTEM_TASK           640//640
#define STACK_SIZE_COMM_LINK_TASK        960
