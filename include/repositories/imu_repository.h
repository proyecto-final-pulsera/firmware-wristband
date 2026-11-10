#pragma once
#include <Arduino.h>
#include <mbed.h>
#include "sensors/SensorClass.h"
#include "drivers/bhi260_driver.h"

#include "app/config.h"

/**
 * @brief Repository de alto nivel para el sensor IMU (Acelerómetro) heredado de SensorClass.
 * Esta clase se suscribe a los paquetes del BHI260AP y mantiene un buffer circular
 * (FIFO) propio para almacenar los últimos datos sin bloquear el sistema principal.
 */
class ImuRepository : public SensorClass {
public:
    /**
     * @brief Singleton access
     */
    static ImuRepository* getInstance();

private:
    /**
     * @brief Constructor por defecto. Inicializa la clase con el ID de acelerómetro
     * no-wakeup (BHI260Driver::ID_ACCEL) y resetea la FIFO.
     */
    ImuRepository();

public:
    virtual ~ImuRepository();

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
     * @brief Permite acceder a un elemento específico del buffer por copia protegido por Mutex.
     * @param index Índice lógico (0 es el dato más antiguo, getAvailableCount()-1 es el más nuevo).
     * @return Copia del dato, o un objeto default si el índice está fuera de rango.
     */
    DataXYZ getElementAt(uint16_t index);

    /**
     * @brief Permite acceder a un elemento específico del buffer por copia SIN proteger por Mutex.
     */
    DataXYZ getElementAtUnprotected(uint16_t index) const;

    /**
     * @brief Extrae todos los datos actuales de la FIFO en bloque (hace pop de los elementos).
     * @param buffer Puntero a un array provisto por el usuario donde se copiarán los datos.
     * @param maxLen Capacidad del buffer provisto (para evitar buffer overflow).
     * @return Cantidad de elementos extraídos.
     */
    uint16_t getFifoValues(DataXYZ* buffer, uint16_t maxLen);

    /**
     * @brief Lineariza la FIFO circular en el buffer interno _fifo (in-place)
     * y retorna el puntero directo para transmisión sin requerir buffers duplicados.
     * @param outCount Puntero donde se devuelve la cantidad de elementos disponibles.
     * @return Puntero constante a los datos contiguos en _fifo, o nullptr si está vacía.
     */
    const DataXYZ* getLinearBuffer(uint16_t* outCount);

    /**
     * @brief Copia los datos actuales de la FIFO sin extraerlos (sin pop).
     */
    uint16_t copyFifoValues(DataXYZ* buffer, uint16_t maxLen);

    /**
     * @brief Copia los datos actuales de la FIFO sin extraerlos, comenzando desde el índice indicado.
     */
    uint16_t copyFifoValues(DataXYZ* buffer, uint16_t maxLen, uint16_t index);

    /**
     * @brief Copia los datos actuales de la FIFO sin extraerlos y sin protección Mutex.
     */
    uint16_t copyFifoValuesUnprotected(DataXYZ* buffer, uint16_t maxLen, uint16_t index) const;

    /**
     * @brief Algoritmos matemáticos de aproximación 3D para cálculo de magnitudes.
     */
    static inline uint16_t approx2DImproved(uint16_t a, uint16_t b) {
        uint16_t max_val = (a > b) ? a : b;
        uint16_t min_val = (a < b) ? a : b;
        uint16_t z0 = max_val;
        uint16_t z1_max = max_val - (max_val >> 3); 
        uint16_t z1_min = (min_val >> 1) + (min_val >> 5);
        uint16_t z1 = z1_max + z1_min;
        return (z0 > z1) ? z0 : z1;
    }

    static uint16_t sumaPitagorica(int16_t x, int16_t y, int16_t z);

    /**
     * @brief Calcula la magnitud 3D de las muestras de la FIFO y las copia en el buffer provisto (protegido por Mutex).
     * @param buffer Array de destino provisto por el llamador.
     * @param maxLen Capacidad máxima a escribir.
     * @param index Índice lógico inicial (0 es el dato más antiguo disponible).
     * @return Cantidad de magnitudes calculadas y copiadas.
     */
    uint16_t copyFifoMagnitudes(uint16_t* buffer, uint16_t maxLen, uint16_t index = 0);

    void lock();
    void unlock();

    void fifoFlushUnprotected();
    const DataXYZ* getLinearBufferUnprotected(uint16_t* outCount);
    bool popUnprotected(DataXYZ& data);
    uint16_t getAvailableCountUnprotected() const;

    // --- Métodos temporales de Test ---
    uint32_t getTotalPushed() const;
    void resetTotalPushed();

private:
    mutable rtos::Mutex _mutex;
    DataXYZ _fifo[IMU_FIFO_SIZE];
    uint16_t _head;   // Índice donde se insertará el próximo elemento
    uint16_t _tail;   // Índice del elemento más antiguo para extraer
    uint16_t _count;  // Cantidad de elementos actuales
    bool _overflow;   // Flag para saber si se pisaron datos
    DataXYZ _lastData;// Mantiene el último dato para toString()
    
    uint32_t _totalPushed; // Temporario para medir la profundidad de HW
};
