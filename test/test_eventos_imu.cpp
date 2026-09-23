#include "test_eventos_imu.h"
#include "drivers/bhi260_driver.h"
#include "drivers/interface_driver.h"
#include "SerialMenuDebug.h"
#include "rtos.h"

static BHI260Driver* imu = nullptr;
static InterfaceDriver* interfaceDriver = nullptr;
static SerialMenuDebug menuDebug;

static void enableTestSensor(uint8_t id) {
    SensorConfigurationPacket cfg;
    cfg.sensorId = id;
    cfg.sampleRate = 1.0f;
    cfg.latency = 0;
    if(imu) {
        imu->configureSensor(cfg);
    }
}

static void disableTestSensor(uint8_t id) {
    SensorConfigurationPacket cfg;
    cfg.sensorId = id;
    cfg.sampleRate = 0.0f;
    cfg.latency = 0;
    if(imu) {
        imu->configureSensor(cfg);
    }
}

void runTestEventosImu() {
    imu = BHI260Driver::getInstance();
    
    // IMPORTANTE: Crear la instancia para evitar nullptr
    interfaceDriver = InterfaceDriver::createInstance();
    
    // Test no arrancó todavía
    interfaceDriver->setLedWarn();
    
    Serial.println("\n================================================");
    Serial.println("--- INICIANDO TEST DE EVENTOS IMU (10 SEG) ---");
    Serial.println("Esperando 3 segundos antes de iniciar...");
    delay(3000);
    
    // Se esta ejecutando el test
    interfaceDriver->setLedNotif();
    menuDebug.clear();
    
    uint8_t targetSensors[] = {
        SENSOR_ID_STD, 
        SENSOR_ID_STATIONARY_DET, 
        SENSOR_ID_DEVICE_ORI, 
        SENSOR_ID_WRIST_TILT_GESTURE
    };
    
    for (int i = 0; i < 4; i++) {
        enableTestSensor(targetSensors[i]);
    }
    
    // Limpiamos basura
    imu->flushFIFOs();
    
    Serial.println("--- COMENZANDO SECUENCIA ---");
    unsigned long start = millis();
    while (millis() - start <= 30000) {
        
        imu->updateFifoData();
        
        while (imu->availableSensorData()) {
            SensorDataPacket data;
            if (imu->readSensorData(data)) {
                switch (data.sensorId) {
                    case SENSOR_ID_STD:
                        menuDebug.logEvent("Step Detector");
                        enableTestSensor(data.sensorId); // One-shot auto-rearm
                        break;
                    case SENSOR_ID_STATIONARY_DET:
                        menuDebug.logEvent("Stationary Detect");
                        enableTestSensor(data.sensorId); // One-shot auto-rearm
                        break;
                    case SENSOR_ID_DEVICE_ORI:
                        menuDebug.logEvent("Device Orientation");
                        // On-change, no re-arm needed
                        break;
                    case SENSOR_ID_WRIST_TILT_GESTURE:
                        menuDebug.logEvent("Wrist Tilt");
                        enableTestSensor(data.sensorId); // One-shot auto-rearm
                        break;
                }
            }
        }
        delay(10);
    }
    
    // Apagamos los sensores
    for (int i = 0; i < 4; i++) {
        disableTestSensor(targetSensors[i]);
    }
    
    // El test finalizo
    interfaceDriver->setLedAlarm();
    
    Serial.println("--- FIN DE LA SECUENCIA ---");
    Serial.println("Escriba 'EVENTOS' para ver los resultados.");
    
    // Loop infinito esperando el menú serie
    while (true) {
        menuDebug.processSerial();
        delay(100);
    }
}
