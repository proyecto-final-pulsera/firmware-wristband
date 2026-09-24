#include <Arduino.h>
#include "test_serial_data.h"
#include "drivers/bhi260_driver.h"
#include "drivers/imu_sensor_driver.h"
#include "drivers/pressure_sensor_driver.h"
#include "drivers/event_sensor_driver.h"
#include "drivers/serial_comm_driver.h"

// =============================================================================
// Estructura de "Snapshot" para congelar el evento
// =============================================================================
struct FallSnapshot {
    DataXYZ imu_data[IMU_FIFO_SIZE];
    float pressure_data[PRESSURE_FIFO_SIZE];
};

static FallSnapshot snapshot;

enum TestState {
    WAIT_MOTION,
    WAIT_STILL
};

void runSerialDataTest() {
    Serial.begin(115200);
    while(!Serial);

    Serial.println("\n================================================");
    Serial.println(" INICIANDO TEST INFINITO: MÁQUINA DE CAÍDAS");
    Serial.println("================================================\n");
    
    // Inicializar el core de Bosch
    BHI260Driver* bhi = BHI260Driver::getInstance();
    bhi->init();
    bhi->enableNonWakeupFIFO();

    // Iniciar driver de comunicación
    SerialCommDriver* comm = SerialCommDriver::getInstance();
    comm->begin();

    // Instancias de los Drivers
    static ImuSensorDriver imuDriver;
    static PressureSensorDriver pressureDriver;
    EventSensorDriver motionEvent(SENSOR_ID_MOTION_DET);
    EventSensorDriver noMotionEvent(SENSOR_ID_STATIONARY_DET);

    // Configurar sensores continuos (Non-Wakeup)
    Serial.println("[DEBUG] Configurando sensores continuos (IMU a 50Hz, Presion a 16Hz)...");
    imuDriver.begin((float)FREQ_IMU, 3000); 
    pressureDriver.begin((float)FREQ_PRESSURE, 3000);

    // Configurar eventos virtuales (Wakeup / One-Shot)
    Serial.println("[DEBUG] Configurando detectores de movimiento...");
    motionEvent.begin(1.0f, 0);
    noMotionEvent.begin(1.0f, 0);

    Serial.println("\n[SISTEMA LISTO] -> Estado Inicial: Esperando Movimiento.");
    
    TestState currentState = WAIT_MOTION;

    // Loop infinito del test
    while (true) {
        // 1. Drenar la FIFO física del BHI260 y poblar los buffers circulares
        bhi->updateFifoData();

        // 2. Máquina de Estados de la Caída
        if (currentState == WAIT_MOTION) {
            
            if (motionEvent.hasEventOccurred()) {
                motionEvent.clearEventFlag();
                
                
                //Serial.println("[!] MOVIMIENTO DETECTADO. Entrando a fase 2 (Esperando Quieto)...");
                
                // Limpiamos cualquier falso positivo del evento "Quieto"
                noMotionEvent.clearEventFlag();
                noMotionEvent.begin(1.0f, 0);
                currentState = WAIT_STILL;
            }
            
        } else if (currentState == WAIT_STILL) {
            
            if (noMotionEvent.hasEventOccurred()) {
                noMotionEvent.clearEventFlag();
                
                //Serial.println("[!] QUIETO DETECTADO. ¡Condición de caída cumplida!");
                //Serial.println("[DEBUG] Tomando Snapshot de la memoria...");

                // Congelamos los datos usando memcpy hacia nuestro Snapshot local
                uint16_t imu_len = imuDriver.getFifoValues(snapshot.imu_data, IMU_FIFO_SIZE);
                uint16_t press_len = pressureDriver.getFifoValues(snapshot.pressure_data, PRESSURE_FIFO_SIZE);

                //Serial.print("[DEBUG] Snapshot capturado: ");
                //Serial.print(imu_len); //Serial.print(" samples IMU, ");
                //Serial.print(press_len); //Serial.println(" samples Barómetro.");

                // Enviar los datos por el driver de comunicaciones (Binario)
                //Serial.println(">>> TRANSMITIENDO TRAMA BINARIA IMU...");
                comm->sendPayload(MSG_IMU_BUFFER, (const uint8_t*)snapshot.imu_data, imu_len * sizeof(DataXYZ));

                //Serial.println(">>> TRANSMITIENDO TRAMA BINARIA PRESIÓN...");
                comm->sendPayload(MSG_PRESSURE_BUFFER, (const uint8_t*)snapshot.pressure_data, press_len * sizeof(float));

                //Serial.println(">>> TRANSMISIÓN COMPLETADA.\n");
                //Serial.println("[SISTEMA] -> Volviendo a fase 1 (Esperando Movimiento).\n");
                
                // Reseteamos el flag de movimiento por si saltó mientras transmitíamos
                motionEvent.clearEventFlag();
                motionEvent.begin(1.0f, 0);
                currentState = WAIT_MOTION;
            }
        }
        
        // Pequeño retardo para no saturar el loop
        delay(10);
    }
}
