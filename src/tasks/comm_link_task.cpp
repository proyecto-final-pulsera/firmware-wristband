#include "tasks/comm_link_task.h"
#include "mbed.h"
#include "drivers/serial_comm_driver.h"
#include "repositories/temperature_repository.h"
#include "repositories/event_repository.h"

void CommLinkTask::init() {
    // Iniciar el hilo asociado al metodo run de esta instancia
    _thread.start(mbed::callback(this, &CommLinkTask::run));
}

bool CommLinkTask::sendMsg(AppMessage* msg) {
    return _comm_link_task_queue.send(msg);
}

void CommLinkTask::run() {
    AppMessage msg;
    SerialCommDriver* comm = SerialCommDriver::getInstance();

    while (true) {
        if (_comm_link_task_queue.receive(&msg)) {
            switch (msg.event_id) {
                case CMD_TX_IMU_BUFFER: {
                    uint16_t count = ImuRepository::getInstance()->getFifoValues(_imu_tx_buffer, IMU_FIFO_SIZE);
                    // ImuRepository::getInstance()->rewind(count);
                    comm->sendPayload(MSG_IMU_BUFFER, (uint8_t*)_imu_tx_buffer, count * sizeof(DataXYZ));
                    break;
                }
                case CMD_TX_PRESSURE_BUFFER: {
                    uint16_t count = PressureRepository::getInstance()->getFifoValues(_pressure_tx_buffer, PRESSURE_FIFO_SIZE);
                    // PressureRepository::getInstance()->rewind(count);
                    comm->sendPayload(MSG_PRESSURE_BUFFER, (uint8_t*)_pressure_tx_buffer, count * sizeof(float));
                    break;
                }
                case CMD_TX_FALL_SENSORS: {
                    // Copiamos ambos buffers a los locales (vaciando las FIFOs)
                    uint16_t imuCount = ImuRepository::getInstance()->getFifoValues(_imu_tx_buffer, IMU_FIFO_SIZE);
                    uint16_t presCount = PressureRepository::getInstance()->getFifoValues(_pressure_tx_buffer, PRESSURE_FIFO_SIZE);
                    float temp = TemperatureRepository::getInstance()->getTemp();
                    
                    // ImuRepository::getInstance()->rewind(imuCount);
                    // PressureRepository::getInstance()->rewind(presCount);

                    // Enviamos en secuencia
                    comm->sendPayload(MSG_IMU_BUFFER, (uint8_t*)_imu_tx_buffer, imuCount * sizeof(DataXYZ));
                    comm->sendPayload(MSG_PRESSURE_BUFFER, (uint8_t*)_pressure_tx_buffer, presCount * sizeof(float));
                    comm->sendPayload(MSG_TEMPERATURE, (uint8_t*)&temp, sizeof(float));
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
                    // Leemos pasos, reseteamos driver, guardamos en buffer local por seguridad
                    _metrics_buffer.step_count = StepCounterRepository::getInstance()->getEventCount();
                    StepCounterRepository::getInstance()->clearEventCount();
                    
                    comm->sendPayload(MSG_METRICS, (uint8_t*)&_metrics_buffer, sizeof(MetricsData));
                    break;
                }
                case CMD_TX_KEEP_ALIVE: {
                    comm->sendPayload(MSG_KEEP_ALIVE, nullptr, 0);
                    break;
                }
                default:
                    break;
            }
        }
    }
}
