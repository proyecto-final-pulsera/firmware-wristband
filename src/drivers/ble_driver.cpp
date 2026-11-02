#include "drivers/ble_driver.h"

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
    _batteryLevelChar = new BLEUnsignedCharCharacteristic("2A19", BLERead | BLENotify);
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

    // Hack de bajo nivel: Forzar la potencia de transmisión de la radio al máximo (+4 dBm)
    // Por algun motivo no tiene el #define RADIO_TXPOWER_TXPOWER_Pos8dBm en los archivos de nRF
    // El chip nRF52840 (NINA-B306) lo soporta nativamente.
    // Escribimos directamente en el registro hardware del microcontrolador saltándonos la librería.
    NRF_RADIO->TXPOWER = (RADIO_TXPOWER_TXPOWER_Pos4dBm << RADIO_TXPOWER_TXPOWER_Pos);
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
        
    if (_txState != TxState::IDLE) {
        return false; // El motor está ocupado despachando fragmentos anteriores
    }

    if (len <= BLE_MTU_PAYLOAD - 1) {
        // Envío Directo (Sin fragmentar) -> [MsgType] [Payload...]
        uint8_t packet[BLE_MTU_PAYLOAD];
        packet[0] = type;
        if (len > 0 && payload != nullptr) {
            memcpy(&packet[1], payload, len);
        }
        _txCharacteristic->writeValue(packet, len + 1);
        return true;
    } else {
        // Envío Fragmentado -> Pasa al motor asíncrono
        _txState = TxState::TX_EN_PROGRESO;
        _currentTxMsgType = type;
        _txBuffer = payload;
        _txBufferLen = len;
        _txOffset = 0;
        _chunkSequence = 0;
        
        // Calculamos el total de fragmentos. Header es de 6 bytes, nos quedan 14 libres por chunk
        uint16_t maxSizePerChunk = BLE_MTU_PAYLOAD - 6;
        _totalChunks = (len + maxSizePerChunk - 1) / maxSizePerChunk;
        
        // Calcular CRC8 global de todo el payload
        _globalCrc = calculateCRC8(payload, len);
        
        while(_txState == TxState::TX_EN_PROGRESO){
            processBleEvents();
        }
        return true;
    }
}

void BleDriver::processBleEvents() {
    BLE.poll();

    // Motor de despacho asíncrono
    if (_txState == TxState::TX_EN_PROGRESO) {
        // Validación de conexion y suscripcion a TX
        if (!BLE.connected() || !_txCharacteristic->subscribed()) {
            _txState = TxState::IDLE; // Si se cae, cancelamos
            return;
        }

        uint16_t remaining = _txBufferLen - _txOffset;
        uint8_t packet[BLE_MTU_PAYLOAD];
        
        // Estructura Cabecera de Fragmento: [MsgType] [SeqHigh] [SeqLow] [TotalHigh] [TotalLow] [CRC8_Global] [Payload...]
        packet[0] = _currentTxMsgType;
        packet[1] = (_chunkSequence >> 8) & 0xFF; // Parte alta
        packet[2] = _chunkSequence & 0xFF; // Parte baja
        packet[3] = (_totalChunks >> 8) & 0xFF; // Parte alta
        packet[4] = _totalChunks & 0xFF; // Parte baja
        packet[5] = _globalCrc;

        uint16_t chunkDataLen = BLE_MTU_PAYLOAD - 6;
        if (remaining < chunkDataLen) {
            chunkDataLen = remaining;
        }

        memcpy(&packet[6], &_txBuffer[_txOffset], chunkDataLen);
        _txCharacteristic->writeValue(packet, chunkDataLen + 6);

        _txOffset += chunkDataLen;
        _chunkSequence++;

        if (_txOffset >= _txBufferLen) {
            _txState = TxState::IDLE; // Transmisión completada
        }
    }
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
    Serial.print("App conectada. MAC: ");
    Serial.println(central.address());
    nicla::leds.setColor(green); // Verde indica conectado
}

void BleDriver::onBleDisconnect(BLEDevice central) {
    // Limpiar estado
    BleDriver* instance = getInstance();
    instance->_txState = TxState::IDLE;

    Serial.print("App desconectada. MAC: ");
    Serial.println(central.address());
    nicla::leds.setColor(blue); // Vuelve a azul para indicar desconectado/esperando
}
