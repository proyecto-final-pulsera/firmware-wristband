#include <Arduino.h>
#include "test_imu_fifo.h"
#include "repositories/imu_repository.h"
#include "drivers/bhi260_driver.h"
#include <BoschSensortec.h>
#include "utils/debug.h"

// Variables for interrupt testing
volatile bool testInterruptTriggered = false;
void onSensorInterrupt() {
    testInterruptTriggered = true;
}

void runImuFifoTest() {
    DEBUG_PRINTLN("\n================================================");
    DEBUG_PRINTLN("--- INICIANDO TEST DE FIFO IMU (10 SEGUNDOS) ---");
    
    BHI260Driver* imu = BHI260Driver::getInstance();
    imu->init();
    // Obtenemos el singleton
    static ImuRepository* imuSensor = ImuRepository::getInstance();
    
    DEBUG_PRINTLN("[DEBUG] Etapa 1/6: Configurando Acelerometro a 50Hz...");
    imuSensor->begin(50.0f, 0); 
    
    DEBUG_PRINTLN("[DEBUG] Etapa 1.5/6: Configurando Tilt Detector (Wakeup)...");
    SensorConfigurationPacket cfgTilt;
    cfgTilt.sensorId = SENSOR_ID_TILT_DETECTOR;
    cfgTilt.sampleRate = 1.0f;
    cfgTilt.latency = 0;
    imu->configureSensor(cfgTilt);

    // Configurar la interrupción dentro del test
    imu->configureInterrupt(onSensorInterrupt);
    
    // Vaciamos basura residual de las FIFOs antes de arrancar
    imu->flushFIFOs();
    imuSensor->fifoFlush();
    testInterruptTriggered = false;

    DEBUG_PRINTLN("[DEBUG] Etapa 2/6: Desactivando host (AP Suspend).");
    DEBUG_PRINTLN("        -> El MCU no recibira interrupciones de datos normales (Accel).");
    DEBUG_PRINTLN("        -> PERO si inclinas la placa (Tilt), deberia disparar la ISR!");
    
    imu->disableNonWakeupFIFO();

    DEBUG_PRINTLN("[DEBUG] Etapa 3/6: Esperando 10 segundos. Mové e incliná la placa ahora...");
    // 3. Pasado 10 segundos, monitoreamos el flag
    unsigned long start =  millis();
    while(millis() - start <= 15000){
        if(testInterruptTriggered) {
            imu->disableInterrupt();
            imu->updateFifoData(); 
            DEBUG_PRINTLN("[TEST] -> ¡Interrupción disparada detectada por el handler!");
            if (imu->availableSensorData()){
                DEBUG_PRINT("ID");
                SensorDataPacket data;
                imu->readSensorData(data);
                DEBUG_PRINT(data.sensorId);
            }
            testInterruptTriggered = false;
            imu->enableInterrupt();
        }
        delay(100);
    }

    DEBUG_PRINTLN("[DEBUG] Etapa 4/6: Despertando host (AP Resume) para permitir lectura masiva...");
    // 4. Indicamos al sensor que el host está despierto
    imu->enableNonWakeupFIFO();

    DEBUG_PRINTLN("[DEBUG] Etapa 5/6: Parseando los datos acumulados de la FIFO fisica...");
    imu->updateFifoData(); 
    
    // 5. Mostrar tamaño de la FIFO de la clase
    uint16_t count = imuSensor->getAvailableCount();
    DEBUG_PRINT("[DEBUG] Etapa 6/6: Elementos recolectados en la FIFO de memoria: ");
    DEBUG_PRINTLN(count);
    
    if (count == 0) {
        DEBUG_PRINTLN("        -> ADVERTENCIA: La FIFO esta vacia. Verifica si la placa se movio.");
    }

    // Mostrar los datos como X - Y - Z
    DEBUG_PRINTLN("\n--- DATOS DE LA FIFO ---");
    while(imuSensor->getAvailableCount() > 0) {
        DataXYZ data;
        if(imuSensor->pop(data)) {
            DEBUG_PRINT(data.x);
            DEBUG_PRINT(" - ");
            DEBUG_PRINT(data.y);
            DEBUG_PRINT(" - ");
            DEBUG_PRINTLN(data.z);
        }
    }
    DEBUG_PRINTLN("------------------------");

    // Desuscribimos y apagamos
    imuSensor->end();
    cfgTilt.sampleRate = 0.0f; // Apagamos el Tilt
    imu->configureSensor(cfgTilt);
    
    DEBUG_PRINTLN("================================================\n");
}

void runFifoDepthTest() {
    DEBUG_PRINTLN("\n================================================");
    DEBUG_PRINTLN("--- INICIANDO TEST DE PROFUNDIDAD HW FIFO ---");
    DEBUG_PRINTLN("-> Objetivo: Medir cuantos paquetes retiene el BHI260.");
    
    BHI260Driver* imu = BHI260Driver::getInstance();
    
    // Obtenemos el singleton
    ImuRepository* imuSensor = ImuRepository::getInstance();
    
    // Lo configuramos a una altísima velocidad: 800 Hz
    DEBUG_PRINTLN("[DEBUG] Etapa 1: Configurando Acelerometro a 800Hz...");
    imuSensor->begin(800.0f, 0); 
    
    // Vaciamos basuras y reseteamos el contador de test
    imu->flushFIFOs();
    imuSensor->fifoFlush();
    imuSensor->resetTotalPushed();

    DEBUG_PRINTLN("[DEBUG] Etapa 2: Apagando host y esperando 10 SEGUNDOS...");
    DEBUG_PRINTLN("        -> Esto forzará el desborde del buffer interno del BHI260.");
    
    imu->disableNonWakeupFIFO();
    
    // Esperamos 10 segundos
    // En 10 segundos a 800 Hz = 8000 muestras
    delay(10000);

    DEBUG_PRINTLN("[DEBUG] Etapa 3: Despertando host y parseando TODO el buffer...");
    imu->enableNonWakeupFIFO();
    
    // La función interna de Bosch (bhy2_get_and_process_fifo) ya tiene un ciclo while
    // interno que lee bloques del tamaño del work_buffer (2048b) y los parsea
    // repetidamente hasta que la FIFO física se vacía por completo.
    // Por ende, un solo llamado a updateFifoData() extrae todo el historial acumulado.
    imu->updateFifoData(); 

    DEBUG_PRINT("[DEBUG] -> Paquetes totales extraidos de la HW FIFO: ");
    DEBUG_PRINTLN(imuSensor->getTotalPushed());
    
    // Apagamos todo
    imu->disableNonWakeupFIFO();
    imuSensor->end();
    
    DEBUG_PRINTLN("================================================\n");
}
