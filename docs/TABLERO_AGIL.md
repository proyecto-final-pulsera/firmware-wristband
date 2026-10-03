# Sprint Backlog - Firmware Nicla Sense ME

Este archivo servirá como nuestra memoria y hoja de ruta compartida (sprint backlog) para desarrollar el firmware correctamente. Iremos marcando las tareas a medida que las completemos.

## Sprint Actual

- [ ] **Detección de dispositivo puesto (Off-Wrist Detection)**

  * **Tiempo estimado:** 4 horas
  * **Descripción:** Comprobar si la pulsera está puesta utilizando los algoritmos y features ya integrados internamente en la IMU.
  * **Criterio de aceptación:** El sistema detecta exitosamente el cambio de estado (puesta / sacada) al hacer la prueba física.

  - **Subtareas:**
    - [ ] Fase de Análisis: Investigar y definir la estrategia técnica para realizar la detección.
    - [ ] Fase de Arquitectura: Definir qué tareas estarán involucradas (MDE en `SystemTask`, interacción con procesamiento).
    - [ ] Fase de Desarrollo: Implementar la solución en el código.
- [ ] **Análisis de Métricas Extras (Actividad y Sueño)**

  - **Descripción:** Estudiar la viabilidad y el mecanismo óptimo para calcular métricas avanzadas de usuario.
  - **Subtareas:**
    - [ ] Análisis de viabilidad: Sedentarismo.
    - [ ] Análisis de viabilidad: Tiempo en movimiento.
    - [ ] Análisis de viabilidad: Tiempo acostado.
    - [ ] Análisis de viabilidad: Levantamiento durante la noche.
    - [ ] Definición de arquitectura: Decidir qué se procesará en el Firmware y qué se delegará al Servidor/App.

### Sprint 6 (HW Pulsera y Notificaciones)

- [X] **HITO: HW de la pulsera listo para usar**

  - **Descripción:** Carcasa y PCB ensamblados y listos en la muñeca para recolección de datos.
  - **Subtareas HW:**
    - [X] Comprobar alimentación de vibrador con cambio.
    - [X] Re mapear pin PWM.
    - [X] Re mapear pulsador notificación.
    - [X] Probar funcionamiento vibrador aislado.
    - [X] Probar funcionamiento vibrador con el resto del RTOS.
    - [X] Documentar modificaciones en HW para segunda placa.
    - [X] Realizar gestos hápticos de prueba.
- [X] **Tarea de Notificaciones y Telemetría**

  - **Descripción:** Implementar la tarea y sus estados a través de los drivers de hardware correspondientes (LED, vibrador, telemetría).
  - **Subtareas:**
    - [X] Definir qué interacciones presenta al sistema (LED, batería, etc.). Implementado en `NotifyTelemetryTask` y `PmicTask`.
    - [X] Generar task de keep alive (fusionada en `NotifyTelemetryTask`).
    - [X] Generar mensaje de batería y actividad en el driver de comm (`CMD_TX_BATTERY_DATA`).
    - [X] Evaluar implementar algoritmo de pulsera no puesta en este módulo. *(Nota: Lo vamos a dejar para implementar desde System donde habra una MDE y en conjunto con la tarea de procesamiento para determinar si la pulsera fue quitada o no).*
    - [X] Probar que lo implementado funcione. (Se realizó prueba unitaria al inicio del sistema validando LEDs, métricas y timers).
    - [X] Integrar el uso del vibrador (`CMD_VIBRATOR_GESTURE`) y probar gestos.
    - [X] Probar el driver de batería con la batería física conectada (actualmente reporta 0 porque no hay batería conectada).
    - [X] **TODO/Evaluar**: Revisar si el dato de la batería lo mandamos por payload o que `CommLinkTask` acceda al driver directamente.
    - [X] **TODO/Evaluar**: Evaluar si conviene quitar el envío cíclico interno de la tarea y hacerlo a través del `SystemTask` con un timer con timeout.
  - **Criterio de aceptación:** Tarea CLI temporal que reciba comandos por serie y accione los semáforos/comandos para forzar y validar los estados de notificación.

## Sprints Previos (Tareas Completadas)

### Sprint 5 (Refactor, Caídas y SystemTask)

- [X] **Tarea 28: Refactor de InterfaceDriver y ButtonDriver**
- [X] **Tarea 6: Tarea de Procesamiento de Caídas (AlarmsEventsTask)**
- [X] **Tarea 7: Tarea Coordinadora del Sistema (SystemTask)**

### Sprint 4 y Tareas Completadas Recientes

- [X] **Tarea 5: Tarea de comunicación y link (Parte 1)**
  - **Descripción:** Implementar el envío de datos de los sensores en la tarea de comunicación.
- [X] **Tarea 16 (Original 3): Driver de comunicación (Bypass Serie + Protocolo)**
- [X] **Tarea 17: Comunicación de Temperatura**
- [X] **Tarea 18: Validación de Comunicación de Temperatura**
- [X] **Tarea 19: Comunicación de Métricas**
- [X] **Tarea 20: Validación de Comunicación de Métricas**
- [X] **Tarea 21: Comunicación de Keep Alive**
- [X] **Tarea 22: Validación de Comunicación de Keep Alive**
- [X] **Tarea 23: Comunicación de Caída**
- [X] **Tarea 24: Validación de Comunicación de Caída**
- [X] **Tarea 25 (Backlog 4): Mecanismos de comunicación entre tareas**
- [X] **Tarea 26: Mecanismos de protección en drivers (escritura y lectura de buffers)**
- [X] **Tarea 27: Unificación de términos de inicialización y Singletons**

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

### 8. Tener muestras de caídas con pulsera

* **Tiempo estimado:** 3 horas
* **Descripción:** Generar caídas verdaderas y eventos cotidianos capturando los buffers.
* **Criterio de aceptación:** Dataset preliminar capturado mediante el script de Python.

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

### Épica 7: Definición y Gestión del Emparejamiento

* **Alcance:** Firmware / App / Seguridad.
* **Objetivo:** Establecer el método completo de emparejamiento, reconfiguración y asociación de la pulsera.
* **Notas para futuro desglose:**
  - Analizar si se requiere almacenar credenciales (claves de encriptación, tokens) de forma persistente.
  - Definir métodos de software/hardware que se deban crear (ej: combinación de botones, timeouts).
  - Diseñar el flujo de reconfiguración del dispositivo (Factory Reset) para desvincularlo y asociarlo con un nuevo usuario.

## 5. Recordatorios y Notas

- **Batería:** Modificar la carga máxima a 4.13V. *(Prueba sugerida: dejarla cargando más tiempo para validar si efectivamente la tensión sube hasta ese máximo).*
