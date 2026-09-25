#include <Arduino.h>
#include "test_virtual_sensors.h"
#include "drivers/bhi260_driver.h"

static BHI260Driver* imu_test_ptr = nullptr;

// Función auxiliar para habilitar/re-habilitar sensores
static void enableTestSensor(uint8_t id, float sampleRate = 25.0f) {
  SensorConfigurationPacket cfg;
  cfg.sensorId = id;
  cfg.sampleRate = sampleRate; 
  cfg.latency = 0;
  if(imu_test_ptr) {
    imu_test_ptr->configureSensor(cfg);
  }
}

void runVirtualSensorsTest() {
  imu_test_ptr = BHI260Driver::getInstance();

  // Array de sensores a probar
  uint8_t testSensors[] = {
      SENSOR_ID_STC, 
      SENSOR_ID_STD, 
      SENSOR_ID_TILT_DETECTOR, 
      SENSOR_ID_WAKE_GESTURE, 
      SENSOR_ID_GLANCE_GESTURE, 
      SENSOR_ID_PICKUP_GESTURE, 
      SENSOR_ID_WRIST_TILT_GESTURE, 
      SENSOR_ID_DEVICE_ORI, 
      SENSOR_ID_STATIONARY_DET, 
      SENSOR_ID_MOTION_DET
  };

  Serial.println("--- INICIANDO TEST DE SENSORES DE EVENTO ---");

  for (int i = 0; i < sizeof(testSensors)/sizeof(uint8_t); i++) {
    uint8_t id = testSensors[i];
    bool present = imu_test_ptr->hasSensor(id);
    Serial.print("Sensor ID "); Serial.print(id);
    Serial.print(" presente en FW: "); Serial.println(present ? "SI" : "NO");
    
    if (present) {
      // Habilitar todos con 1.0f
      enableTestSensor(id, 25.0f);
    }
  }

  // Vaciamos FIFOs iniciales (meta eventos) para obligar al pin físico a bajar a LOW
  imu_test_ptr->flushFIFOs();
  
  Serial.println("--- LISTO PARA RECIBIR EVENTOS ---");
}

void loopVirtualSensorsTest() {
  if (!imu_test_ptr) return;

  // 1. OBTENER DATOS DE LA FIFO FÍSICA
  imu_test_ptr->updateFifoData();

  // 2. Procesar todos los paquetes disponibles
  while (imu_test_ptr->availableSensorData()) {
    SensorDataPacket data;
    if (imu_test_ptr->readSensorData(data)) {
      
      switch (data.sensorId) {
        case SENSOR_ID_STC: Serial.println(">>> EVENTO 53: Step Counter Wakeup!"); break;
        case SENSOR_ID_STD: Serial.println(">>> EVENTO 94: Step Detector Wakeup!"); break;
        case SENSOR_ID_TILT_DETECTOR: Serial.println(">>> EVENTO 48: Tilt Detector!"); break;
        case SENSOR_ID_WAKE_GESTURE: Serial.println(">>> EVENTO 57: Wake Gesture!"); break;
        case SENSOR_ID_GLANCE_GESTURE: Serial.println(">>> EVENTO 59: Glance Gesture!"); break;
        case SENSOR_ID_PICKUP_GESTURE: Serial.println(">>> EVENTO 61: Pickup Gesture!"); break;
        case SENSOR_ID_WRIST_TILT_GESTURE: Serial.println(">>> EVENTO 67: Wrist Tilt Gesture!"); break;
        case SENSOR_ID_DEVICE_ORI: Serial.println(">>> EVENTO 70: Device Orientation Wakeup!"); break;
        case SENSOR_ID_STATIONARY_DET: Serial.println(">>> EVENTO 75: Stationary Detect!"); break;
        case SENSOR_ID_MOTION_DET: Serial.println(">>> EVENTO 77: Motion Detect!"); break;
      }

      // Si el sensor es One-Shot, se auto-desactiva al dispararse.
      // En la arquitectura de Bosch BHY2, los sensores con payload PEVENT (como 75 y 77) operan como One-Shot mecánicamente.
      // Excluimos solo 53 (Counter) y 70 (Orientation) que no envían PEVENT.
      if( data.sensorId == SENSOR_ID_MOTION_DET) enableTestSensor(SENSOR_ID_STATIONARY_DET);
      if( data.sensorId == SENSOR_ID_STATIONARY_DET) enableTestSensor(SENSOR_ID_MOTION_DET);
      if (data.sensorId != SENSOR_ID_STC 
        && data.sensorId != SENSOR_ID_DEVICE_ORI
        && data.sensorId != SENSOR_ID_STATIONARY_DET
        && data.sensorId != SENSOR_ID_MOTION_DET
      ) {
          enableTestSensor(data.sensorId);
      }
    }
  }
}
