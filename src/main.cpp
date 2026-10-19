#include <Arduino.h>
#include "app/app.h"
#include "rtos.h"


#include <ArduinoBLE.h>

void setup() {
    Serial.begin(115200);
    
    App::init();
}

void loop() {
    rtos::ThisThread::sleep_for(std::chrono::milliseconds(50));
}


// #include <Arduino.h>
// #include "Nicla_System.h"
// #include "drivers/ble_driver.h"
// #include "app/app.h"

// BleDriver* bleComm;

// // Variables para pruebas de tiempo
// unsigned long lastTick = 0;
// unsigned long lastHugeBuffer = 0;

// Simulamos el "peor caso": Buffer gigante de 5314 bytes (16 segs de datos)
/*
*   IMU: 2 bytes x 3 canales x 16 seg a 50 Hz
*   BAR: 2 bytes x 1 canal x 16 seg a 16 Hz
*   TMP: 2 bytes x 1 canal (solo un dato - promedio)
*/
// uint8_t testBuffer[5314];
// bool pendingImuRetry = false;

// void onRxReceived() {
//     CommPacket pkt;
    
//     // Llamar a processRxData para desempaquetar
//     if (bleComm->processRxData(&pkt)) {
//         Serial.println("\n--- [DEBUG] Paquete RX Recibido ---");
//         Serial.print("Raw Bytes: [0x");
//         if (pkt.type < 16) Serial.print("0");
//         Serial.print(pkt.type, HEX);
        
//         for(int i = 0; i < pkt.length; i++) {
//             Serial.print(", 0x");
//             if (pkt.payload[i] < 16) Serial.print("0");
//             Serial.print(pkt.payload[i], HEX);
//         }
//         Serial.println("]");

//         if (pkt.type == MSG_ACK && pkt.length >= 2) {
//             MsgType ackedMsg = (MsgType)pkt.payload[0];
//             uint8_t status = pkt.payload[1];
            
//             // Serial.println("Interpretación:");
//             Serial.println(" - Tipo de Mensaje: MSG_ACK (0x00)");
            
//             Serial.print(" - Referencia a MsgType: 0x");
//             if (ackedMsg < 16) Serial.print("0");
//             Serial.println(ackedMsg, HEX);
            
//             Serial.print(" - Status: 0x");
//             if (status < 16) Serial.print("0");
//             Serial.print(status, HEX);
//             Serial.println(status == 0 ? " (ÉXITO)" : " (ERROR)");

//             if (status == 0) {
//                 Serial.println("-> Acción: Transmisión confirmada. No se requiere reenvío.");
//                 pendingImuRetry = false; // Limpiar banderas por si acaso
//             } else {
//                 Serial.println("-> Acción: [NACK] ERROR reportado. Evaluando reenvío asíncrono...");
                
//                 // Filtramos solo los mensajes críticos que ameritan reenvío según tu app
//                 if (ackedMsg == MSG_IMU_BUFFER || ackedMsg == MSG_PRESSURE_BUFFER || 
//                     ackedMsg == MSG_TEMPERATURE || ackedMsg == MSG_FALL_SENSORS || 
//                     ackedMsg == MSG_PANIC_BTN_PRESS) {
                    
//                     if (ackedMsg == MSG_IMU_BUFFER) {
//                         Serial.println("-> Programando reintento para MSG_IMU_BUFFER.");
//                         pendingImuRetry = true;
//                     } else {
//                         Serial.println("-> (A implementar: reintento para este MsgType específico)");
//                     }
//                 } else {
//                     Serial.println("-> Acción: El mensaje fallido no requiere reenvío automático.");
//                 }
//             }
//         } else {
//             Serial.println("Evento: Comando recibido no es un ACK o tiene formato incorrecto.");
//         }
//         Serial.println("-----------------------------------");
//     }
// }

// void setup() {
//     Serial.begin(115200);
//     // while(!Serial);

//     Serial.println("\n=== Iniciando Entorno de Prueba: BleDriver ===");

//     // Inicializar el sistema base de Nicla (necesario para BLE y sensores)
//     nicla::begin();
//     nicla::leds.begin();
//     nicla::leds.setColor(blue); // Azul indica inicializando/desconectado

//     // Obtener instancia e inicializar driver
//     bleComm = BleDriver::getInstance();
//     bleComm->init();
    
//     // Vincular la interrupción de recepción
//     // bleComm->attachRxNotifier(onRxReceived); // Attachea la funcion a ejecutar desde la ISR de Rx por BLE a nivel RTOS
//     // bleComm->attachRxInterrupt(); // Attachea la ISR al evento de Rx por BLE a nivel FW

//     // Rellenar buffer gigante con datos falsos (un patrón repetitivo)
//     // for (int i=0; i<sizeof(testBuffer); i++) {
//     //     testBuffer[i] = (uint8_t)(i % 256);
//     // }

//     Serial.println("BLE Configurado correctamente.");
//     Serial.println("Esperando conexión desde el smartphone...");
//     App::init();

// }

// void loop() {
//     BLE.poll();
//     delay(10);
// }