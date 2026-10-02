# Plan de Implementación: Tareas Notify Telemetry y PMIC

## Objetivo
Reestructurar la arquitectura de tareas cíclicas y de notificaciones unificándolas bajo una tarea multipropósito `NotifyTelemetryTask`, e independizar el watchdog de batería en una tarea ultra-sencilla `PmicTask`.

---

## 1. PMIC Task (Nuevo)
Crearemos una tarea extremadamente sencilla y de bajo consumo exclusivamente para mantener al PMIC BQ25120A despierto. No tendrá mailbox, solo un ciclo con un delay.

**`include/tasks/pmic_task.h`**
```cpp
#pragma once
#include <mbed.h>

class PmicTask {
private:
    PmicTask() {}
    ~PmicTask() {}
    PmicTask(const PmicTask&) = delete;
    PmicTask& operator=(const PmicTask&) = delete;

    rtos::Thread _thread;
    void run();

public:
    static PmicTask& getInstance() {
        static PmicTask instance;
        return instance;
    }
    void init();
};
```

**`src/tasks/pmic_task.cpp`**
```cpp
#include "tasks/pmic_task.h"
#include "drivers/battery_driver.h"

void PmicTask::init() {
    _thread.start(mbed::callback(this, &PmicTask::run));
}

void PmicTask::run() {
    while (true) {
        // Ping cada 40 segundos
        BatteryDriver::getInstance()->ping();
        rtos::ThisThread::sleep_for(std::chrono::milliseconds(40000));
    }
}
```

---

## 2. Notify & Telemetry Task (Fusión)
Vamos a reemplazar y renombrar el `notif_ui_task` por `notify_telemetry_task`, y descartar el `telemetry_task` anterior.

**Estructuras de datos a compartir:**
```cpp
struct BatteryData {
    uint8_t charge_percent;
    bool is_charging;
};

struct ActivityData {
    uint32_t step_count;
    // futuras metricas
};
```

**Mecanismo de Despertado (Autónomo):**
Se utilizará un chequeo no bloqueante de la cola (por ejemplo un timeout con `try_get_for` en Mbed OS) para poder:
1. Atender mensajes instantáneamente.
2. Si el timeout expira (ej. no llegaron mensajes en 1 segundo), la tarea revisa si toca hacer un envío de Keep Alive o actualizar telemetría sin depender de avisos del `SystemTask`.

**Comandos Soportados en el Mailbox:**
* `CMD_VIBRATOR_GESTURE` (flags: id del gesto)
* `CMD_LED_NOTIFY` (flags: color / modo)
* `CMD_KEEP_ALIVE_ENA`
* `CMD_KEEP_ALIVE_DES`
* `CMD_FORCE_KEEP_ALIVE`
* `CMD_NOTIF_BATTERY_DATA`
* `CMD_NOTIF_ACTIVITY_DATA`

---

## 3. Integración con el Sistema y CommLink
*   **`app_messages.h`**: Se definen los nuevos IDs `TASK_NOTIFY_TELEMETRY` y `TASK_PMIC`.
*   **`app.cpp`**: Se elimina la inicialización vieja y se inicializan las dos nuevas tareas.
*   **`comm_link_task.h / .cpp`**: 
    Se añadirán los eventos correspondientes para recibir batería y actividad.
    Cuando `CommLinkTask` es avisada de que hay nueva telemetría, llamará a `NotifyTelemetryTask::getSharedBatteryData()` copiando los datos a un buffer interno sin usar `malloc`.

---

## Preguntas Abiertas para Feedback

1. **Mecanismo de timeout:** ¿Estás de acuerdo con usar el timeout de la cola de mensajes (polling temporal) para que la tarea se despierte de los eventos cíclicos, o preferís alguna otra aproximación de Mbed OS (EventFlags + Timer)?
2. **Estructura MetricsData:** Actualmente en `comm_link_task.h` existe un `MetricsData` con `step_count` y un `CMD_TX_METRICS`. ¿Querés que reemplacemos eso directamente por `ActivityData` y `CMD_NOTIF_ACTIVITY_DATA` para mantener los nombres que propusiste?

## Resultados de Pruebas

Se valido satisfactoriamente el modulo a traves de una secuencia de arranque (temporal) en SystemTask. Los resultados fueron:
- **Timers C�clicos**: El encolado de keep alive, peticion de datos de bateria y lectura manual de metricas responden correctamente.
- **LED Driver**: Responde adecuadamente a comandos de color (Rojo, Verde, Azul, Apagado) via mailbox.
- **Python Script**: Parseo actualizado en el host compatible con MSG_METRICS de 16-bits y el nuevo MSG_BATTERY_DATA.

**Pendientes:**
- Testear lectura real con bateria fisica conectada (actualmente reporta 0%).
- Testear vibrador cuando este disponible (gestos hapticos).
