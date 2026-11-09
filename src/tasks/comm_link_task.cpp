#include "tasks/comm_link_task.h"
#include "mbed.h"
#include "drivers/serial_comm_driver.h"
#include "repositories/temperature_repository.h"
#include "repositories/event_repository.h"
#include "drivers/ble_driver.h"
#include "drivers/led_driver.h"
#include "tasks/system_task.h"
#include "utils/debug.h"

void CommLinkTask::init() {
#ifdef USE_SERIAL_COMM
    _comm = SerialCommDriver::getInstance();
#else
    _comm = BleDriver::getInstance();
#endif
    osStatus stat = _thread.start(mbed::callback(this, &CommLinkTask::run));
    if (stat != osOK) {
        DEBUG_PRINT("[ERROR] CommLinkTask: Fallo al iniciar hilo (codigo ");
        DEBUG_PRINT((int)stat);
        DEBUG_PRINTLN(" - Memoria insuficiente)");
    }
}

bool CommLinkTask::sendMsg(AppMessage* msg) {
    return _comm_link_task_queue.send(msg);
}

void CommLinkTask::run() {
    AppMessage msg;
    // Driver de comunicacion principal
#ifdef USE_SERIAL_COMM
    CommDriver* comm = SerialCommDriver::getInstance();
#else
    CommDriver* comm = BleDriver::getInstance();
#endif
    _comm = comm;

    while (true) {
        // Pacing dinámico: 20 ms si estamos despachando fragmentos asíncronos, 100 ms en reposo
        uint32_t queue_timeout_ms = comm->isTxInProgress() ? 20 : 100;

        if (_comm_link_task_queue.receive(&msg, queue_timeout_ms)) {
            // 1. Recepción (RX)
            if (msg.event_id == EVT_RX_PACKET) {
                handleRxPacket(comm);
            }
            // 2. Transmisión (TX)
            else if (comm->isAuthenticated()) {
                handleTxCommand(msg, comm);
            }
        }

        // Atiende eventos GATT, MTU, descubrimiento y conexion continuamente
        comm->processEvents();   

        // Monitoreo de conexion y watchdog de autenticacion
        // TODO Mover a la clase de comm:
    #ifndef USE_SERIAL_COMM
        bool currently_connected = comm->isConnected();
        if (currently_connected && !_was_connected) {
            _was_connected = true;
            _auth_attempts = 0;
            comm->setAuthenticated(false);
            _auth_timer_start_ms = millis();
            DEBUG_PRINTLN("[CommLinkTask] Central conectado. Esperando autenticacion UID...");
        } else if (!currently_connected && _was_connected) {
            _was_connected = false;
            _auth_attempts = 0;
            _auth_timer_start_ms = 0;
            comm->setAuthenticated(false);
            DEBUG_PRINTLN("[CommLinkTask] Central desconectado. Sesion reseteada.");
        }

        if (currently_connected && !comm->isAuthenticated()) {
            if (millis() - _auth_timer_start_ms >= BLE_AUTH_TIMEOUT_MS) {
                DEBUG_PRINTLN("[CommLinkTask] Timeout de autenticacion expirado (10s). Desconectando...");
                comm->disconnect();
                comm->setAuthenticated(false);
                _auth_attempts = 0;
                LedDriver::getInstance()->setLedColor(RGBColors::blue);
            }
        }
    #endif
    }
}

// =========================================================================
// Procesamiento Modular de Transmisión (TX)
// =========================================================================

void CommLinkTask::handleTxCommand(const AppMessage& msg, CommDriver* comm) {
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

// =========================================================================
// Procesamiento Modular de Recepción (RX)
// =========================================================================

void CommLinkTask::handleRxPacket(CommDriver* comm) {
    if (!comm->processRxData(&_rx_packet)) {
        return;
    }

    DEBUG_PRINTLN("[CommLinkTask] Paquete RX Recibido");

    if (_rx_packet.type == MSG_AUTH) {
        handleAuthPacket(_rx_packet.payload, _rx_packet.length, comm);
        return;
    }

    if (!comm->isAuthenticated()) {
        DEBUG_PRINTLN("[CommLinkTask] ALERTA: Paquete descartado (Dispositivo no autenticado)");
        return;
    }

    if (_rx_packet.type == MSG_ACK && _rx_packet.length >= 2) {
        MsgType ackedMsg = static_cast<MsgType>(_rx_packet.payload[0]);
        uint8_t status   = _rx_packet.payload[1];
        handleAck(ackedMsg, status);
    }
}

void CommLinkTask::handleAuthPacket(const uint8_t* payload, uint16_t length, CommDriver* comm) {
    const char* expected_uid = BRACELET_UID;
    size_t expected_len = strlen(expected_uid);

    DEBUG_PRINT("[CommLinkTask] MSG_AUTH recibido. Longitud: ");
    DEBUG_PRINTLN(length);

    bool match = (length == expected_len) && (memcmp(payload, expected_uid, expected_len) == 0);

    if (match) {
        DEBUG_PRINTLN("[CommLinkTask] Autenticacion EXITOSA. UID correcto.");
        comm->setAuthenticated(true);
        _auth_attempts = 0;
        _auth_timer_start_ms = 0;
        LedDriver::getInstance()->setLedColor(RGBColors::green);

        uint8_t ack_payload[2] = { static_cast<uint8_t>(MSG_AUTH), 0x00 };
        comm->sendPayload(MSG_ACK, ack_payload, sizeof(ack_payload));
    } else {
        _auth_attempts++;
        DEBUG_PRINT("[CommLinkTask] Autenticacion FALLIDA (Intento ");
        DEBUG_PRINT(_auth_attempts);
        DEBUG_PRINT(" de ");
        DEBUG_PRINT(BLE_AUTH_MAX_ATTEMPTS);
        DEBUG_PRINTLN(")");

        uint8_t ack_payload[2] = { static_cast<uint8_t>(MSG_AUTH), 0x01 };
        comm->sendPayload(MSG_ACK, ack_payload, sizeof(ack_payload));

        if (_auth_attempts >= BLE_AUTH_MAX_ATTEMPTS) {
            DEBUG_PRINTLN("[CommLinkTask] Maximo de intentos fallidos alcanzado. Desconectando central...");
            comm->disconnect();
            comm->setAuthenticated(false);
            _auth_attempts = 0;
            LedDriver::getInstance()->setLedColor(RGBColors::blue);
        }
    }
}

void CommLinkTask::handleAck(MsgType ackedMsg, uint8_t status) {
    DEBUG_PRINT("[CommLinkTask] MSG_ACK para MsgType: 0x");
    if (ackedMsg < 16) DEBUG_PRINT("0");
    DEBUG_PRINT(ackedMsg, HEX);
    DEBUG_PRINT(" | Status: ");
    DEBUG_PRINTLN(status == 0 ? "0x00 (EXITO)" : "0x01 (ERROR/NACK)");

    if (status == 0) {
        // Transmisión confirmada con éxito -> Resetear contador de reintentos
        if (ackedMsg < sizeof(_retry_counts)) {
            _retry_counts[ackedMsg] = 0;
        }
        return;
    }

    // NACK: evaluar reenvío si no superó el límite máximo
    const uint8_t MAX_TX_RETRIES = 2;
    if (ackedMsg < sizeof(_retry_counts) && _retry_counts[ackedMsg] < MAX_TX_RETRIES) {
        _retry_counts[ackedMsg]++;
        DEBUG_PRINT("[CommLinkTask] Reintentando envio de MsgType: 0x");
        DEBUG_PRINT(ackedMsg, HEX);
        DEBUG_PRINT(" (Intento ");
        DEBUG_PRINT(_retry_counts[ackedMsg]);
        DEBUG_PRINTLN(")");

        dispatchRetry(ackedMsg);
    } else {
        DEBUG_PRINT("[CommLinkTask] Limite de reintentos alcanzado para MsgType: 0x");
        DEBUG_PRINTLN(ackedMsg, HEX);
        if (ackedMsg < sizeof(_retry_counts)) {
            _retry_counts[ackedMsg] = 0;
        }
    }
}

void CommLinkTask::dispatchRetry(MsgType type) {
    switch (type) {
        case MSG_IMU_BUFFER:       notifyTask<CommLinkTask>(CMD_TX_IMU_BUFFER);       break;
        case MSG_PRESSURE_BUFFER:  notifyTask<CommLinkTask>(CMD_TX_PRESSURE_BUFFER);  break;
        case MSG_TEMPERATURE:      notifyTask<CommLinkTask>(CMD_TX_TEMPERATURE);      break;
        case MSG_FALL_SENSORS:     notifyTask<CommLinkTask>(CMD_TX_FALL_SENSORS);     break;
        case MSG_PANIC_BTN_PRESS:  notifyTask<CommLinkTask>(CMD_TX_PANIC_BTN_PRESS);  break;
        case MSG_NOTIF_BTN_PRESS:  notifyTask<CommLinkTask>(CMD_TX_NOTIF_BTN_PRESS);  break;
        case MSG_ALARM:            notifyTask<CommLinkTask>(CMD_TX_ALARM);            break;
        case MSG_WARNING:          notifyTask<CommLinkTask>(CMD_TX_WARNING);          break;
        case MSG_METRICS:          notifyTask<CommLinkTask>(CMD_TX_METRICS);          break;
        case MSG_BATTERY_DATA:     notifyTask<CommLinkTask>(CMD_TX_BATTERY_DATA);     break;
        default: break;
    }
}
