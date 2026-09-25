#include "app/app.h"


// Tareas
#include "tasks/system_task.h"
#include "tasks/comm_link_task.h"
#include "tasks/notif_ui_task.h"
#include "tasks/alarms_events_task.h"

// Drivers
#include "drivers/battery_driver.h"
#include "drivers/bhi260_driver.h"
#include "drivers/interface_driver.h"
#include "drivers/serial_comm_driver.h"
#include "drivers/ble_driver.h"
#include "drivers/vibrator_driver.h"

// ============================================================================
// RUTINAS DE SERVICIO DE INTERRUPCION (ISRs)
// ============================================================================

void isr_bhi260() {

}

void isr_serial_rx() {

}



// ============================================================================
// INICIALIZACION GLOBAL DEL SISTEMA
// ============================================================================
void App::init() {
    // ------------------------------------------------------------------------
    // 1. Instanciar e Inicializar Hardware y Drivers Base
    // ------------------------------------------------------------------------
    
    // PMIC (Batería)
    BatteryDriver* battery = BatteryDriver::createInstance();
    // battery->init(); // Si el createInstance no lo inicializa por dentro
    
    // Sensor Principal BHI260
    BHI260Driver* bhi260 = BHI260Driver::getInstance();
    bhi260->init();
    bhi260->configureInterrupt(isr_bhi260);

    // Interfaz de Usuario (Botones y LEDs)
    InterfaceDriver* interface = InterfaceDriver::createInstance();

    // Vibrador (Pendiente de mergear rama)
    // VibratorDriver* vibrator = VibratorDriver::createInstance();
    // vibrator->init();

    // Comunicaciones
    SerialCommDriver* serialDriver = SerialCommDriver::getInstance();
    
    
    // ------------------------------------------------------------------------
    // 2. Levantar las Tareas (Threads) del RTOS
    // ------------------------------------------------------------------------
    // Se configuran e inician los hilos.
    
    SystemTask::getInstance().init();
    CommLinkTask::getInstance().init();
    NotifUiTask::getInstance().init();
    AlarmsEventsTask::getInstance().init();


    // ------------------------------------------------------------------------
    // 3. Habilitar Interrupciones (Post-RTOS)
    // ------------------------------------------------------------------------
    // Se habilitan al final para evitar que una ISR intente enviar un mensaje
    // a una tarea que todavia no fue inicializada.

    bhi260->enableInterrupt();
    
    // interface->enableInterrupts(); // Descomentar/Implementar si es necesario
    // serialDriver.enableInterrupt(); 
}
