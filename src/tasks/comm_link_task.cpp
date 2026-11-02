#include "tasks/comm_link_task.h"
#include "mbed.h"
#include "drivers/serial_comm_driver.h"
#include "repositories/temperature_repository.h"
#include "repositories/event_repository.h"
#include "drivers/ble_driver.h"
#include "tasks/system_task.h"
#include "utils/debug.h"

void CommLinkTask::init() {
    osStatus stat = _thread.start(mbed::callback(this, &CommLinkTask::run));
    if (stat != osOK) {
        Serial.print("[ERROR] CommLinkTask: Fallo al iniciar hilo (codigo ");
        Serial.print((int)stat);
        Serial.println(" - Memoria insuficiente)");
    }
}

bool CommLinkTask::sendMsg(AppMessage* msg) {
    return _comm_link_task_queue.send(msg);
}

void CommLinkTask::run() {
    AppMessage msg;
    // Driver de comunicacion principal: BLE para transmitir hacia la app movil
    BleDriver* comm = BleDriver::getInstance();
    // SerialCommDriver* commSerial = SerialCommDriver::getInstance(); // Reservado para pruebas sin BLE

    while (true) {
        if (_comm_link_task_queue.receive(&msg)) {
            uint32_t t_start_ms = millis();
            
            switch (msg.event_id) {
                
                case CMD_TX_IMU_BUFFER: {
                    ImuRepository* imu = ImuRepository::getInstance();
                    imu->lock();
                    uint16_t count = 0;
                    const DataXYZ* buf = imu->getLinearBufferUnprotected(&count);
                    if (buf != nullptr && count > 0) {
                        comm->sendPayload(MSG_IMU_BUFFER, (const uint8_t*)buf, count * sizeof(DataXYZ));
                        imu->fifoFlushUnprotected();
                    }
                    imu->unlock();
                    break;
                }
                case CMD_TX_PRESSURE_BUFFER: {
                    PressureRepository* pres = PressureRepository::getInstance();
                    pres->lock();
                    uint16_t count = 0;
                    const float* buf = pres->getLinearBufferUnprotected(&count);
                    if (buf != nullptr && count > 0) {
                        comm->sendPayload(MSG_PRESSURE_BUFFER, (const uint8_t*)buf, count * sizeof(float));
                        pres->fifoFlushUnprotected();
                    }
                    pres->unlock();
                    break;
                }
                case CMD_TX_FALL_SENSORS: {
                    ImuRepository* imu = ImuRepository::getInstance();
                    imu->lock();
                    uint16_t imuCount = 0;
                    const DataXYZ* imuBuf = imu->getLinearBufferUnprotected(&imuCount);
                    if (imuBuf != nullptr && imuCount > 0) {
                        comm->sendPayload(MSG_IMU_BUFFER, (const uint8_t*)imuBuf, imuCount * sizeof(DataXYZ));
                        imu->fifoFlushUnprotected();
                    }
                    imu->unlock();

                    PressureRepository* pres = PressureRepository::getInstance();
                    pres->lock();
                    uint16_t presCount = 0;
                    const float* presBuf = pres->getLinearBufferUnprotected(&presCount);
                    if (presBuf != nullptr && presCount > 0) {
                        comm->sendPayload(MSG_PRESSURE_BUFFER, (const uint8_t*)presBuf, presCount * sizeof(float));
                        pres->fifoFlushUnprotected();
                    }
                    pres->unlock();

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

            // Validar si el envío demoró más que la ventana de impacto (en segundos)
            float elapsed_sec = (millis() - t_start_ms) / 1000.0f;
            float threshold_sec = (float)IMPACT_WINDOW_SIZE / (float)FREQ_IMU;
            if (elapsed_sec > threshold_sec) {
                DEBUG_PRINT("[CommLinkTask] ALERTA: Envio demorado! Tiempo (s): "); DEBUG_PRINT(elapsed_sec);
                DEBUG_PRINT(" > Umbral (s): "); DEBUG_PRINTLN(threshold_sec);
                notifyTask<SystemTask>(SystemTask::EVT_TX_DELAYED);
            }
        }
    }
}
