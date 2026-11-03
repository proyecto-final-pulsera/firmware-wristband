#pragma once
#include "drivers/comm_driver.h"
#include "Nicla_System.h"
#include <ArduinoBLE.h>

class BleDriver : public CommDriver {
private:
    BleDriver() = default;

    // Singleton pattern
    BleDriver(const BleDriver&) = delete;
    BleDriver& operator=(const BleDriver&) = delete;

    // Punteros a la estructura GATT (Solo Servicio Custom de Comunicación)
    BLEService* _commService = nullptr;
    BLECharacteristic* _txCharacteristic = nullptr;
    BLECharacteristic* _rxCharacteristic = nullptr;

    // Servicios estándar GATT
    BLEService* _batteryService = nullptr;
    BLEUnsignedCharCharacteristic* _batteryLevelChar = nullptr;
    // Device Information Service comentado para maximizar holgura en Heap (~350 B):
    // BLEService* _deviceInfoService = nullptr;
    // BLEStringCharacteristic* _manufacturerNameChar = nullptr;
    // BLEStringCharacteristic* _firmwareRevChar = nullptr;

    // Máquina de estados para fragmentación
    enum class TxState {
        IDLE,
        TX_EN_PROGRESO
    };

    TxState _txState = TxState::IDLE;
    MsgType _currentTxMsgType;
    const uint8_t* _txBuffer = nullptr;
    uint16_t _txBufferLen = 0;
    uint16_t _txOffset = 0;
    uint16_t _chunkSequence = 0;
    uint16_t _totalChunks = 0;
    uint8_t _globalCrc = 0;

    // MTU seguro para compatibilidad a Bluetooth 4.2 a partir de 2014
    static const uint16_t BLE_MTU_PAYLOAD = 128; // El maximo seria 244

    // Buffer temporal para recepción (RX)
    uint8_t _rxBuffer[128];
    uint16_t _rxIndex = 0;

    // Buffer para armado de paquetes de transmision (TX) fuera del stack de CommLinkTask
    uint8_t _txPacketBuffer[BLE_MTU_PAYLOAD];

    // Callbacks estáticos de interrupción BLE
    static void onBleWrite(BLEDevice central, BLECharacteristic characteristic);
    static void onBleConnect(BLEDevice central);
    static void onBleDisconnect(BLEDevice central);

public:
    static BleDriver* getInstance();

    // =========================================================================
    // Contrato Base (CommDriver)
    // =========================================================================
    void init() override;
    void attachRxInterrupt() override;
    bool processRxData(CommPacket* out_msg) override;
    bool sendPayload(MsgType type, const uint8_t* payload, uint16_t len) override;

    // =========================================================================
    // Métodos propios de BLE
    // =========================================================================
    
    /// Niveles de potencia de transmisión de la antena de radio nRF52 (TX Power)
    enum class TxPower : int8_t {
        POS_4_DBM  = 4,   ///< +4 dBm (Máxima potencia nRF52832, mayor alcance)
        ZERO_DBM   = 0,   ///<  0 dBm (Potencia estándar)
        NEG_4_DBM  = -4,  ///< -4 dBm
        NEG_8_DBM  = -8,  ///< -8 dBm
        NEG_12_DBM = -12, ///< -12 dBm
        NEG_16_DBM = -16, ///< -16 dBm
        NEG_20_DBM = -20, ///< -20 dBm
        NEG_40_DBM = -40  ///< -40 dBm (Ahorro máximo de batería)
    };

    /**
     * @brief Configura la potencia de transmisión de la antena BLE.
     * @param power Nivel de potencia deseado (ej: TxPower::POS_4_DBM).
     */
    void setTxPower(TxPower power);

    /**
     * @brief Configura la potencia de transmisión de la antena BLE en dBm.
     * @param power_dbm Potencia en dBm (+4, 0, -4, -8, -12, -16, -20, -40).
     */
    void setTxPower(int8_t power_dbm);

    /// Indica si el motor asíncrono está ocupado transmitiendo fragmentos
    bool isTxInProgress() const { return _txState == TxState::TX_EN_PROGRESO; }

    // Debe llamarse periódicamente en el super-loop o tarea para que el engine despache fragmentos
    void processBleEvents();

    // Publicar batería en el servicio estándar
    void updateBatteryLevel(uint8_t level);
};
