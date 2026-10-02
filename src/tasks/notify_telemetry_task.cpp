#include "tasks/notify_telemetry_task.h"
#include "tasks/comm_link_task.h"
#include "drivers/battery_driver.h"
#include "repositories/imu_repository.h"
#include "drivers/led_driver.h"
#include "drivers/vibrator_driver.h"
#include "utils/debug.h"

BatteryData NotifyTelemetryTask::_shared_battery_data = {0, false};
MetricsData NotifyTelemetryTask::_shared_metrics_data = {0};

void NotifyTelemetryTask::init() {
    _thread.start(mbed::callback(this, &NotifyTelemetryTask::run));
}

bool NotifyTelemetryTask::sendMsg(AppMessage* msg) {
    return _mailbox.send(msg);
}

void NotifyTelemetryTask::updateMetrics() {
    // Actualizar step_count desde el repositorio
    _shared_metrics_data.step_count = StepCounterRepository::getInstance()->getEventCount();
    StepCounterRepository::getInstance()->clearEventCount();
}

void NotifyTelemetryTask::run() {
    AppMessage msg;
    
    while (true) {
        // Esperamos un mensaje bloqueando indefinidamente (Cero timeouts dinámicos)
        if (_mailbox.receive(&msg, osWaitForever)) {
            switch (msg.event_id) {
                case CMD_VIBRATOR_GESTURE:
                    // TODO: vibrator driver
                    break;
                case CMD_LED_NOTIFY: {
                    uint32_t led_flags = msg.flags;
                    LedDriver* led = LedDriver::getInstance();
                    
                    if (led_flags == LED_COLOR_OFF) {
                        led->setLedState(false);
                    } else if (led_flags == LED_PROFILE_WARN) {
                        led->setLedWarn();
                    } else if (led_flags == LED_PROFILE_ALARM) {
                        led->setLedAlarm();
                    } else if (led_flags == LED_PROFILE_NOTIF) {
                        led->setLedNotif();
                    } else {
                        // Procesar colores custom
                        if (led_flags & LED_COLOR_RED)   led->setLedColor(RGBColors::red);
                        if (led_flags & LED_COLOR_GREEN) led->setLedColor(RGBColors::green);
                        if (led_flags & LED_COLOR_BLUE)  led->setLedColor(RGBColors::blue);
                        
                        led->setLedState(true);
                    }
                    break;
                }
                case CMD_NOTIF_BATTERY_DATA: {
                    // Actualizamos valores reales
                    _shared_battery_data.charge_percent = BatteryDriver::getInstance()->getBatteryCharge();
                    _shared_battery_data.is_charging = (BatteryDriver::getInstance()->getOperatingStatus() == OperatingStatus::Charging);
                    
                    DEBUG_PRINT("[NOTIFY_TELEMETRY] Nivel de bateria ");
                    DEBUG_PRINTLN(_shared_battery_data.is_charging);
                    
                    // Notificamos a la cola usando el pointer en el envelope
                    notifyTask<CommLinkTask>(
                        CommLinkTask::CMD_TX_BATTERY_DATA,
                        TASK_NOTIFY_TELEMETRY,
                        PRIORITY_LOW,
                        &_shared_battery_data,
                        sizeof(BatteryData)
                    );
                    break;
                }
                case CMD_NOTIF_ACTIVITY_DATA: {
                    // 1. Actualizamos los datos internos
                    updateMetrics();
                    
                    // 2. Enviamos explícitamente la variable estática de la clase
                    notifyTask<CommLinkTask>(
                        CommLinkTask::CMD_TX_METRICS,
                        TASK_NOTIFY_TELEMETRY,
                        PRIORITY_LOW,
                        &_shared_metrics_data,
                        sizeof(MetricsData)
                    );
                    break;
                }
            }
        }
    }
}
