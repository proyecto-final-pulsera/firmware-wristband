#include <Arduino.h>
#include "test_imu_fifo.h"
#include "drivers/imu_sensor_driver.h"
#include "drivers/bhi260_driver.h"
#include <BoschSensortec.h>

// Handler para la interrupción (resuelto en el test)
void onSensorInterrupt() {
    BHI260Driver::getInstance()->handleInterrupt();
}

void runImuFifoTest() {
    Serial.println("\n================================================");
    Serial.println("--- INICIANDO TEST DE FIFO IMU (10 SEGUNDOS) ---");
    
    BHI260Driver* imu = BHI260Driver::getInstance();
    
    // Instanciamos el objeto de manera ESTATICA para no desbordar el Stack del main thread
    static ImuSensorDriver imuSensor;
    
    Serial.println("[DEBUG] Etapa 1/6: Configurando Acelerometro a 50Hz...");
    imuSensor.begin(50.0f, 0); 
    
    Serial.println("[DEBUG] Etapa 1.5/6: Configurando Tilt Detector (Wakeup)...");
    SensorConfigurationPacket cfgTilt;
    cfgTilt.sensorId = SENSOR_ID_TILT_DETECTOR;
    cfgTilt.sampleRate = 1.0f;
    cfgTilt.latency = 0;
    imu->configureSensor(cfgTilt);

    // Configurar la interrupción dentro del test
    imu->configureInterrupt(onSensorInterrupt);
    
    // Vaciamos basura residual de las FIFOs antes de arrancar
    imu->flushFIFOs();
    imuSensor.fifoFlush();
    imu->clearInterruptFlag();

    Serial.println("[DEBUG] Etapa 2/6: Desactivando host (AP Suspend).");
    Serial.println("        -> El MCU no recibira interrupciones de datos normales (Accel).");
    Serial.println("        -> PERO si inclinas la placa (Tilt), deberia disparar la ISR!");
    
    imu->suspendHost();

    Serial.println("[DEBUG] Etapa 3/6: Esperando 10 segundos. Mové e incliná la placa ahora...");
    // 3. Pasado 10 segundos, monitoreamos el flag
    unsigned long start =  millis();
    while(millis() - start <= 10000){
        if(imu->isInterruptTriggered()) {
            imu->disableInterrupt();
            imu->updateFifoData(); 
            Serial.println("[TEST] -> ¡Interrupción disparada detectada por el handler!");
            if (imu->availableSensorData()){
                Serial.print("ID");
                SensorDataPacket data;
                imu->readSensorData(data);
                Serial.print(data.sensorId);
            }
            imu->clearInterruptFlag();
            imu->enableInterrupt();
        }
        delay(100);
    }

    Serial.println("[DEBUG] Etapa 4/6: Despertando host (AP Resume) para permitir lectura masiva...");
    // 4. Indicamos al sensor que el host está despierto
    imu->resumeHost();

    Serial.println("[DEBUG] Etapa 5/6: Parseando los datos acumulados de la FIFO fisica...");
    imu->updateFifoData(); 
    
    // 5. Mostrar tamaño de la FIFO de la clase
    uint16_t count = imuSensor.getAvailableCount();
    Serial.print("[DEBUG] Etapa 6/6: Elementos recolectados en la FIFO de memoria: ");
    Serial.println(count);
    
    if (count == 0) {
        Serial.println("        -> ADVERTENCIA: La FIFO esta vacia. Verifica si la placa se movio.");
    }

    // Mostrar los datos como X - Y - Z
    Serial.println("\n--- DATOS DE LA FIFO ---");
    while(imuSensor.getAvailableCount() > 0) {
        DataXYZ data;
        if(imuSensor.pop(data)) {
            Serial.print(data.x);
            Serial.print(" - ");
            Serial.print(data.y);
            Serial.print(" - ");
            Serial.println(data.z);
        }
    }
    Serial.println("------------------------");

    // Desuscribimos y apagamos
    imuSensor.end();
    cfgTilt.sampleRate = 0.0f; // Apagamos el Tilt
    imu->configureSensor(cfgTilt);
    
    Serial.println("================================================\n");
}

void runFifoDepthTest() {
    Serial.println("\n================================================");
    Serial.println("--- INICIANDO TEST DE PROFUNDIDAD HW FIFO ---");
    Serial.println("-> Objetivo: Medir cuantos paquetes retiene el BHI260.");
    
    BHI260Driver* imu = BHI260Driver::getInstance();
    
    // Instancia estática para no volar el stack
    static ImuSensorDriver imuSensor;
    
    // Lo configuramos a una altísima velocidad: 200 Hz
    Serial.println("[DEBUG] Etapa 1: Configurando Acelerometro a 200Hz...");
    imuSensor.begin(200.0f, 0); 
    
    // Vaciamos basuras y reseteamos el contador de test
    imu->flushFIFOs();
    imuSensor.fifoFlush();
    imuSensor.resetTotalPushed();

    Serial.println("[DEBUG] Etapa 2: Apagando host y esperando 30 SEGUNDOS...");
    Serial.println("        -> Esto forzará el desborde del buffer interno del BHI260.");
    
    imu->suspendHost();
    
    // Esperamos 30 segundos
    // En 30 segundos a 200 Hz = 6000 muestras = ~48 KB de datos.
    delay(30000);

    Serial.println("[DEBUG] Etapa 3: Despertando host y parseando TODO el buffer...");
    imu->resumeHost();
    
    // La función interna de Bosch (bhy2_get_and_process_fifo) ya tiene un ciclo while
    // interno que lee bloques del tamaño del work_buffer (2048b) y los parsea
    // repetidamente hasta que la FIFO física se vacía por completo.
    // Por ende, un solo llamado a updateFifoData() extrae todo el historial acumulado.
    imu->updateFifoData(); 

    Serial.print("[DEBUG] -> Paquetes totales extraidos de la HW FIFO: ");
    Serial.println(imuSensor.getTotalPushed());
    
    // Apagamos todo
    imu->suspendHost();
    imuSensor.end();
    
    Serial.println("================================================\n");
}
