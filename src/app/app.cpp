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
    SerialCommDriver* serialDriver = SerialCommDriver::getInstance();
    serialDriver->init();
    
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
