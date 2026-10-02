import os

content = """# Arquitectura de Tareas RTOS y Flujo de Datos

Este documento describe la arquitectura final de Tareas (Threads) del RTOS, detallando las responsabilidades de cada una, cómo interactúan entre sí, a qué drivers de hardware tienen acceso y qué repositorios de software utilizan para almacenar la información.

## 1. Diagrama General de Arquitectura (Interacciones)

El siguiente diagrama muestra el flujo de comunicación desde el hardware hasta las colas de mensajes (Mailboxes) de las tareas del RTOS.

```mermaid
flowchart TD
    %% HARDWARE
    subgraph HW [Hardware Físico]
        BHI260[BHI260AP IMU]
        BQ25120A[PMIC y ADC Batería]
        RGB[LED RGB]
        VIB[Motor Vibrador]
        BTNS[Botones Físicos]
        UART[UART Serial]
    end

    %% DRIVERS
    subgraph DRV [Capa de Drivers]
        BhiDrv[BHI260 Driver]
        BatDrv[Battery Driver]
        LedDrv[LED Driver]
        VibDrv[Vibrator Driver]
        BtnDrv[Button Driver]
        CommDrv[Comm Driver]
    end

    %% ISRs
    subgraph ISR_Group [Rutas de Interrupción ISR]
        HW_ISR[Pines de Interrupción]
        TICK_ISR[Timers Cíclicos Mbed]
    end

    %% REPOSITORIES
    subgraph REPO [Repositorios de Datos]
        ImuRepo[(IMU Repo)]
        PressRepo[(Pressure Repo)]
        TempRepo[(Temp Repo)]
        StepRepo[(Step Repo)]
        MotRepo[(Motion Repo)]
        NoMotRepo[(NoMotion Repo)]
    end

    %% TASKS
    subgraph TASKS [Capa RTOS - Tareas]
        SysTask[System Task]
        AlarmsTask[Alarms y Events Task]
        CommTask[Comm Link Task]
        NotifTask[Notify y Telemetry Task]
        PmicTask[PMIC Task]
    end

    %% HW to Drivers
    BHI260 <--> BhiDrv
    BQ25120A <--> BatDrv
    RGB <--> LedDrv
    VIB <--> VibDrv
    BTNS --> BtnDrv
    UART <--> CommDrv

    %% ISR to Tasks
    HW_ISR -->|Encola Evento| SysTask
    TICK_ISR -->|Encola Evento| CommTask
    TICK_ISR -->|Encola Evento| NotifTask

    %% Tasks to Tasks (Mailboxes)
    SysTask -->|Comandos MDE / Eval| AlarmsTask
    SysTask -->|Comandos UI / Leds| NotifTask
    SysTask -->|S.O.S / KeepAlive| CommTask
    AlarmsTask -->|Trama Caída / Buffer| CommTask
    NotifTask -->|Trama Batería / Métricas| CommTask

    %% Tasks to Drivers
    SysTask --> BhiDrv
    SysTask --> BtnDrv
    AlarmsTask --> BhiDrv
    CommTask --> CommDrv
    NotifTask --> LedDrv
    NotifTask --> VibDrv
    NotifTask --> BatDrv
    PmicTask --> BatDrv

    %% Tasks to Repositories
    SysTask -.->|Lee Estado| MotRepo
    SysTask -.->|Lee Estado| NoMotRepo
    AlarmsTask -.->|Escribe| ImuRepo
    AlarmsTask -.->|Escribe| PressRepo
    AlarmsTask -.->|Escribe| TempRepo
    AlarmsTask -.->|Escribe| StepRepo
    AlarmsTask -.->|Limpia| MotRepo
    AlarmsTask -.->|Limpia| NoMotRepo
    
    CommTask -.->|Lee Trama| ImuRepo
    CommTask -.->|Lee Trama| PressRepo
    CommTask -.->|Lee Trama| TempRepo
    
    NotifTask -.->|Extrae Métricas| StepRepo
```

---

## 2. Descripción de las Tareas (Threads)

A continuación, se detalla el rol, los disparadores, la comunicación y el acceso de cada tarea.

### A. System Task (`SystemTask`)
*   **Rol / Responsabilidad:** Actúa como el orquestador principal (Master) del sistema. Maneja las Máquinas de Estado (MDE) del acelerómetro BHI260 para minimizar el consumo energético (hibernación y despertar) y gestiona los rebotes/estados de los botones físicos (Notificación y Pánico).
*   **Disparadores y Condiciones:**
    *   La tarea duerme esperando un mensaje en su buzón (Mailbox).
    *   **ISRs de Hardware:** Cuando un botón se presiona o el IMU genera una interrupción, sus respectivas funciones ISR (`panic_button_isr`, `bhi_isr_handler`) inyectan un comando en el buzón de esta tarea (`CMD_EVALUATE_PANIC_BUTTON`, `EVT_BHI_INTERRUPT`).
    *   **Timeouts Condicionales:** Si un botón está en pleno análisis de rebote, la tarea reemplaza la espera infinita por un _timeout_ corto (ej. 10ms) para procesar activamente las máquinas de estado locales (`updateButtonsMde()`).
*   **Interacciones (Envía Mensajes a):**
    *   `AlarmsEventsTask`: Le ordena drenar los buffers del IMU (`UPDATE_BUFFER_BHI`) y evaluar el algoritmo de caídas (`CMD_PROCESS_IMU`).
    *   `NotifyTelemetryTask`: Le manda notificaciones para la UI (Encender/apagar LEDs).
    *   `CommLinkTask`: Reporta apretadas del botón de pánico (`CMD_TX_PANIC_BTN_PRESS`).
*   **ISRs de Tareas Cíclicas:** Posee Mbed Tickers (timers asíncronos por hardware) que, al agotarse su tiempo (`KEEP_ALIVE_PERIOD_SEC` o `METRICS_PERIOD_SEC`), mandan mensajes directamente al buzón de `CommLinkTask` y `NotifyTelemetryTask` respectivamente.
*   **Hardware y Repositorios:** Interactúa con `BHI260Driver` (para prender/apagar FIFOs) y `ButtonDriver`. Lee los repositorios lógicos de gestos `MotionRepository` y `NoMotionRepository` para decidir transiciones de estado.

### B. Alarms & Events Task (`AlarmsEventsTask`)
*   **Rol / Responsabilidad:** Ejecutar el procesamiento algorítmico y matemático complejo. Extrae los datos desde el sensor IMU (I2C) y corre la lógica (Ventana de análisis) que determina si el usuario se cayó o no.
*   **Disparadores y Condiciones:** Duerme bloqueada de forma infinita hasta que `SystemTask` le envía un mensaje por buzón pidiéndole actualizar memoria o procesar un impacto.
*   **Interacciones (Envía Mensajes a):**
    *   `CommLinkTask`: Si detecta una caída u otro evento, empaqueta los datos de alerta (`CMD_TX_ALARM`) y le ordena extraer el historial completo (snapshot) para su envío por serie (`CMD_TX_IMU_BUFFER`).
*   **Hardware y Repositorios:** Llama recurrentemente a `BHI260Driver->getSensorData()`. Actualiza _casi todos_ los repositorios con nuevos datos: `ImuRepository`, `PressureRepository`, `TemperatureRepository`, y `StepCounterRepository`.

### C. Notify & Telemetry Task (`NotifyTelemetryTask`)
*   **Rol / Responsabilidad:** Gestionar toda la retroalimentación hacia el usuario (Interfaces y Feedback) y agrupar los datos de telemetría secundaria (Métricas y Batería) antes de comunicarlos.
*   **Disparadores y Condiciones:** Duerme permanentemente esperando comandos de interfaz o solicitudes de estado cíclicas. El Timer cíclico originado en `SystemTask` deposita solicitudes periódicas aquí.
*   **Interacciones (Envía Mensajes a):**
    *   `CommLinkTask`: Una vez que la tarea recopila los datos del estado de la batería o cuenta de pasos actual, encola mensajes (`CMD_TX_BATTERY_DATA`, `CMD_TX_METRICS`) conteniendo los _pointers_ a la memoria de esos datos para que el enlace de red los envíe hacia afuera.
*   **Hardware y Repositorios:** Es la única dueña de `LedDriver` y `VibratorDriver`. También consulta a `BatteryDriver` por voltajes/estados de carga. Extrae y blanquea el conteo de pasos desde el `StepCounterRepository`.

### D. Communication Link Task (`CommLinkTask`)
*   **Rol / Responsabilidad:** Encargada del envío y recepción hacia el mundo exterior (Por ahora un enlace UART "By-pass", a futuro un módulo BLE o Lora). Se responsabiliza de empaquetar, calcular CRC, y asegurar la integridad en las tramas salientes.
*   **Disparadores y Condiciones:** Duerme permanentemente. Únicamente se despierta cuando cualquier otra tarea del sistema le deposita un paquete (o un _pointer_ al mismo) en su Mailbox solicitando que lo transmita.
*   **Interacciones:** Funciona como un _sumidero_ pasivo. Las demás tareas se comunican _hacia_ ella, pero ella solo transmite por hardware (Tx) hacia afuera.
*   **Hardware y Repositorios:** Domina en exclusiva el `CommDriver`. Para tramas pesadas (como el snapshot pre/post impacto), en lugar de traficar megabytes por mensajes, esta tarea lee directamente los búferes en anillo en el `ImuRepository` y `PressureRepository` para enviarlos fragmentadamente.

### E. PMIC Task (`PmicTask`)
*   **Rol / Responsabilidad:** Tarea simplificada de ultra-bajo consumo cuya única meta es evitar que el controlador de energía (BQ25120A) se apague por seguridad, mediante la técnica de "Watchdog Pinging".
*   **Disparadores y Condiciones:** No posee mailbox ni espera mensajes de nadie. Posee un bucle infinito con un descanso (`rtos::ThisThread::sleep_for()`) de 40 segundos exactos.
*   **Interacciones:** Nulas. Es una isla independiente.
*   **Hardware y Repositorios:** Hace uso del `BatteryDriver` para enviar comandos I2C al PMIC.

---

## 3. Manejo de IRQs (Hardware e ISRs)

La filosofía del RTOS impone que los tiempos muertos dentro de una ISR (Interrupt Service Routine) de hardware sean nulos o mínimos, prohibiendo totalmente las llamadas I2C/UART bloqueantes.

*   **Pines de Hardware (Botones e IMU):** Los rebotes de GPIO o el pin HINT del BHI260 disparan funciones en código asíncrono. Ninguna de estas funciones ejecuta lógica. Su única directiva es construir un `AppMessage` extremadamente corto y enviarlo mediante `_mailbox.try_put()` o equivalentes a la `SystemTask`. Esto saca al procesador de la IRQ inmediatamente y delega todo el cómputo pesado al entorno seguro del RTOS Threading de la System Task.
*   **Tickers Cíclicos (Keep Alive / Metrics):** Los envíos periódicos de salud (Ping o Métricas) se gestionan configurando un temporizador de Mbed (`mbed::Ticker`). Dicho temporizador se asocia a interrupciones por Timer nativas. Al saltar la IRQ, se ejecuta una función corta (`onKeepAliveTick` o `onMetricsTick` en `SystemTask`) que se encarga exclusivamente de encolar un mensaje de disparo hacia `CommLinkTask` o `NotifyTelemetryTask` respectivamente. De esta manera, el RTOS despierta armónicamente las tareas en el próximo ciclo sin consumir procesador en bucles de espera pasiva.
"""

with open('docs/ARQUITECTURA_TAREAS.md', 'w', encoding='utf-8') as f:
    f.write(content)
