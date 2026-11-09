#include "app/app.h"
#include "app/config.h"
#include "utils/debug.h"

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

#ifdef USE_SERIAL_COMM
void isr_serial_rx() {
    // Mockup: Recepcion serial deshabilitada temporalmente para evitar eventos espurios
}
#else
void isr_serial_rx() {

}
#endif

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

#ifndef USE_SERIAL_COMM
void isr_ble_rx() {
    AppMessage msg;
    msg.event_id = CommLinkTask::EVT_RX_PACKET;
    msg.emisor_id = TASK_ISR_ROUTINE;
    CommLinkTask::getInstance().sendMsg(&msg);
}
#endif

// ============================================================================
// INICIALIZACION GLOBAL DEL SISTEMA
// ============================================================================
void App::init() {
    // ------------------------------------------------------------------------
    // Instanciar e Inicializar Hardware y Drivers Base
    // ------------------------------------------------------------------------
    
    DEBUG_PRINTLN("[App Init] Inicializando drivers");

#ifdef USE_SERIAL_COMM
    SerialCommDriver* comm = SerialCommDriver::getInstance();
    comm->init();
    // En modo serial unidireccional no se conecta la ISR de RX para evitar atender eventos espurios
#else
    BleDriver* bleComm = BleDriver::getInstance();
    bleComm->init();
    bleComm->attachRxNotifier(isr_ble_rx); // Attachea la funcion a ejecutar desde la ISR de Rx por BLE a nivel RTOS
#endif
    // Vibrador 
    VibratorDriver* vibrator = VibratorDriver::getInstance();
    vibrator->init();
    
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

    DEBUG_PRINTLN("[App Init] Seteando IRQ Handlers");
    PanicButton* panicBtn = PanicButton::getInstance();
    panicBtn->setIrqHandler(isr_panic_button);

    NotifButton* notifBtn = NotifButton::getInstance();
    notifBtn->setIrqHandler(isr_notif_button);

    // ------------------------------------------------------------------------
    // Inicializar Repositorios de Sensores
    // ------------------------------------------------------------------------
    
    bhi260->disableNonWakeupFIFO();
    
    DEBUG_PRINTLN("[App Init] Inicializando repositorios");

    ImuRepository::getInstance()->begin((float)FREQ_IMU, LATENCIA_IMU);
    PressureRepository::getInstance()->begin((float)FREQ_PRESSURE, (uint32_t)-1);
    TemperatureRepository::getInstance()->begin(1.0f, (uint32_t)-1);

    MotionRepository::getInstance()->begin(1.0f, 0);
    NoMotionRepository::getInstance()->begin(1.0f, 0);
    StepCounterRepository::getInstance()->begin(1.0f, (uint32_t)-1);
    
    // ------------------------------------------------------------------------
    // Levantar las Tareas (Threads) del RTOS
    // ------------------------------------------------------------------------
    
    DEBUG_PRINTLN("[App Init] Inicializando Tasks");

    CommLinkTask::getInstance().init();
    NotifyTelemetryTask::getInstance().init();
    PmicTask::getInstance().init();
    AlarmsEventsTask::getInstance().init();
    SystemTask::getInstance().init();

    // ------------------------------------------------------------------------
    // Habilitar Interrupciones (Post-RTOS)
    // ------------------------------------------------------------------------
    // Se habilitan al final para evitar que una ISR intente enviar un mensaje
    // a una tarea que todavia no fue inicializada.

    DEBUG_PRINTLN("[App Init] Habilitacion de IRQ");

    bhi260->enableInterrupt();
    bhi260->flushFIFOs();

    panicBtn->enableInterrupt();
    notifBtn->enableInterrupt();
    bleComm->attachRxInterrupt(); // Attachea la ISR al evento de Rx por BLE a nivel FW
    // serialDriver->attachRxInterrupt();

    DEBUG_PRINTLN("[App Init] Inicializacion del sistema completada");
}
