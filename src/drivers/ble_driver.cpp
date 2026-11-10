#include "drivers/ble_driver.h"
#include <mbed.h>
#include "utils/debug.h"

// UUIDs Base (Custom Communication Service)
#define BLE_COMM_SERVICE_UUID "19B10000-E8F2-537E-4F6C-D104768A1214"
#define BLE_COMM_TX_CHAR_UUID "19B10001-E8F2-537E-4F6C-D104768A1214"
#define BLE_COMM_RX_CHAR_UUID "19B10002-E8F2-537E-4F6C-D104768A1214"

BleDriver* BleDriver::getInstance() {
    static BleDriver instance;
    return &instance;
}

void BleDriver::init() {
    if (!BLE.begin()) {
        return; // Fallo en chip radio
    }

    BLE.setLocalName("SafebandBLE");
    BLE.setDeviceName("SafebandBLE");

    // 1. Custom Communication Service
    _commService = new BLEService(BLE_COMM_SERVICE_UUID);
    
    // TX: Notify, el tamaño máximo de paquete enviado será BLE_MTU_PAYLOAD
    _txCharacteristic = new BLECharacteristic(BLE_COMM_TX_CHAR_UUID, BLENotify, BLE_MTU_PAYLOAD);
    
    // RX: Write, tamaño igual
    _rxCharacteristic = new BLECharacteristic(BLE_COMM_RX_CHAR_UUID, BLEWrite, BLE_MTU_PAYLOAD);

    _commService->addCharacteristic(*_txCharacteristic);
    _commService->addCharacteristic(*_rxCharacteristic);
    BLE.addService(*_commService);

    // 2. Battery Service (Estándar: 0x180F)
    _batteryService = new BLEService("180F");
    _batteryLevelChar = new BLEUnsignedCharCharacteristic("2A19", BLERead | BLENotify); // (Estándar: 0x2A19)
    _batteryService->addCharacteristic(*_batteryLevelChar);
    BLE.addService(*_batteryService);
    _batteryLevelChar->writeValue(100);

    // 3. Device Information Service (Estándar: 0x180A) - Comentado para maximizar holgura en Heap
    // _deviceInfoService = new BLEService("180A");
    // _manufacturerNameChar = new BLEStringCharacteristic("2A29", BLERead, 20);
    // _firmwareRevChar = new BLEStringCharacteristic("2A26", BLERead, 10);
    // _deviceInfoService->addCharacteristic(*_manufacturerNameChar);
    // _deviceInfoService->addCharacteristic(*_firmwareRevChar);
    // BLE.addService(*_deviceInfoService);
    // _manufacturerNameChar->writeValue("Proyecto_Final");
    // _firmwareRevChar->writeValue("1.0.0");

    // Publicitar solo el servicio custom para que la app sepa conectar
    BLE.setAdvertisedService(*_commService);

    BLE.setEventHandler(BLEConnected, onBleConnect);
    BLE.setEventHandler(BLEDisconnected, onBleDisconnect);

    BLE.advertise();

    // Configurar potencia de transmisión de la antena (por defecto al máximo: +4 dBm)
    setTxPower(TxPower::POS_4_DBM);
}

void BleDriver::attachRxInterrupt() {
    if (_rxCharacteristic != nullptr) {
        _rxCharacteristic->setEventHandler(BLEWritten, onBleWrite);
    }
}

bool BleDriver::processRxData(CommPacket* out_msg) {
    if (_rxIndex > 0) {
        // En una app real, acá se parsea el buffer crudo para poblar el CommPacket
        // Asumiendo formato de llegada: [MsgType] [Payload...]
        out_msg->type = (MsgType)_rxBuffer[0];
        out_msg->length = _rxIndex - 1;
        if (out_msg->length > 0) {
            memcpy(out_msg->payload, &_rxBuffer[1], out_msg->length);
        }
        _rxIndex = 0; // Consumido
        return true;
    }
    return false;
}

bool BleDriver::sendPayload(MsgType type, const uint8_t* payload, uint16_t len) {

    if (!BLE.connected())
        return false;

    if (len <= BLE_MTU_PAYLOAD - 1) {
        // Envío Directo (Sin fragmentar) -> [MsgType] [Payload...]
        // Los mensajes prioritarios simples pueden despacharse de inmediato sin interferir con la fragmentación
        _txPacketBuffer[0] = type;
        if (len > 0 && payload != nullptr) {
            memcpy(&_txPacketBuffer[1], payload, len);
        }
        bool success = (_txCharacteristic->writeValue(_txPacketBuffer, len + 1) != 0);
        BLE.poll();
        return success;
    } else {
        // Envío Fragmentado Bloqueante (Síncrono a nivel tarea con pacing RTOS)
        _isTxInProgress = true;

        uint16_t maxSizePerChunk = BLE_MTU_PAYLOAD - 6;
        uint16_t totalChunks = (len + maxSizePerChunk - 1) / maxSizePerChunk;
        uint8_t globalCrc = calculateCRC8(payload, len);

        for (uint16_t seq = 0; seq < totalChunks; seq++) {
            if (!BLE.connected() || !_txCharacteristic->subscribed()) {
                _isTxInProgress = false;
                return false;
            }

            uint16_t offset = seq * maxSizePerChunk;
            uint16_t remaining = len - offset;
            uint16_t chunkDataLen = (remaining < maxSizePerChunk) ? remaining : maxSizePerChunk;

            // Estructura Cabecera de Fragmento: [MsgType] [SeqHigh] [SeqLow] [TotalHigh] [TotalLow] [CRC8_Global] [Payload...]
            _txPacketBuffer[0] = type;
            _txPacketBuffer[1] = (seq >> 8) & 0xFF; // Parte alta
            _txPacketBuffer[2] = seq & 0xFF;        // Parte baja
            _txPacketBuffer[3] = (totalChunks >> 8) & 0xFF; // Parte alta
            _txPacketBuffer[4] = totalChunks & 0xFF;        // Parte baja
            _txPacketBuffer[5] = globalCrc;

            memcpy(&_txPacketBuffer[6], &payload[offset], chunkDataLen);

            if (!_txCharacteristic->writeValue(_txPacketBuffer, chunkDataLen + 6)) {
                _isTxInProgress = false;
                return false;
            }

            BLE.poll();

            // Pacing entre fragmentos: cede el tiempo de CPU a otras tareas del RTOS
            // y da tiempo al stack BLE / SoftDevice para transmitir el paquete
            rtos::ThisThread::sleep_for(std::chrono::milliseconds(20));
        }

        _isTxInProgress = false;
        return true;
    }
}

void BleDriver::processEvents() {
    BLE.poll();
}

void BleDriver::updateBatteryLevel(uint8_t level) {
    if (_batteryLevelChar != nullptr) {
        _batteryLevelChar->writeValue(level);
    }
}

// =========================================================================
// Callbacks Estáticos BLE
// =========================================================================

void BleDriver::onBleWrite(BLEDevice central, BLECharacteristic characteristic) {
    BleDriver* instance = getInstance();
    uint16_t len = characteristic.valueLength();
    
    // Evitar desbordamientos
    if (len > 0 && len <= sizeof(instance->_rxBuffer)) {
        characteristic.readValue(instance->_rxBuffer, len);
        instance->_rxIndex = len;
        
        // Notificamos a la capa RTOS superior (inyectada por el CommDriver)
        instance->notifyRxFromISR(); 
    }
}

void BleDriver::onBleConnect(BLEDevice central) {
    DEBUG_PRINT("[BLE_Driver] App conectada. MAC: ");
    DEBUG_PRINTLN(central.address());
    nicla::leds.setColor(yellow); // Amarillo indica conectado esperando autenticacion UID
}

void BleDriver::onBleDisconnect(BLEDevice central) {
    // Limpiar estado
    BleDriver* instance = getInstance();
    instance->_isTxInProgress = false;

    DEBUG_PRINT("[BLE_Driver] App desconectada. MAC: ");
    DEBUG_PRINTLN(central.address());
    nicla::leds.setColor(blue); // Vuelve a azul para indicar desconectado/esperando
}

// ==================================================================
// Potencia de Transmisión de Antena (TX Power)
// ==================================================================
void BleDriver::setTxPower(TxPower power) {
    setTxPower(static_cast<int8_t>(power));
}

void BleDriver::setTxPower(int8_t power_dbm) {
    uint32_t reg_val;
    switch (power_dbm) {
        case 4:
            reg_val = (RADIO_TXPOWER_TXPOWER_Pos4dBm << RADIO_TXPOWER_TXPOWER_Pos);
            break;
        case 0:
            reg_val = (RADIO_TXPOWER_TXPOWER_0dBm << RADIO_TXPOWER_TXPOWER_Pos);
            break;
        case -4:
            reg_val = (RADIO_TXPOWER_TXPOWER_Neg4dBm << RADIO_TXPOWER_TXPOWER_Pos);
            break;
        case -8:
            reg_val = (RADIO_TXPOWER_TXPOWER_Neg8dBm << RADIO_TXPOWER_TXPOWER_Pos);
            break;
        case -12:
            reg_val = (RADIO_TXPOWER_TXPOWER_Neg12dBm << RADIO_TXPOWER_TXPOWER_Pos);
            break;
        case -16:
            reg_val = (RADIO_TXPOWER_TXPOWER_Neg16dBm << RADIO_TXPOWER_TXPOWER_Pos);
            break;
        case -20:
            reg_val = (RADIO_TXPOWER_TXPOWER_Neg20dBm << RADIO_TXPOWER_TXPOWER_Pos);
            break;
        case -40:
            reg_val = (RADIO_TXPOWER_TXPOWER_Neg40dBm << RADIO_TXPOWER_TXPOWER_Pos);
            break;
        default:
            // Por defecto máxima potencia de transmisión (+4 dBm)
            reg_val = (RADIO_TXPOWER_TXPOWER_Pos4dBm << RADIO_TXPOWER_TXPOWER_Pos);
            break;
    }
    NRF_RADIO->TXPOWER = reg_val;
}

void BleDriver::disconnect() {
    if (BLE.connected()) {
        BLE.disconnect();
    }
}

bool BleDriver::isConnected() {
    return BLE.connected();
}
