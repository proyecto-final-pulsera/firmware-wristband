#pragma once
#include <Arduino.h>
#include "BQ25120A.h"
#include "Nicla_System.h"

/**
 * @class BatteryDriver
 * @brief Controlador para gestionar y monitorear el estado de la batería.
 *        Interactúa con el PMIC (Power Management IC) de la placa Nicla.
 */
class BatteryDriver
{
public:
    static BatteryDriver* getInstance();
    
    // Prohibimos copia y asignación (refuerzo del Singleton)
    BatteryDriver(const BatteryDriver&) = delete;
    BatteryDriver& operator=(const BatteryDriver&) = delete;

    /**
     * @brief Inicializa las configuraciones necesarias para leer la batería.
     */
    void init();

    /**
     * @brief Obtiene el nivel o porcentaje de carga de la batería.
     * @return Nivel de carga de la batería.
     */
    uint8_t getBatteryCharge(); 
    
    /**
     * @brief Obtiene el estado operativo del PMIC (ej. Cargando, Batería llena, Error).
     * @return Estado operativo actual.
     */
    OperatingStatus getOperatingStatus();
    
    /**
     * @brief Determina si la placa está siendo alimentada por la batería.
     * @return true si se alimenta de la batería, false si es por USB/VIN.
     */
    bool runsOnBattery();
    
    /**
     * @brief Obtiene la tensión actual de la batería.
     * @return Tensión actual en Volts.
     */
    float getCurrentVoltage();

    /**
     * @brief Envía un ping manual por I2C al PMIC.
     * Sirve para reiniciar el watchdog timer del chip y evitar que entre en bajo consumo.
     */
    void ping();

private:
    static BatteryDriver* _instance;
    static BatteryDriver* createInstance();

    BatteryDriver();

    // Instancia de la clase nicla para interactuar con la API del sistema
    nicla _board;
};