#include "tasks/comm_link_task.h"
#include "mbed.h"
#include "drivers/serial_comm_driver.h"
#include "repositories/temperature_repository.h"
#include "repositories/event_repository.h"
#include "drivers/ble_driver.h"

void CommLinkTask::init() {
    // Iniciar el hilo asociado al metodo run de esta instancia
    _thread.start(mbed::callback(this, &CommLinkTask::run));
}

bool CommLinkTask::sendMsg(AppMessage* msg) {
    return _comm_link_task_queue.send(msg);
}

void CommLinkTask::run() {
    AppMessage msg;
    BleDriver* comm = BleDriver::getInstance();

    while (true) {
        if (_comm_link_task_queue.receive(&msg)) {
            switch (msg.event_id) {
                case CMD_TX_IMU_BUFFER: {
                    uint16_t count = 0;
                    const DataXYZ* buf = ImuRepository::getInstance()->getLinearBuffer(&count);
                    if (buf != nullptr && count > 0) {
                        comm->sendPayload(MSG_IMU_BUFFER, (const uint8_t*)buf, count * sizeof(DataXYZ));
                        ImuRepository::getInstance()->fifoFlush();
                    }
                    break;
                }
                case CMD_TX_PRESSURE_BUFFER: {
                    uint16_t count = 0;
                    const float* buf = PressureRepository::getInstance()->getLinearBuffer(&count);
                    if (buf != nullptr && count > 0) {
                        comm->sendPayload(MSG_PRESSURE_BUFFER, (const uint8_t*)buf, count * sizeof(float));
                        PressureRepository::getInstance()->fifoFlush();
                    }
                    break;
                }
                case CMD_TX_FALL_SENSORS: {
                    uint16_t imuCount = 0;
                    const DataXYZ* imuBuf = ImuRepository::getInstance()->getLinearBuffer(&imuCount);
                    if (imuBuf != nullptr && imuCount > 0) {
                        comm->sendPayload(MSG_IMU_BUFFER, (const uint8_t*)imuBuf, imuCount * sizeof(DataXYZ));
                        ImuRepository::getInstance()->fifoFlush();
                    }

                    uint16_t presCount = 0;
                    const float* presBuf = PressureRepository::getInstance()->getLinearBuffer(&presCount);
                    if (presBuf != nullptr && presCount > 0) {
                        comm->sendPayload(MSG_PRESSURE_BUFFER, (const uint8_t*)presBuf, presCount * sizeof(float));
                        PressureRepository::getInstance()->fifoFlush();
                    }

                    float temp = TemperatureRepository::getInstance()->getTemp();
                    comm->sendPayload(MSG_TEMPERATURE, (const uint8_t*)&temp, sizeof(float));
                    break;
                }
                case CMD_TX_TEMPERATURE: {
                    float temp = TemperatureRepository::getInstance()->getTemp();
                    comm->sendPayload(MSG_TEMPERATURE, (uint8_t*)&temp, sizeof(float));
                    break;
                }
                case CMD_TX_ALARM: {
                    uint8_t alarm_id = (uint8_t)msg.flags;
                    comm->sendPayload(MSG_ALARM, &alarm_id, sizeof(uint8_t));
                    break;
                }
                case CMD_TX_WARNING: {
                    uint8_t warning_id = (uint8_t)msg.flags;
                    comm->sendPayload(MSG_WARNING, &warning_id, sizeof(uint8_t));
                    break;
                }
                case CMD_TX_METRICS: {
                    // El payload ya viene en el mensaje (gracias a NotifyTelemetryTask)
                    // Para ser seguros, si viene un puntero valido lo enviamos directo
                    if (msg.payload_ptr != nullptr && msg.payload_len > 0) {
                        comm->sendPayload(MSG_METRICS, (uint8_t*)msg.payload_ptr, msg.payload_len);
                    }
                    break;
                }
                case CMD_TX_BATTERY_DATA: {
                    if (msg.payload_ptr != nullptr && msg.payload_len > 0) {
                        comm->sendPayload(MSG_BATTERY_DATA, (uint8_t*)msg.payload_ptr, msg.payload_len);
                    }
                    break;
                }
                case CMD_TX_KEEP_ALIVE: {
                    comm->sendPayload(MSG_KEEP_ALIVE, nullptr, 0);
                    break;
                }
                case CMD_TX_PANIC_BTN_PRESS: {
                    comm->sendPayload(MSG_PANIC_BTN_PRESS, nullptr, 0);
                    break;
                }
                case CMD_TX_NOTIF_BTN_PRESS: {
                    comm->sendPayload(MSG_NOTIF_BTN_PRESS, nullptr, 0);
                    break;
                }
                default:
                    break;
            }
        }
    }
}
