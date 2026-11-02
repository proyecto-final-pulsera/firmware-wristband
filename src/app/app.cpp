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

void onRxReceived(){
    return;
}

// ============================================================================
// INICIALIZACION GLOBAL DEL SISTEMA
// ============================================================================
void App::init() {
    // ------------------------------------------------------------------------
    // 1. Instanciar e Inicializar Hardware y Drivers Base
    // ------------------------------------------------------------------------
    
    // Comunicaciones BLE: Inicializar PRIMERO para que Cordio aloje sus 13 KB
    // en un Heap 100% contiguo y libre de fragmentación.
    BleDriver* bleComm = BleDriver::getInstance();
    bleComm->init();

    Serial.println("BLE Configurado correctamente.");
    Serial.println("Esperando conexión desde el smartphone...");

    // Vibrador 
    VibratorDriver* vibrator = VibratorDriver::getInstance();
    vibrator->init();
    Serial.println("INIT Vibrator");

    // PMIC (Batería)
    BatteryDriver* battery = BatteryDriver::getInstance();
    battery->init();
    Serial.println("INIT Battery");    

    // Sensor Principal BHI260
    BHI260Driver* bhi260 = BHI260Driver::getInstance();
    bhi260->init();
    bhi260->configureInterrupt(isr_bhi260);
    Serial.println("INIT BHI");

    // Interfaz LED
    LedDriver* led = LedDriver::getInstance();
    led->init();
    Serial.println("INIT Led");

    PanicButton* panicBtn = PanicButton::getInstance();
    panicBtn->setIrqHandler(isr_panic_button);

    NotifButton* notifBtn = NotifButton::getInstance();
    notifBtn->setIrqHandler(isr_notif_button);
    Serial.println("INIT Buttons");

    // bleComm->attachRxNotifier(onRxReceived); // Attachea la funcion a ejecutar desde la ISR de Rx por BLE a nivel RTOS
    // bleComm->attachRxInterrupt(); // Attachea la ISR al evento de Rx por BLE a nivel FW

    Serial.println("INIT FLAG 01: Drivers OK.");

    // ------------------------------------------------------------------------
    // 1.5 Inicializar Repositorios de Sensores
    // ------------------------------------------------------------------------
    bhi260->disableNonWakeupFIFO();
    
    ImuRepository::getInstance()->begin((float)FREQ_IMU, LATENCIA_IMU);
    PressureRepository::getInstance()->begin((float)FREQ_PRESSURE, (uint32_t)-1);
    TemperatureRepository::getInstance()->begin(1.0f, (uint32_t)-1);

    MotionRepository::getInstance()->begin(1.0f, 0);
    NoMotionRepository::getInstance()->begin(1.0f, 0);
    StepCounterRepository::getInstance()->begin(1.0f, (uint32_t)-1);
    
    Serial.println("INIT FLAG 01.5: Repositories OK.");      

    // ------------------------------------------------------------------------
    // 2. Levantar las Tareas (Threads) del RTOS
    // ------------------------------------------------------------------------
    // Se configuran e inician los hilos.
    
    CommLinkTask::getInstance().init();
    NotifyTelemetryTask::getInstance().init();
    PmicTask::getInstance().init();
    AlarmsEventsTask::getInstance().init();
    SystemTask::getInstance().init();
    Serial.println("INIT FLAG 02: All Tasks started.");

    // ------------------------------------------------------------------------
    // 3. Habilitar Interrupciones (Post-RTOS)
    // ------------------------------------------------------------------------
    // Se habilitan al final para evitar que una ISR intente enviar un mensaje
    // a una tarea que todavia no fue inicializada.

    bhi260->enableInterrupt();
    bhi260->flushFIFOs();

    panicBtn->enableInterrupt();
    notifBtn->enableInterrupt();
    // serialDriver->attachRxInterrupt();
    Serial.println("INIT FLAG 03: Interrupts Enabled."); 
}
