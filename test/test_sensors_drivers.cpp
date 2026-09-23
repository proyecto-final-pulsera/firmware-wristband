#include <Arduino.h>
#include "test_sensors_drivers.h"

#include "drivers/bhi260_driver.h"
#include "drivers/imu_sensor_driver.h"
#include "drivers/pressure_sensor_driver.h"
#include "drivers/temperature_sensor_driver.h"
#include "drivers/event_sensor_driver.h"

// Utilizamos SENSOR_ID_ANY_MOTION_WU (143) y SENSOR_ID_STATIONARY_DET (75) que ya vienen nativos en Arduino_BHY2

// Flag volátil para la interrupción
volatile bool isrFlag = false;

// Handler ISR físico
void onBhiInt() {
    isrFlag = true;
    Serial.println("");
}

void runSensorsDriversTest() {
    Serial.println("\n================================================");
    Serial.println("--- INICIANDO TEST MULTI-SENSOR DRIVERS ---");
    
    BHI260Driver* bhi = BHI260Driver::getInstance();

    // 1. Instanciar todos los drivers (Static para no desbordar el stack del main)
    static ImuSensorDriver imuDriver;
    static PressureSensorDriver pressureDriver;
    // static TemperatureSensorDriver tempDriver;
    
    // Instanciamos los de evento (WAKE UP)
    static EventSensorDriver motionEvent(SENSOR_ID_MOTION_DET);
    static EventSensorDriver noMotionEvent(SENSOR_ID_STATIONARY_DET); 

    // 2. Configurar y encender los sensores en el BHI260
    Serial.println("[DEBUG] Configurando Sensores Continuos (NON WAKE-UP)...");
    imuDriver.begin(50.0f, 10000);       // IMU a 50Hz
    pressureDriver.begin(10.0f, 25000);  // Barómetro a 10Hz
    // tempDriver.begin(1.0f, 0);       // Temperatura a 1Hz

    Serial.println("[DEBUG] Configurando Sensores de Evento (WAKE-UP)...");
    motionEvent.begin(1.0f, 0);
    noMotionEvent.begin(1.0f, 0);

    // Esperamos para que el sensor aplique internamente la configuración
    delay(100);

    // Consultamos la configuración real que quedó aplicada
    SensorConfig imuCfg = imuDriver.getConfiguration();
    Serial.print("[DEBUG] CONFIG IMU -> Sample Rate: ");
    Serial.print(imuCfg.sample_rate);
    Serial.print(" Hz, Latency: ");
    Serial.print(imuCfg.latency);
    Serial.print(" ms, Range: ");
    Serial.println(imuCfg.range);

    SensorConfig pressCfg = pressureDriver.getConfiguration();
    Serial.print("[DEBUG] CONFIG PRESSURE -> Sample Rate: ");
    Serial.print(pressCfg.sample_rate);
    Serial.print(" Hz, Latency: ");
    Serial.print(pressCfg.latency);
    Serial.print(" ms, Range: ");
    Serial.println(pressCfg.range);

    // 3. Configurar Interrupción usando la librería del BHI260
    bhi->configureInterrupt(onBhiInt);

    // 4. Limpiar FIFOs basuras iniciales
    bhi->flushFIFOs();
    isrFlag = false;

    // 5. Estado Inicial: Apagamos el recolector de FIFO Non-WakeUp
    Serial.println("[DEBUG] ESTADO INICIAL: Disable Non-Wakeup FIFO.");
    bhi->disableNonWakeupFIFO();
    bool isNonWakeupActive = false;

    Serial.println("[DEBUG] Esperando por eventos de movimiento... (Test dura 30 seg)");
    
    unsigned long startTime = millis();
    while (millis() - startTime < 50000) {
        
        // Atender interrupción de la IMU
        if (isrFlag) {
            isrFlag = false;
            
            // Leemos los paquetes disponibles en el hardware
            bhi->updateFifoData();
            
            // Chequear eventos de Wake Up
            if (motionEvent.hasEventOccurred()) {
                Serial.println("[EVENTO] -> MOTION DETECTADO!");
                motionEvent.clearEventFlag();
                
                // Si detecta movimiento, despertamos (activamos la lectura Non-WakeUp)
                if (!isNonWakeupActive) {
                    Serial.println("         -> Habilitando Non-Wakeup FIFO (IMU/Press/Temp on)");
                    bhi->enableNonWakeupFIFO();
                    bhi->updateFifoData();
                    imuDriver.fifoFlush();
                    pressureDriver.fifoFlush();
                    isNonWakeupActive = true;
                }
            }

            if (noMotionEvent.hasEventOccurred()) {
                Serial.println("[EVENTO] -> NO MOTION (Stationary) DETECTADO!");
                noMotionEvent.clearEventFlag();
                
                // Si la placa se queda quieta, volvemos a suspender los datos continuos para ahorrar batería
                if (isNonWakeupActive) {
                    Serial.println("         -> Deshabilitando Non-Wakeup FIFO (Ahorro de energia)");
                    bhi->disableNonWakeupFIFO();
                    isNonWakeupActive = false;
                }
            }
            
            // Procesamos lo que haya ingresado en los buffers
            // Verificamos llenado de los buffers de las clases wrapper
            if(isNonWakeupActive){ 
                Serial.print("IMU Datos Push: ");
                Serial.println(imuDriver.getTotalPushed());
                Serial.print("Press Datos Push: ");
                Serial.println(pressureDriver.getTotalPushed());
            }
            if (imuDriver.isFull()) {
                Serial.println("[BUFF LLENO] Sensor IMU saturado. Procediendo a vaciar...");
                imuDriver.fifoFlush();
            }

            if (pressureDriver.isFull()) {
                Serial.println("[BUFF LLENO] Sensor PRESION saturado. Procediendo a vaciar...");
                pressureDriver.fifoFlush();
            }

            //Consumimos temperatura si hay nueva para limpiar su flag
            // if (tempDriver.isUpdated()) {
            //     tempDriver.getTemp();
            // }
            
            
            //bhi->enableInterrupt();
        }
    }

    // 6. Apagar sensores al terminar
    imuDriver.end();
    pressureDriver.end();
    // tempDriver.end();
    motionEvent.end();
    noMotionEvent.end();
    
    bhi->enableNonWakeupFIFO(); // Dejarlo prendido por las dudas para el resto del firmware
    
    Serial.println("--- FIN DEL TEST MULTI-SENSOR ---");
    Serial.println("================================================\n");
}
