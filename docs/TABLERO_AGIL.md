# Sprint Backlog - Firmware Nicla Sense ME

Este archivo servirá como nuestra memoria y hoja de ruta compartida (sprint backlog) para desarrollar el firmware correctamente. Iremos marcando las tareas a medida que las completemos.

## Sprint Actual (24/09/2026)

- [X] **Tarea 16 (Original 3): Driver de comunicación (Bypass Serie + Protocolo)**

  - **Tiempo estimado:** 7 horas
  - **Descripción:** Definir métodos del driver y proveer un bypass para transmitir por puerto serie en lugar de BLE. Se debe implementar un protocolo de tramas simple (ej. `[START] [TIPO] [PAYLOAD] [CHECKSUM]`). Crear script en Python para recibir y parsear.
  - **Criterio de aceptación:** Enviar datos del acelerómetro y barómetro por serie de forma transparente al sistema y poder verlos parseados en el script de Python.
- [X] **Tarea 17: Comunicación de Temperatura**

  - **Descripción:** Implementar el envío de datos de temperatura a través del driver de comunicación utilizando el protocolo de tramas definido.
  - **Criterio de aceptación:** Los datos de temperatura se transmiten correctamente por serie con el formato de trama correspondiente.
- [X] **Tarea 18: Validación de Comunicación de Temperatura**

  - **Descripción:** Verificar que los datos de temperatura recibidos en el script de Python coincidan con los valores leídos por el sensor. Validar integridad de trama (checksum) y parseo correcto.
  - **Criterio de aceptación:** El script Python muestra los valores de temperatura parseados correctamente y sin errores de integridad.
- [X] **Tarea 19: Comunicación de Métricas**

  - **Descripción:** Implementar el envío de métricas del sistema (batería, estado, etc.) a través del driver de comunicación utilizando el protocolo de tramas definido.
  - **Criterio de aceptación:** Las métricas se transmiten correctamente por serie con el formato de trama correspondiente.
- [X] **Tarea 20: Validación de Comunicación de Métricas**

  - **Descripción:** Verificar que las métricas recibidas en el script de Python coincidan con los valores reales del sistema. Validar integridad de trama y parseo correcto.
  - **Criterio de aceptación:** El script Python muestra las métricas parseadas correctamente y sin errores de integridad.
- [X] **Tarea 21: Comunicación de Keep Alive**

  - **Descripción:** Implementar el envío periódico de un mensaje de keep alive a través del driver de comunicación, para que el receptor confirme que la pulsera sigue activa y conectada.
  - **Criterio de aceptación:** El mensaje de keep alive se transmite periódicamente por serie con el formato de trama correspondiente.
- [X] **Tarea 22: Validación de Comunicación de Keep Alive**

  - **Descripción:** Verificar que los mensajes de keep alive llegan al script de Python con la periodicidad esperada. Validar integridad de trama y detección de timeout ante desconexión.
  - **Criterio de aceptación:** El script Python detecta los keep alive periódicos y reporta correctamente si se pierde la comunicación.
- [X] **Tarea 23: Comunicación de Caída**

  - **Descripción:** Implementar el envío del evento de caída (con su snapshot de datos del buffer) a través del driver de comunicación utilizando el protocolo de tramas definido.
  - **Criterio de aceptación:** El evento de caída se transmite correctamente por serie con el formato de trama correspondiente, incluyendo el payload de datos del sensor.
- [X] **Tarea 24: Validación de Comunicación de Caída**

  - **Descripción:** Verificar que el evento de caída recibido en el script de Python contenga los datos esperados del snapshot. Validar integridad de trama, parseo correcto y que no haya pérdida de datos.
  - **Criterio de aceptación:** El script Python muestra el evento de caída con todos los datos del snapshot parseados correctamente y sin errores de integridad.
- [X] **Tarea 25 (Backlog 4): Mecanismos de comunicación entre tareas**

  - **Tiempo estimado:** 3 horas
  - **Descripción:** Definir e implementar los mecanismos (colas, semáforos, event groups) basados en la arquitectura de la tarea 1.
  - **Criterio de aceptación:** Mecanismos del RTOS creados, inicializados y listos para ser utilizados por las tareas.
- [ ] **Tarea 26: Mecanismos de protección en drivers (escritura y lectura de buffers)**

  - **Descripción:** Implementar protección de concurrencia (mutex, critical sections) en las operaciones de escritura y lectura de los buffers circulares de los drivers de sensores (IMU, presión, temperatura). Garantizar que no existan race conditions entre el thread productor (ISR/recolector) y el consumidor (processing task).
  - **Criterio de aceptación:** Los buffers de los drivers soportan acceso concurrente sin corrupción de datos. Se puede demostrar que un hilo escribe y otro lee simultáneamente sin errores.
- [ ] **Tarea 27: Unificación de términos de inicialización y Singletons**

  - **Descripción:** Estandarizar las firmas de inicialización en todos los drivers (ej. unificar el uso de `init()`). Además, unificar la nomenclatura y los métodos de los patrones Singleton en todo el proyecto para que todos usen la misma convención (ej. decidir entre `createInstance()` o `getInstance()`, o cómo se relacionan entre sí).
  - **Criterio de aceptación:** Todos los drivers y tareas presentan la misma convención de nombrado para su ciclo de vida, inicialización y acceso a su instancia única.
- [ ] **Tarea 28: Refactor de InterfaceDriver y ButtonDriver**

  - **Descripción:** El driver de pulsador y manejo de interrupciones actual no es claro (`InterfaceDriver`). Se debe instanciar la ISR fuera de la clase (en `app.cpp` junto a las demás) y hacer que la Tarea (`SystemTask` o `AlarmsEventsTask`) corra la MDE correspondiente.
  - **Criterio de aceptación:** La clase `InterfaceDriver` queda libre de acoplamientos de semáforos e ISRs ocultas, cediendo el control del polling/MDE a las tareas del RTOS.
## Sprints Previos (Tareas Completadas)

### Sprint 1: Inicialización

- [X] **Tarea 1: Hello World y Verificación de Entorno**
  - Configurar un "Hello World" básico en el `main` del proyecto actual.
  - Incluir todos los headers de los drivers actuales (`#include ...`) en el main.
  - Compilar para verificar si hay errores de vinculación/inclusión.
  - Cargar el firmware en la placa Nicla Sense ME.
  - Verificar que se imprime el mensaje "Hello World" a través del puerto serie.
- [X] **Tarea 2: Hello World del Sensor Bosch (AccelGyro)**
  - Implementar un "Hello World" específico para el sensor Bosch.
  - Portar/adaptar el ejemplo `AccelGyro` de los repositorios de Arduino hacia nuestro `main`.
  - Compilar y flashear a la placa.
  - Ejecutar el programa y verificar la lectura de datos (Acelerómetro y Giroscopio) en el monitor serie.
- [X] **Tarea 3: Revisión y Próximos Pasos**
  - Una vez finalizados los pasos 1 y 2, evaluar el estado actual.
  - Definir las siguientes funcionalidades a desarrollar e incorporarlas al Backlog.

### Sprint 2: Independencia de Arduino_BHY2

**Objetivo:** Dejar de depender de la librería `Arduino_BHY2` y manejar los periféricos con drivers propios y clases estáticas del sistema (ej. `nicla`).

- [X] **Tarea 4: Driver de Batería (Parte 1 - Implementación y Prueba base)**
  - Implementar las funciones del driver de batería utilizando los métodos estáticos de la clase `nicla`.
  - Crear un programa en `main.cpp` para leer y probar valores como: detección de batería, estado de carga y nivel de tensión.
  - *Nota:* El controlador requiere un ping constante por I2C. Para esta tarea, usaremos el thread interno que crea `nicla` automáticamente, permitiéndote medir y verificar físicamente la placa.
- [X] **Tarea 5: Driver de Batería (Parte 2 - Ping I2C Manual)**
  - Crear un método propio en nuestro driver de batería para poder enviar el ping I2C manualmente al controlador.
  - *(Más adelante integraremos esto en un thread de estado global donde realizaremos este y otros procesos)*.

### Sprint 3: IMU, Sensor Bosch y Tareas

A continuación, se listan las tareas planificadas para el desarrollo del driver del sensor BHI260AP, el manejo de eventos y notificaciones:

- [X] **Tarea 6:** Definir los métodos para la clase `bhi260_driver` y cómo es su estructura.
- [X] **Tarea 7:** Modificar `Sensortec` para que exponga `_bhy2` a sus clases derivadas/herencias.
- [X] **Tarea 8:** Configurar la interrupción del sensor en el microcontrolador. Agregar su handler y comprobar que sea llamado.
- [X] **Tarea 9:** Implementar las funciones de `update` para las FIFOs `wakeup` y `nonwakeup`.
- [X] **Tarea 10:** Configurar una serie de sensores virtuales y atenderlos por ambas FIFOs.
- [X] **Tarea 11:** Armar la clase `sensorClass` (`imu_sensor_driver`).
- [X] **Tarea 12:** Probar el largo máximo del buffer `nonwakeup`.
- [X] **Tarea 13:** Probar el sensor de *Significant Motion* y validar si se puede configurar su umbral. (Descartado: no configurable y diseñado para geolocalización AOSP, no para impactos).
- [X] **Tarea 14:** Probar el sensor de *Step Detector*.
- [X] **Tarea 15:** Probar el sensor de *Stationary Detect* y validar si se puede configurar su umbral o tiempos.
- [X] **Agregado:** Implementación Driver de Presión: Obtención de datos del barómetro del sensor BHY, usando herencia de `SensorClass` y FIFO de software propia.
- [X] **Agregado:** Implementación Driver de Temperatura: Implementación de la captura de datos térmicos. (Bug de parseo nativo resuelto).
- [X] **Agregado:** Implementación Driver de Eventos: Generic event listener driver y estructura `EventSensorDriver`.
- [X] **Agregado:** Buffers Parametrizados: Macros dinámicas (`FREQ * SEGUNDOS_RETENCION`) integradas en los drivers.
- [X] **Agregado:** HITO ALCANZADO (Pipeline Teórico Completo): Diseño arquitectónico de 4 etapas (ISR, System, Processing, Comm Task) con solución matemática de "Delayed Sliding Window" y solapamiento del 50%.

---

## 3. Backlog (Próximos Sprints)

### 5. Tarea de comunicación y link (Parte 1)

* **Tiempo estimado:** 4 horas
* **Descripción:** Implementar el envío de datos de los sensores en la tarea de comunicación.
* **Criterio de aceptación:** Activar mediante puerto serie los semáforos/queues de envío, y enviar los datos usando el bypass del driver de comunicación.

### 6. Tarea de alarma y eventos

* **Tiempo estimado:** 5 horas
* **Descripción:** Atender eventos de caída y botón de pánico. Controlar transiciones de estado según el IMU o el botón.
* **Criterio de aceptación:** Recibir por puerto serie un print de los eventos generados, verificando que el RTOS permite ejecutar esto con una tarea de menor prioridad corriendo simultáneamente (sin bloqueos).

### 7. HITO: HW de la pulsera listo para usar

* **Tiempo estimado:** Hito (0 horas de soft)
* **Descripción:** Carcasa y PCB ensamblados y listos en la muñeca para recolección de datos.

### 8. Tener muestras de caídas con pulsera

* **Tiempo estimado:** 3 horas
* **Descripción:** Generar caídas verdaderas y eventos cotidianos capturando los buffers.
* **Criterio de aceptación:** Dataset preliminar capturado mediante el script de Python.

### 9. Generar el pipeline para guardar datos en microcontrolador

* **9A. Wake-up y recolección (4 horas):** Implementar la lógica para despertar al micro frente al evento, acceder al buffer y extraer los datos.
* **9B. Procesamiento y detección (4 horas):** Procesar un buffer (puede ser hardcodeado de una caída real) y validar que la lógica dispare la detección de la caída.
* **Criterio de aceptación:** El micro despierta, procesa datos y levanta el evento de caída exitosamente.

### 10. Detección de dispositivo puesto o no

* **Tiempo estimado:** 4 horas
* **Descripción:** Comprobar si la pulsera está puesta utilizando los algoritmos y features ya integrados internamente en la IMU.
* **Criterio de aceptación:** El sistema detecta exitosamente el cambio de estado (puesta / sacada) al hacer la prueba física.

### 11. Tarea de notificaciones y telemetría

* **Tiempo estimado:** 5 horas
* **Descripción:** Implementar la tarea y sus estados a través de los drivers de hardware correspondientes (buzzer, led, etc.).
* **Criterio de aceptación:** Tarea CLI temporal que reciba comandos por serie y accione los semáforos para forzar y validar los estados de notificación.

### 12. Detección de intención de re-emparejamiento

* **Tiempo estimado:** 2 horas
* **Descripción:** Implementar el método de llamado (ej. mantener botón presionado) para detectar la intención de re-emparejamiento y generar el reseteo del sistema. Nota: Se deja el hook/espacio; el manejo del stack BLE lo implementará otra persona.
* **Criterio de aceptación:** La acción física accede correctamente a la función base y resetea el sistema.

---

## 4. Épicas del Sistema (Mediano y Largo Plazo)

Estas tareas representan los grandes bloques de trabajo (Epics) que deberán ser refinados y desglosados en tareas más pequeñas y estimables a medida que el proyecto avance.

### Épica 1: Interacción App Cuidador - Servidor

* **Alcance:** App Móvil / Servidor.
* **Objetivo:** Conectar la interfaz del cuidador con el backend.
* **Notas para futuro desglose:**
  Requerirá dividirse en endpoints específicos: Autenticación/Login, Recepción de Alertas en tiempo real (FCM/WebSockets) y Consulta de Estado (batería, conexión de la pulsera).

### Épica 2: Seguridad y Gestión de Secretos en la App

* **Alcance:** App Móvil / DevOps.
* **Objetivo:** Evitar la exposición de datos sensibles (API Keys, configuraciones de base de datos) en el repositorio Git.
* **Notas para futuro desglose:**
  Implementar variables de entorno (.env).
  Configurar .gitignore.
  Revocar y rotar cualquier clave que ya haya sido pusheada por error en el pasado.

### Épica 3: Validación del Pipeline de Inferencia

* **Alcance:** Servidor / Machine Learning / Firmware.
* **Objetivo:** Asegurar que los datos crudos emitidos por la pulsera en tiempo real mantengan la misma calidad, escala y forma que los datos teóricos usados para entrenar el modelo.
* **Notas para futuro desglose:**
  Armar un script que inyecte un stream de la pulsera (vía serie) hacia la inferencia.
  Comparar estadísticamente (escala, jitter, frecuencia de muestreo) contra el dataset ideal de entrenamiento.

### Épica 4: Expansión del Dataset de Entrenamiento

* **Alcance:** Data Science / Recolección en campo.
* **Objetivo:** Obtener un set de eventos más exhaustivo para robustecer el modelo.
* **Notas para futuro desglose:**
  Para evitar tareas infinitas, se deberá definir una cuota dura. Ej: "50 caídas simuladas por 3 usuarios distintos" y "100 eventos de Actividades de la Vida Diaria (ADL)".

### Épica 5: Alineación Espacial del Acelerómetro (Servidor)

* **Alcance:** Servidor / Procesamiento de señales.
* **Objetivo:** Modificar la orientación de los datos del sensor físico para que sus ejes coincidan con el marco de referencia esperado por el modelo.
* **Notas para futuro desglose:**
  Implementar matriz de rotación matemática.
  Aplicar TDD (Test Driven Development): crear tests unitarios con vectores de entrada/salida conocidos antes de integrarlo al pipeline.

### Épica 6: Integración del Stack BLE

* **Alcance:** Firmware / App.
* **Objetivo:** Implementar la comunicación Bluetooth Low Energy completa entre la pulsera y el teléfono/gateway.
* **Notas para futuro desglose:**
  Deberá dividirse en:
  Advertising y control de conexión (GAP).
  Creación de Servicios y Características (GATT).
  Lógica de Seguridad y Emparejamiento (Bonding - relacionado a la Tarea 12 de Firmware).
  Transmisión de payloads de sensores (Notifications).

---

## 5. Recordatorios y Notas

- **Batería:** Modificar la carga máxima a 4.13V. *(Prueba sugerida: dejarla cargando más tiempo para validar si efectivamente la tensión sube hasta ese máximo).*
- **Procesamiento de Buffer IMU:**
  - **Contexto:** Los datos se escriben en la clase (posiblemente por un thread recolector) y otro thread se encargará de procesarlos.
  - **Riesgo:** Si el buffer no se copia o protege adecuadamente, habrá una *condición de carrera* (Race Condition) entre ambos threads.
  - **Opciones a evaluar para el procesamiento:**
    1. Procesar los datos de forma interna directamente dentro de la clase.
    2. Exponer un `struct const` (como un buffer circular de solo lectura) hacia afuera para procesarlos externamente de forma segura.
    3. Implementar un esquema de **Productor/Consumidor** (Producer/Consumer) utilizando herramientas del RTOS (Mailbox, Queue o EventFlags) para gestionar la concurrencia.
- **Implementación del Pipeline RTOS (Para Tareas 4 y 9):**
  - Recordar aplicar Mutex/Critical Section en los buffers.
  - Utilizar ventana deslizante (`getElementAt`) con solapamiento del 50%.
  - Implementar flag de OVERRUN para escaneo profundo en caso de retraso.
  - Evaluar encapsular el procesamiento matemático del algoritmo de caída directamente dentro de `ImuSensorDriver`.
- **Arquitectura de Envío de Datos (Snapshot Buffer):**
  - Para evitar perder datos por la latencia de transmisión (BLE/Serial), **NO** se bloqueará el buffer circular de los sensores.
  - La Capa de Tareas (Capa de Aplicación / Processing Task) debe ser dueña de una estructura de memoria estática (`FallSnapshot` ~12KB) para congelar la "foto" del evento.
  - Al detectar la caída, la Tarea copia con `memcpy` ambos sensores a esta estructura en < 1ms y manda los punteros a la Queue.
  - El driver `CommDriver` es 100% agnóstico y **no posee buffers internos de TX** para almacenar el evento; solo recibe el puntero al Snapshot provisto por la Tarea y lo transmite.
