# Documentación del Proyecto: Firmware Pulsera

Este documento sirve como bitácora de las decisiones de arquitectura más críticas, descubrimientos sobre el hardware y notas de implementación que surgieron durante el desarrollo del firmware.

## 1. Decisiones de Arquitectura

### 1.1 Patrón Wrapper para el IMU (BHI260AP)
Se decidió implementar una clase `BHI260Driver` utilizando el patrón Singleton y actuando como un **Wrapper** sobre la instancia global `sensortec` provista por la librería `Arduino_BHY2`.
* **Motivo:** Esto aísla la lógica de aplicación de las peculiaridades de la librería de Bosch, permitiendo unificar el manejo de interrupciones, la configuración de bajo consumo (Low Power) y el vaciado de las FIFOs en un solo lugar limpio. Se utilizó `friend class BHI260Driver;` dentro de `BoschSensortec.h` para poder acceder al struct nativo `_bhy2` de la API en C y extender sus funciones (como el modo Suspend).

## 2. Notas de Implementación

### 2.1 El problema del Pin 14 (InterruptIn) y el Flanco de Subida
* **Síntoma:** Al configurar el pin de interrupción (D14) en Arduino con `attachInterrupt(..., RISING)`, el handler nunca se disparaba, aunque físicamente el pin estaba en ALTO (3.3V).
* **Descubrimiento:** El BHI260AP genera "Meta Eventos" (ej. "Initialized") apenas arranca y los carga en su FIFO, levantando el pin de interrupción. Como la interrupción nativa en Mbed/Arduino se configura por **flanco de subida (Edge Triggered - RISING)**, y el pin ya estaba en ALTO antes de ejecutar `attachInterrupt`, el microcontrolador perdía el flanco y se quedaba bloqueado para siempre.
* **Solución Crítica:** Se implementó el método `flushFIFOs()` en el driver. Al final del `setup()`, justo después de atar la interrupción, vaciamos por completo cualquier paquete inicial en el BHI260AP. Esto obliga al sensor a bajar físicamente la línea D14 a `LOW`, garantizando que el próximo dato genere un flanco de subida limpio y active la interrupción de Arduino sin problemas.

### 2.2 Host Interface Control y Bajo Consumo (Modo Suspend)
Para dormir el MCU sin perder eventos críticos, se implementaron los métodos `suspendHost()` y `resumeHost()`. Estos escriben en el registro `BHY2_HIF_CTRL_AP_SUSPENDED`.
* **Mecanismo:** Al poner AP Suspended en 1, el BHI260AP enmascara los datos regulares (Non-Wakeup FIFO) para que **no disparen el pin de interrupción**. El sensor acumula los datos físicamente, pero solo levantará la línea de interrupción (despertando al micro) si ocurre un evento crítico configurado como "Wakeup" (ej. Gestos, Doble Toque).

## 3. Notas de Sensores y Machine Learning (BSX)

### 3.1 Firmware y Sensores Virtuales
El BHI260AP (FUSER2) no expone parámetros configurables simples (como umbrales de sensibilidad) para sus "Sensores Virtuales" (Gestos). Estos algoritmos son cajas negras de Machine Learning (Bosch Sensor Fusion - BSX) empaquetados en un parche de memoria RAM.
* **Consulta de Disponibilidad:** No todos los parches traen todos los algoritmos. Se implementó el método `hasSensor(ID)` que interroga directamente al firmware del chip en el arranque para verificar si un algoritmo específico está cargado.

### 3.2 Comportamiento de Gestos (El estándar AOSP)
Bosch diseña las cinemáticas de los gestos virtuales para cumplir estrictamente con el **Android Sensor HAL (Android Open Source Project - AOSP)**. Entender las definiciones de Google es la única forma de saber qué espera la inteligencia artificial de Bosch.
* **Fuente Oficial de Android:** [Android Sensor Types (AOSP)](https://source.android.com/devices/sensors/sensor-types?hl=es-419#wake_up_gesture) - *Documentación clave para entender el comportamiento y cinemática de cada gesto.*
* **Wrist Tilt Gesture (ID 67):** (Recomendado para pulseras). Reacciona a un giro fluido de muñeca desde un estado de reposo, imitando el gesto clásico para mirar un Smartwatch.
* **Pickup Gesture (ID 61):** Optimizado para Smartphones. Exige que el dispositivo se levante desde una mesa plana y se rote hacia la cara del usuario. Si se levanta de forma paralela al suelo, el evento se ignora.

### 3.3 El "One-Shot" y el Sensor Estacionario (Stationary Detect - ID 75)
Los sensores de evento especiales reportan mediante un mecanismo llamado "One-Shot" o de un solo disparo.
* **Comportamiento Estacionario:** Según la especificación AOSP, el dispositivo debe estar con aceleración constante (gravedad 1G) y giro 0 rad/s durante **exactamente 5 segundos** ininterrumpidos. Cualquier micro vibración en la mesa resetea el contador a cero.
* **Re-armado (El gran secreto):** Cuando pasan los 5 segundos, el sensor escupe 1 único paquete (sin payload) avisando el evento y **se apaga lógicamente**. Para que vuelva a dispararse, el usuario debe mover la placa (para sacarla del estado estacionario) y la placa debe volver a quedar quieta. En el código (`test_virtual_sensors.cpp`), se descubrió que es necesario volver a invocar `configureSensor()` cada vez que se dispara un sensor One-Shot para re-habilitarlo (auto-rearmado).

## 4. Notas del Sistema Operativo (Mbed OS)

### 4.1 Memoria RAM y Stack Overflow en Hilos
Durante el desarrollo de buffers grandes (ej. FIFO interna de 500 datos), se detectaron crasheos abruptos del microcontrolador.
* **Descubrimiento:** Al instanciar de forma local (dentro de una función) objetos con buffers grandes, el compilador los coloca en la memoria Stack (Pila) del hilo en ejecución. En el framework de Arduino sobre Mbed OS, el hilo principal (`main thread`) tiene un límite de Stack precompilado muy estricto (generalmente 4KB a 8KB). Exceder este tamaño genera un "Stack Overflow" fatal.
* **Solución:** Los arrays y buffers pesados nunca deben declararse como variables locales estándar. Para evitar corromper la pila del hilo de Mbed, las clases con gran footprint de memoria deben instanciarse globalmente, dinámicamente con `new` (en el Heap), o declarándolas `static` de forma local (enviándolas al segmento BSS/Data).

---

## 5. Limitaciones de Acceso a Datos y la FIFO

### 5.1 El motor de parseo privado (`static`) de Bosch
Durante el desarrollo se intentó crear dos métodos en `BHI260Driver` (`updateWakeupFifo` y `updateNonWakeupFifo`) para que el usuario pudiera vaciar y procesar de forma selectiva solo la cola de interrupciones Wakeup o solo la cola de datos en background (Non-Wakeup).
* **El Problema:** La función en C de la API de Bosch que convierte los bytes crudos (SPI/I2C) en objetos comprensibles se llama `parse_fifo()`. Los ingenieros de Bosch declararon esta función (y varias de sus dependencias) como **`static`** dentro del archivo `bhy2.c`. Esto significa que el compilador aísla la función y la hace 100% privada e inaccesible desde cualquier código externo (como nuestro driver C++).
* **Solución Implementada:** Se desestimó la idea de editar el código fuente de la librería del fabricante para evitar problemas de compatibilidad y la necesidad de mantener un "fork" manual. En su lugar, se mantiene el uso del método nativo `updateFifoData()`, el cual utiliza por debajo `bhy2_get_and_process_fifo()` para drenar y parsear todo en un solo barrido masivo directo del hardware. Si la aplicación requiere filtrar (descartar) un tipo de dato mientras extrae otro, se acordó realizar ese descarte por software (mediante flags booleanos en los métodos `pop()` y `push()` de la clase interna del sensor).

### 5.2 Efectos colaterales del Host Suspend y Tiempos de Refresco
Durante el desarrollo del sistema de lectura por lotes (Batching con AP Suspend), se descubrió que el registro `BHY2_HIF_CTRL_AP_SUSPENDED` altera el comportamiento interno de la función `bhy2_get_and_process_fifo()` de la API de Bosch.
* **Comportamiento Específico:** 
  * Si el host está **suspendido** (`suspendHost()`, o `AP Suspend = 1`), invocar el parseo *únicamente* extrae y parsea los datos pertenecientes a la **Wakeup FIFO** (los eventos críticos). El microcontrolador ignora la **Non-Wakeup FIFO** por completo, protegiéndola.
  * Si el host está **despierto** (`resumeHost()`, o `AP Suspend = 0`), el parseo extrae **ambas FIFOs**, trayendo finalmente toda la data regular acumulada (ej. paquetes del acelerómetro de background).
* **Delay Mandatorio Post-Despertar:** Se descubrió que luego de mandar la señal de `resumeHost()`, el chip BHI260AP demora en reconfigurar sus canales internos. **Si se intenta vaciar la FIFO inmediatamente después del resume, la Non-Wakeup FIFO retorna vacía (cero datos).**
* **Solución Implementada:** Se incrustó un `delay(10)` mandatorio directamente dentro del método `BHI260Driver::resumeHost()` para garantizar que cuando el firmware vuelva al main thread y ejecute `updateFifoData()`, la matriz de Bosch ya tenga las colas de memoria disponibles para su vaciado masivo.

*Documento actualizado durante la fase de optimización de memoria e integraciones.*
