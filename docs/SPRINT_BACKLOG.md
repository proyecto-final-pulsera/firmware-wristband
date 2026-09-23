# Sprint Backlog - Firmware Nicla Sense ME

Este archivo servirá como nuestra memoria y hoja de ruta compartida (sprint backlog) para desarrollar el firmware correctamente. Iremos marcando las tareas a medida que las completemos.

## Sprint Actual

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

## Sprint 2: Independencia de Arduino_BHY2

**Objetivo:** Dejar de depender de la librería `Arduino_BHY2` y manejar los periféricos con drivers propios y clases estáticas del sistema (ej. `nicla`).

- [X] **Tarea 4: Driver de Batería (Parte 1 - Implementación y Prueba base)**

  - Implementar las funciones del driver de batería utilizando los métodos estáticos de la clase `nicla`.
  - Crear un programa en `main.cpp` para leer y probar valores como: detección de batería, estado de carga y nivel de tensión.
  - *Nota:* El controlador requiere un ping constante por I2C. Para esta tarea, usaremos el thread interno que crea `nicla` automáticamente, permitiéndote medir y verificar físicamente la placa.
- [X] **Tarea 5: Driver de Batería (Parte 2 - Ping I2C Manual)**

  - Crear un método propio en nuestro driver de batería para poder enviar el ping I2C manualmente al controlador.
  - *(Más adelante integraremos esto en un thread de estado global donde realizaremos este y otros procesos)*.

## Sprint 3: IMU, Sensor Bosch y Tareas

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
- [ ] **Tarea 16:** Generar metodo para procesar buffer en datos de imu.
- [ ] **Tarea 17:** Desarrollar el pre-evento y post-evento para el buffer del acelerómetro.
- [ ] **Tarea 18:** Armar la tarea `task_alarms_events`.
- [ ] **Tarea 19:** Armar la tarea `task_tel_notif`.
- [ ] **Tarea 20:** Revisar sensor de Temperatura. Se comprobó que interrumpe correctamente pero no arroja datos (Payload vacío o falla en el parseo).

## Backlog Próximos Sprints

*(Vacío por ahora)*

## Recordatorios y Notas

- **Batería:** Modificar la carga máxima a 4.13V. *(Prueba sugerida: dejarla cargando más tiempo para validar si efectivamente la tensión sube hasta ese máximo).*
- **Procesamiento de Buffer IMU (Para Tarea 16):**
  - **Contexto:** Los datos se escriben en la clase (posiblemente por un thread recolector) y otro thread se encargará de procesarlos.
  - **Riesgo:** Si el buffer no se copia o protege adecuadamente, habrá una *condición de carrera* (Race Condition) entre ambos threads.
  - **Opciones a evaluar para el procesamiento:**
    1. Procesar los datos de forma interna directamente dentro de la clase.
    2. Exponer un `struct const` (como un buffer circular de solo lectura) hacia afuera para procesarlos externamente de forma segura.
    3. Implementar un esquema de **Productor/Consumidor** (Producer/Consumer) utilizando herramientas del RTOS (Mailbox, Queue o EventFlags) para gestionar la concurrencia.
