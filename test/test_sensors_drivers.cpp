#include <Arduino.h>
#include "test_sensors_drivers.h"
#include "utils/debug.h"
#include "drivers/bhi260_driver.h"
#include "repositories/imu_repository.h"
#include "repositories/pressure_repository.h"
#include "repositories/temperature_repository.h"
#include "repositories/event_repository.h"

// Utilizamos SENSOR_ID_ANY_MOTION_WU (143) y SENSOR_ID_STATIONARY_DET (75) que ya vienen nativos en Arduino_BHY2

// Flag volátil para la interrupción
volatile bool isrFlag = false;

// Handler ISR físico
void onBhiInt() {
    isrFlag = true;
    DEBUG_PRINTLN("");
}

void runSensorsDriversTest() {
    DEBUG_PRINTLN("\n================================================");
    DEBUG_PRINTLN("--- INICIANDO TEST MULTI-SENSOR DRIVERS ---");
    
    BHI260Driver* bhi = BHI260Driver::getInstance();
    bhi->init();
    // 1. Instanciar todos los drivers (Singletons)
    ImuRepository* imuDriver = ImuRepository::getInstance();
    PressureRepository* pressureDriver = PressureRepository::getInstance();
    TemperatureRepository* tempDriver = TemperatureRepository::getInstance();
    
    // Instanciamos los de evento (WAKE UP)
    MotionRepository* motionEvent = MotionRepository::getInstance();
    NoMotionRepository* noMotionEvent = NoMotionRepository::getInstance(); 

    // 2. Configurar y encender los sensores en el BHI260
    DEBUG_PRINTLN("[DEBUG] Configurando Sensores Continuos (NON WAKE-UP)...");
    imuDriver->begin((float)FREQ_IMU, 3000);       // IMU a 50Hz, 3s latencia
    pressureDriver->begin((float)FREQ_PRESSURE,(uint32_t) -1);  // Barómetro a 16Hz, no interrumpe
    tempDriver->begin(1.0f,(uint32_t) -1);       // Temperatura a 1Hz

    DEBUG_PRINTLN("[DEBUG] Configurando Sensores de Evento (WAKE-UP)...");
    motionEvent->begin(1.0f, 0);
    noMotionEvent->begin(1.0f, 0);

    // Esperamos para que el sensor aplique internamente la configuración
    delay(100);

    // Consultamos la configuración real que quedó aplicada
    SensorConfig imuCfg = imuDriver->getConfiguration();
    DEBUG_PRINT("[DEBUG] CONFIG IMU -> Sample Rate: ");
    DEBUG_PRINT(imuCfg.sample_rate);
    DEBUG_PRINT(" Hz, Latency: ");
    DEBUG_PRINT(imuCfg.latency);
    DEBUG_PRINT(" ms, Range: ");
    DEBUG_PRINTLN(imuCfg.range);

    SensorConfig pressCfg = pressureDriver->getConfiguration();
    DEBUG_PRINT("[DEBUG] CONFIG PRESSURE -> Sample Rate: ");
    DEBUG_PRINT(pressCfg.sample_rate);
    DEBUG_PRINT(" Hz, Latency: ");
    DEBUG_PRINT(pressCfg.latency);
    DEBUG_PRINT(" ms, Range: ");
    DEBUG_PRINTLN(pressCfg.range);

    // 3. Configurar Interrupción usando la librería del BHI260
    bhi->configureInterrupt(onBhiInt);

    // 4. Limpiar FIFOs basuras iniciales
    bhi->flushFIFOs();
    isrFlag = false;

    // 5. Estado Inicial: Apagamos el recolector de FIFO Non-WakeUp
    DEBUG_PRINTLN("[DEBUG] ESTADO INICIAL: Disable Non-Wakeup FIFO.");
    bhi->disableNonWakeupFIFO();
    bool isNonWakeupActive = false;

    DEBUG_PRINTLN("[DEBUG] Esperando por eventos de movimiento... (Test dura 30 seg)");
    
    unsigned long startTime = millis();
    while (millis() - startTime < 50000) {
        
        // Atender interrupción de la IMU
        if (isrFlag) {
            isrFlag = false;
            
            // Leemos los paquetes disponibles en el hardware
            bhi->updateFifoData();
            
            // Chequear eventos de Wake Up
            if (motionEvent->hasEventOccurred()) {
                DEBUG_PRINTLN("[EVENTO] -> MOTION DETECTADO!");
                motionEvent->clearEventFlag();
                noMotionEvent->begin(1.0f, 0);
                
    
                
                // Si detecta movimiento, despertamos (activamos la lectura Non-WakeUp)
                if (!isNonWakeupActive) {
                    DEBUG_PRINTLN("         -> Habilitando Non-Wakeup FIFO (IMU/Press/Temp on)");
                    bhi->enableNonWakeupFIFO();
                    bhi->updateFifoData();
                    imuDriver->fifoFlush();
                    pressureDriver->fifoFlush();
                    isNonWakeupActive = true;
                }
            }

            if (noMotionEvent->hasEventOccurred()) {
                DEBUG_PRINTLN("[EVENTO] -> NO MOTION (Stationary) DETECTADO!");
                noMotionEvent->clearEventFlag();
                motionEvent->begin(1.0f, 0);
                
                // Si la placa se queda quieta, volvemos a suspender los datos continuos para ahorrar batería
                if (isNonWakeupActive) {
                    DEBUG_PRINTLN("         -> Deshabilitando Non-Wakeup FIFO (Ahorro de energia)");
                    bhi->disableNonWakeupFIFO();
                    isNonWakeupActive = false;
                }
            }
            
            // Procesamos lo que haya ingresado en los buffers
            // Verificamos llenado de los buffers de las clases wrapper
            if(isNonWakeupActive){ 
                DEBUG_PRINT("IMU Datos Push: ");
                DEBUG_PRINTLN(imuDriver->getTotalPushed());
                DEBUG_PRINT("Press Datos Push: ");
                DEBUG_PRINTLN(pressureDriver->getTotalPushed());
            }
            if (imuDriver->isFull()) {
                DEBUG_PRINTLN("[BUFF LLENO] Sensor IMU saturado. Procediendo a vaciar...");
                imuDriver->fifoFlush();
            }

            if (pressureDriver->isFull()) {
                DEBUG_PRINTLN("[BUFF LLENO] Sensor PRESION saturado. Procediendo a vaciar...");
                pressureDriver->fifoFlush();
            }

            //Consumimos temperatura si hay nueva para limpiar su flag
            if (tempDriver->isUpdated()) {
                DEBUG_PRINT("Temperatura Actualizada: ");
                DEBUG_PRINTLN(tempDriver->getTemp());
            }
            
            
            //bhi->enableInterrupt();
        }
    }

    // 6. Apagar sensores al terminar
    imuDriver->end();
    pressureDriver->end();
    // tempDriver->end();
    motionEvent->end();
    noMotionEvent->end();
    
    bhi->enableNonWakeupFIFO(); // Dejarlo prendido por las dudas para el resto del firmware
    
    DEBUG_PRINTLN("--- FIN DEL TEST MULTI-SENSOR ---");
    DEBUG_PRINTLN("================================================\n");
}
