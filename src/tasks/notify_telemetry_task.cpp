#include "tasks/notify_telemetry_task.h"
#include "tasks/comm_link_task.h"
#include "drivers/battery_driver.h"
#include "repositories/imu_repository.h"
#include "drivers/led_driver.h"
#include "drivers/vibrator_driver.h"
#include "utils/debug.h"

#define VIB_TIME_2S_MS 2000
#define VIB_TIME_4S_MS 4000
#define VIB_TIME_7S_MS 7000

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
                case CMD_VIBRATOR_INTERMITTENT: {
                    /* ADVERTENCIA: Las rutinas de vibracion son bloqueantes (usan sleep_for).
                     * Si se envian multiples solicitudes seguidas, bloquearan otras rutinas. */
                    uint32_t duration_ms = decodeVibratorDuration(msg.flags);
                    playVibratorIntermittent(duration_ms);
                    break;
                }
                case CMD_VIBRATOR_CONTINUOUS: {
                    /* ADVERTENCIA: Las rutinas de vibracion son bloqueantes (usan sleep_for).
                     * Si se envian multiples solicitudes seguidas, bloquearan otras rutinas. */
                    uint32_t duration_ms = decodeVibratorDuration(msg.flags);
                    playVibratorContinuous(duration_ms);
                    break;
                }
                case CMD_VIBRATOR_CRESCENDO: {
                    /* ADVERTENCIA: Las rutinas de vibracion son bloqueantes (usan sleep_for).
                     * Si se envian multiples solicitudes seguidas, bloquearan otras rutinas. */
                    uint32_t duration_ms = decodeVibratorDuration(msg.flags);
                    playVibratorCrescendo(duration_ms);
                    break;
                }
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
                    
                    // 2. Enviamos explcitamente la variable estǭtica de la clase
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

uint32_t NotifyTelemetryTask::decodeVibratorDuration(uint32_t flags) {
    if (flags & VIB_DURATION_4S) return VIB_TIME_4S_MS;
    if (flags & VIB_DURATION_7S) return VIB_TIME_7S_MS;
    return VIB_TIME_2S_MS;
}

void NotifyTelemetryTask::playVibratorIntermittent(uint32_t duration_ms) {
    VibratorDriver* vibrador = VibratorDriver::getInstance();
    vibrador->enable();
    
    uint32_t elapsed = 0;
    bool on = true;
    while (elapsed < duration_ms) {
        vibrador->setStrength(on ? 70 : 0);
        rtos::ThisThread::sleep_for(std::chrono::milliseconds(250));
        elapsed += 250;
        on = !on;
    }
    
    vibrador->setStrength(0);
    vibrador->disable();
}

void NotifyTelemetryTask::playVibratorContinuous(uint32_t duration_ms) {
    VibratorDriver* vibrador = VibratorDriver::getInstance();
    vibrador->enable();
    
    vibrador->setStrength(70);
    rtos::ThisThread::sleep_for(std::chrono::milliseconds(duration_ms));
    
    vibrador->setStrength(0);
    vibrador->disable();
}

void NotifyTelemetryTask::playVibratorCrescendo(uint32_t duration_ms) {
    VibratorDriver* vibrador = VibratorDriver::getInstance();
    vibrador->enable();
    
    uint32_t steps = duration_ms / 100;
    for (uint32_t i = 1; i <= steps; i++) {
        uint8_t strength = (uint8_t)((i * 100) / steps);
        if (strength > 100) strength = 100;
        vibrador->setStrength(strength);
        rtos::ThisThread::sleep_for(std::chrono::milliseconds(100));
    }
    
    vibrador->setStrength(0);
    vibrador->disable();
}
