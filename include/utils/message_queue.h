#pragma once
#include <mbed.h>
#include "tasks/app_messages.h"

/**
 * @class MessageQueue
 * @brief OS Abstraction Layer (OSAL) para el envio de mensajes entre tareas.
 */
template <typename T, uint32_t SIZE>
class MessageQueue {
private:
    rtos::Mail<T, SIZE> _mail;

    // --- Magia de C++ (Sobrecarga de funciones) ---
    // Si T es exactamente 'AppMessage', el compilador usa esta funcion y estampa el tiempo
    void apply_helpers(AppMessage* msg) {
        // Asignamos el timestamp actual automaticamente
        msg->timestamp = rtos::Kernel::get_ms_count();
    }
    
    // Si T es cualquier otro tipo (ej. int, float), el compilador usa esta funcion que no hace nada
    void apply_helpers(void* msg) {
        // No operation
    }

public:
    /**
     * @brief Encola un mensaje en el RTOS de forma segura usando Mailbox.
     * 
     * @note SOBRE LA PRIORIDAD (FUTURA IMPLEMENTACION):
     *       Actualmente estamos usando rtos::Mail, el cual es estricto FIFO.
     *       Si a futuro necesitamos habilitar prioridad por hardware en base al
     *       campo 'priority_level', podemos modificar esta clase internamente
     *       reemplazando el rtos::Mail por un rtos::MemoryPool y un rtos::Queue 
     *       por separado. Al hacer eso, podriamos usar _queue.try_put(msg, prioridad).
     *       Como esta clase encapsula la logica, las tareas no notaran el cambio.
     */
    bool send(const T* msg) {
        T* new_msg = _mail.try_alloc();
        if (new_msg == nullptr) {
            return false; // Buzon lleno
        }
        
        // Copiamos los datos del usuario al bloque de memoria del mailbox
        *new_msg = *msg;
        
        // Inyectamos el timestamp (solo si es un AppMessage)
        apply_helpers(new_msg);

        // Encolamos en el mailbox
        _mail.put(new_msg);
        return true;
    }

    /**
     * @brief Verifica si el buzon esta vacio.
     * @return true si esta vacio, false si hay mensajes esperando.
     */
    bool empty() const {
        return _mail.empty();
    }

    /**
     * @brief Verifica si el buzon esta lleno.
     * @return true si ya alcanzo su limite, false si hay espacio.
     */
    bool full() const {
        return _mail.full();
    }

    /**
     * @brief Saca un mensaje del Mailbox (si lo hay) y libera la memoria automaticamente.
     * @param msg_out Puntero donde se volcaran los datos extraidos.
     * @param timeout_ms Tiempo maximo a esperar (por defecto espera por siempre).
     * @return true si recibio un mensaje, false si ocurrio un timeout.
     */
    bool receive(T* msg_out, uint32_t timeout_ms = rtos::Kernel::wait_for_u32_forever) {
        T* received_ptr = _mail.try_get_for(timeout_ms);
        if (received_ptr != nullptr) {
            *msg_out = *received_ptr; // Copiamos al espacio del usuario
            _mail.free(received_ptr); // Devolvemos el sobre vacio al mailbox
            return true;
        }
        return false;
    }
};
