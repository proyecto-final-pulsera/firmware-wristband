#include "app/app.h"


// Tareas
#include "tasks/system_task.h"
#include "tasks/comm_link_task.h"
#include "tasks/notify_telemetry_task.h"
#include "tasks/pmic_task.h"
#include "tasks/alarms_events_task.h"

// Drivers
#include "drivers/battery_driver.h"
#include "drivers/bhi260_driver.h"
#include "drivers/led_driver.h"
#include "drivers/button_driver.h"
#include "drivers/serial_comm_driver.h"
#include "drivers/ble_driver.h"
#include "drivers/vibrator_driver.h"

// Repositories
#include "repositories/imu_repository.h"
#include "repositories/pressure_repository.h"
#include "repositories/temperature_repository.h"
#include "repositories/event_repository.h"

// ============================================================================
// RUTINAS DE SERVICIO DE INTERRUPCION (ISRs)
// ============================================================================


void isr_bhi260() {
    AppMessage msg;
    msg.event_id = SystemTask::EVT_BHI_INTERRUPT;
    msg.emisor_id = TASK_ISR_ROUTINE;
    SystemTask::getInstance().sendMsg(&msg);
}

void isr_serial_rx() {

}

void isr_panic_button() {

    if (PanicButton::getInstance()->onInterrupt()) {
        AppMessage msg;
        msg.event_id = SystemTask::CMD_EVALUATE_PANIC_BUTTON;
        msg.emisor_id = TASK_ISR_ROUTINE; 
        SystemTask::getInstance().sendMsg(&msg);
    }
}

void isr_notif_button() {
    
    if (NotifButton::getInstance()->onInterrupt()) {
        AppMessage msg;
        msg.event_id = SystemTask::CMD_EVALUATE_NOTIF_BUTTON;
        msg.emisor_id = TASK_ISR_ROUTINE; 
        SystemTask::getInstance().sendMsg(&msg);
    }
}

// void onRxReceived(){
//     return;
// }

// void onRxReceived() {
//     CommPacket pkt;
//     BleDriver* bleComm = BleDriver::getInstance();
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

// ============================================================================
// INICIALIZACION GLOBAL DEL SISTEMA
// ============================================================================
void App::init() {
    // ------------------------------------------------------------------------
    // 1. Instanciar e Inicializar Hardware y Drivers Base
    // ------------------------------------------------------------------------
    
    // PMIC (Bateri­a)
    BatteryDriver* battery = BatteryDriver::getInstance();
    battery->init();
    
    // Sensor Principal BHI260
    BHI260Driver* bhi260 = BHI260Driver::getInstance();
    bhi260->init();
    bhi260->configureInterrupt(isr_bhi260);

    // Interfaz LED
    LedDriver* led = LedDriver::getInstance();
    led->init();

    PanicButton* panicBtn = PanicButton::getInstance();
    panicBtn->setIrqHandler(isr_panic_button);

    NotifButton* notifBtn = NotifButton::getInstance();
    notifBtn->setIrqHandler(isr_notif_button);

    // Vibrador 
    VibratorDriver* vibrator = VibratorDriver::getInstance();
    vibrator->init();

    // Comunicaciones
    // SerialCommDriver* serialDriver = SerialCommDriver::getInstance();
    // serialDriver->init();
    // BleDriver* bleComm = BleDriver::getInstance();
    // bleComm->init();

    // bleComm->attachRxNotifier(onRxReceived); // Attachea la funcion a ejecutar desde la ISR de Rx por BLE a nivel RTOS
    // bleComm->attachRxInterrupt(); // Attachea la ISR al evento de Rx por BLE a nivel FW


    // ------------------------------------------------------------------------
    // 1.5 Inicializar Repositorios de Sensores
    // ------------------------------------------------------------------------
    bhi260->disableNonWakeupFIFO();
    
    ImuRepository::getInstance()->begin((float)FREQ_IMU, 3000); // Latencia de 3000ms
    PressureRepository::getInstance()->begin((float)FREQ_PRESSURE, (uint32_t)-1);
    TemperatureRepository::getInstance()->begin(1.0f, (uint32_t)-1);

    MotionRepository::getInstance()->begin(1.0f, 0);
    NoMotionRepository::getInstance()->begin(1.0f, 0);
    StepCounterRepository::getInstance()->begin(1.0f, (uint32_t)-1);
    
    
    // ------------------------------------------------------------------------
    // 2. Levantar las Tareas (Threads) del RTOS
    // ------------------------------------------------------------------------
    // Se configuran e inician los hilos.
    
    
    CommLinkTask::getInstance().init();
    NotifyTelemetryTask::getInstance().init();
    PmicTask::getInstance().init();
    AlarmsEventsTask::getInstance().init();
    SystemTask::getInstance().init();

    // ------------------------------------------------------------------------
    // 3. Habilitar Interrupciones (Post-RTOS)
    // ------------------------------------------------------------------------
    // Se habilitan al final para evitar que una ISR intente enviar un mensaje
    // a una tarea que todavia no fue inicializada.

    bhi260->enableInterrupt();
    bhi260->flushFIFOs();

    panicBtn->enableInterrupt();
    notifBtn->enableInterrupt();
    // serialDriver.enableInterrupt(); 
}
