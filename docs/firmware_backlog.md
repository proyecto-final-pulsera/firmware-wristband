# Backlog de Firmware - Corto Plazo

**Total estimado: ~48 horas**

---

### 1. Definir cómo tratar los datos (Pipeline micro)
* **Tiempo estimado:** 3 horas
* **Descripción:** Teniendo los drivers de los sensores, diseñar el flujo de datos desde el *wake-up* hasta el envío al servidor (ahorro de energía y procesamiento).
* **Criterio de aceptación:** Proponer y evaluar lógicamente el flujo para un caso de caída verdadera, caída falsa y evento normal sin encontrar problemas de diseño.

### 2. Implementar driver de barómetro
* **Tiempo estimado:** 4 horas
* **Descripción:** Obtener datos del barómetro del sensor (BHY), usar herencia de `SensorClass`, guardar en FIFO y hacer *pop* de los datos.
* **Criterio de aceptación:** Los datos se encolan correctamente y solo se reciben cuando se activa la *non-wakeup* FIFO.

### 3. Driver de comunicación (Bypass Serie + Protocolo)
* **Tiempo estimado:** 7 horas
* **Descripción:** Definir métodos del driver y proveer un *bypass* para transmitir por puerto serie en lugar de BLE. Se debe implementar un protocolo de tramas simple (ej. `[START] [TIPO] [PAYLOAD] [CHECKSUM]`). Crear script en Python para recibir y parsear.
* **Criterio de aceptación:** Enviar datos del acelerómetro y barómetro por serie de forma transparente al sistema y poder verlos parseados en el script de Python.

### 4. Mecanismos de comunicación entre tareas
* **Tiempo estimado:** 3 horas
* **Descripción:** Definir e implementar los mecanismos (colas, semáforos, *event groups*) basados en la arquitectura de la tarea 1.
* **Criterio de aceptación:** Mecanismos del RTOS creados, inicializados y listos para ser utilizados por las tareas.

### 5. Tarea de comunicación y link (Parte 1)
* **Tiempo estimado:** 4 horas
* **Descripción:** Implementar el envío de datos de los sensores en la tarea de comunicación.
* **Criterio de aceptación:** Activar mediante puerto serie los semáforos/queues de envío, y enviar los datos usando el *bypass* del driver de comunicación.

### 6. Tarea de alarma y eventos
* **Tiempo estimado:** 5 horas
* **Descripción:** Atender eventos de caída y botón de pánico. Controlar transiciones de estado según el IMU o el botón.
* **Criterio de aceptación:** Recibir por puerto serie un *print* de los eventos generados, verificando que el RTOS permite ejecutar esto con una tarea de menor prioridad corriendo simultáneamente (sin bloqueos).

### 7. HITO: HW de la pulsera listo para usar
* **Tiempo estimado:** Hito (0 horas de soft)
* **Descripción:** Carcasa y PCB ensamblados y listos en la muñeca para recolección de datos.

### 8. Tener muestras de caídas con pulsera
* **Tiempo estimado:** 3 horas
* **Descripción:** Generar caídas verdaderas y eventos cotidianos capturando los buffers.
* **Criterio de aceptación:** Dataset preliminar capturado mediante el script de Python.

### 9. Generar el pipeline para guardar datos en microcontrolador
* **9A. Wake-up y recolección (4 horas):** Implementar la lógica para despertar al micro frente al evento, acceder al buffer y extraer los datos.
* **9B. Procesamiento y detección (4 horas):** Procesar un buffer (puede ser *hardcodeado* de una caída real) y validar que la lógica dispare la detección de la caída.
* **Criterio de aceptación:** El micro despierta, procesa datos y levanta el evento de caída exitosamente.

### 10. Detección de dispositivo puesto o no
* **Tiempo estimado:** 4 horas
* **Descripción:** Comprobar si la pulsera está puesta utilizando los algoritmos y *features* ya integrados internamente en la IMU.
* **Criterio de aceptación:** El sistema detecta exitosamente el cambio de estado (puesta / sacada) al hacer la prueba física.

### 11. Tarea de notificaciones y telemetría
* **Tiempo estimado:** 5 horas
* **Descripción:** Implementar la tarea y sus estados a través de los drivers de hardware correspondientes (buzzer, led, etc.).
* **Criterio de aceptación:** Tarea CLI temporal que reciba comandos por serie y accione los semáforos para forzar y validar los estados de notificación.

### 12. Detección de intención de re-emparejamiento
* **Tiempo estimado:** 2 horas
* **Descripción:** Implementar el método de llamado (ej. mantener botón presionado) para detectar la intención de re-emparejamiento y generar el reseteo del sistema. *Nota: Se deja el *hook*/espacio; el manejo del stack BLE lo implementará otra persona.*
* **Criterio de aceptación:** La acción física accede correctamente a la función base y resetea el sistema.
