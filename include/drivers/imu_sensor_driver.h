#pragma once
#include <Arduino.h>
#include "sensors/SensorClass.h"
#include "drivers/bhi260_driver.h"

// Capacidad máxima de la FIFO circular para el IMU
#define IMU_FIFO_SIZE 500

/**
 * @brief Driver de alto nivel para el sensor IMU (Acelerómetro) heredado de SensorClass.
 * Esta clase se suscribe a los paquetes del BHI260AP y mantiene un buffer circular
 * (FIFO) propio para almacenar los últimos datos sin bloquear el sistema principal.
 */
class ImuSensorDriver : public SensorClass {
public:
    /**
     * @brief Constructor por defecto. Inicializa la clase con el ID de acelerómetro
     * no-wakeup (BHI260Driver::ID_ACCEL) y resetea la FIFO.
     */
    ImuSensorDriver();
    
    virtual ~ImuSensorDriver();

    // --- Métodos virtuales puros reescritos de SensorClass ---
    
    /**
     * @brief Recibe un paquete estándar de datos desde el parser de Bosch.
     * Convierte el paquete a DataXYZ y lo pushea a la FIFO circular.
     */
    void setData(SensorDataPacket &data) override;
    
    /**
     * @brief Recibe un paquete largo de datos desde el parser de Bosch.
     * En el caso del acelerómetro no se utiliza (se ignora).
     */
    void setData(SensorLongDataPacket &data) override;
    
    /**
     * @brief Convierte el último dato a formato String para debug.
     */
    String toString() override;

    // --- Métodos de la FIFO Circular ---

    /**
     * @brief Limpia la FIFO vaciando los datos (resetea los índices head/tail).
     */
    void fifoFlush();

    /**
     * @brief Agrega un dato al final de la FIFO. Si la FIFO está llena, 
     * sobrescribe el dato más antiguo.
     * @param data Estructura DataXYZ a pushear.
     * @return true si se pusheó correctamente sin sobrescribir, false si hubo desbordamiento.
     */
    bool push(const DataXYZ& data);

    /**
     * @brief Extrae el dato más antiguo de la FIFO.
     * @param data Referencia donde se copiará el dato extraído.
     * @return true si había datos disponibles, false si la FIFO está vacía.
     */
    bool pop(DataXYZ& data);

    /**
     * @brief Retorna la cantidad actual de datos no leídos en la FIFO.
     * @return Número de elementos disponibles.
     */
    uint16_t getAvailableCount() const;

    /**
     * @brief Indica si el buffer circular está lleno.
     */
    bool isFull() const;

    /**
     * @brief Indica si el buffer ha sobrescrito datos (overflow) desde la última limpieza.
     */
    bool hasOverflowed() const;

    /**
     * @brief Limpia el flag de overflow.
     */
    void clearOverflow();

    /**
     * @brief Retrocede el índice de lectura (tail) para "recuperar" datos que ya fueron extraídos (popped).
     * @param steps Cantidad de datos a recuperar.
     * @return La cantidad real de datos recuperados (puede ser menor a steps si no hay suficiente historial).
     */
    uint16_t rewind(uint16_t steps);

    /**
     * @brief Permite acceder a un elemento específico del buffer sin copiarlo.
     * @param index Índice lógico (0 es el dato más antiguo, getAvailableCount()-1 es el más nuevo).
     * @return Puntero constante al dato, o nullptr si el índice está fuera de rango.
     */
    const DataXYZ* getElementAt(uint16_t index) const;

    /**
     * @brief Extrae todos los datos actuales de la FIFO en bloque.
     * @param buffer Puntero a un array provisto por el usuario donde se copiarán los datos.
     * @param maxLen Capacidad del buffer provisto (para evitar buffer overflow).
     * @return Cantidad de elementos extraídos.
     */
    uint16_t getFifoValues(DataXYZ* buffer, uint16_t maxLen);

    // --- Métodos temporales de Test ---
    uint32_t getTotalPushed() const;
    void resetTotalPushed();

private:
    DataXYZ _fifo[IMU_FIFO_SIZE];
    uint16_t _head;   // Índice donde se insertará el próximo elemento
    uint16_t _tail;   // Índice del elemento más antiguo para extraer
    uint16_t _count;  // Cantidad de elementos actuales
    bool _overflow;   // Flag para saber si se pisaron datos
    DataXYZ _lastData;// Mantiene el último dato para toString()
    
    uint32_t _totalPushed; // Temporario para medir la profundidad de HW
};
