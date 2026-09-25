#include "drivers/battery_driver.h"

// ==================================================================
// Singleton
// ==================================================================
BatteryDriver* BatteryDriver::_instance = nullptr;

BatteryDriver* BatteryDriver::getInstance() {
    if (_instance == nullptr) {
        _instance = createInstance();
    }
    return _instance;
}

BatteryDriver* BatteryDriver::createInstance() {
    return new BatteryDriver();
}

// ==================================================================
// Implementación de BatteryDriver
// ==================================================================

BatteryDriver::BatteryDriver() {
    // El init() debe llamarse explicitamente por quien instancia el driver
}

void BatteryDriver::init() {
    // Inicializar el controlador del sistema Nicla (PMIC BQ25120A, etc.)
    nicla::begin();
    
    // Habilitar el LDO de 3.3V
    nicla::enable3V3LDO();

    // Habilitar carga a 50mA
    nicla::enableCharging(50);
    
    // Configurar el temporizador de seguridad de carga (3 horas)
    nicla::configureChargingSafetyTimer(ChargingSafetyTimerOption::ThreeHours);
}

bool BatteryDriver::runsOnBattery() {
    return nicla::runsOnBattery();
}

float BatteryDriver::getCurrentVoltage() {
    return nicla::getCurrentBatteryVoltage();
}

void BatteryDriver::ping() {
    // Para reiniciar el watchdog de 50 segundos del PMIC, realizamos una lectura I2C
    // directamente al registro del PMIC utilizando la instancia expuesta por nicla.
    nicla::_pmic.getFastChargeControlRegister();
}

uint8_t BatteryDriver::getBatteryCharge() {
    // Obtenemos el porcentaje de tensión (puede retornar -1 si falla, por lo que casteamos con precaución)
    int8_t percentage = nicla::getBatteryVoltagePercentage();
    
    // Si hubo un error o valor inválido, retornamos 0
    if (percentage < 0) {
        return 0;
    }
    
    return static_cast<uint8_t>(percentage);
}

OperatingStatus BatteryDriver::getOperatingStatus() {
    // Retornamos el estado operativo reportado por el PMIC
    return nicla::getOperatingStatus();
}