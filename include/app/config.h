#pragma once

// Activar para habilitar los mensajes por Serial (Debug)
#define DEBUG

// Frecuencia de muestreo del IMU (en Hz)
#ifndef FREQ_IMU
#define FREQ_IMU 50
#endif

// Longitud del buffer del IMU (en segundos)
#ifndef LEN_BUFFER_IMU_SEG
#define LEN_BUFFER_IMU_SEG 16
#endif

// Tamaño del buffer de IMU.
#define IMU_FIFO_SIZE (FREQ_IMU * LEN_BUFFER_IMU_SEG)

// Tamaño de la ventana donde se procesa la posible caida.
#define IMPACT_WINDOW_SIZE 300 // 6 segundos a 50 Hz

// Posicion a partir de la cual se obtiene la ventana de procesamiento.
#define START_IMPACT_WINDOW_INDEX ((IMU_FIFO_SIZE / 2) - (IMPACT_WINDOW_SIZE / 2))

// Tamaño de la media movil que recorre la ventana de impacto (se calcula a partir del shift).
#define SLIDING_WINDOW_SHIFT 3 // Shift para promediar con bitshift
#define SLIDING_WINDOW_SIZE (1 << SLIDING_WINDOW_SHIFT) // 2^3 = 8

// Valor umbral maximo a partir del cual se consigera que hubo caida libre.
#define THRESHOLD_FREE_FALL 1500  // Valor provisorio

// Valor umbral minimo a partir del cual se considera que hubo un impacto.
#define THRESHOLD_IMPACT 7000     // Valor provisorio
