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

Para dormir el MCU sin perder eventos críticos, se implementaron los métodos `disableNonWakeupFIFO()` y `enableNonWakeupFIFO()` (antes llamados suspend/resume). Estos escriben en el registro `BHY2_HIF_CTRL_AP_SUSPENDED`.

* **Mecanismo:** Al apagar el recolector de FIFO Non-Wakeup, el BHI260AP enmascara los datos regulares para que **no disparen el pin de interrupción**. El sensor acumula los datos físicamente, pero solo levantará la línea de interrupción (despertando al micro) si ocurre un evento crítico configurado como "Wakeup" (ej. Gestos, Doble Toque).

## 3. Notas de Sensores y Machine Learning (BSX)

### 3.1 Firmware y Sensores Virtuales

El BHI260AP (FUSER2) no expone parámetros configurables simples (como umbrales de sensibilidad) para sus "Sensores Virtuales" (Gestos). Estos algoritmos son cajas negras de Machine Learning (Bosch Sensor Fusion - BSX) empaquetados en un parche de memoria RAM.

* **Consulta de Disponibilidad:** No todos los parches traen todos los algoritmos. Se implementó el método `hasSensor(ID)` que interroga directamente al firmware del chip en el arranque para verificar si un algoritmo específico está cargado.

### 3.2 Comportamiento de Gestos (El estándar AOSP)

Bosch diseña las cinemáticas de los gestos virtuales para cumplir estrictamente con el **Android Sensor HAL (Android Open Source Project - AOSP)**. Entender las definiciones de Google es la única forma de saber qué espera la inteligencia artificial de Bosch.

* **Fuente Oficial de Android:** [Android Sensor Types (AOSP)](https://source.android.com/devices/sensors/sensor-types?hl=es-419#wake_up_gesture) - *Documentación clave para entender el comportamiento y cinemática de cada gesto.*
* **Wrist Tilt Gesture (ID 67):** (Recomendado para pulseras). Reacciona a un giro fluido de muñeca desde un estado de reposo, imitando el gesto clásico para mirar un Smartwatch.
* **Pickup Gesture (ID 61):** Optimizado para Smartphones. Exige que el dispositivo se levante desde una mesa plana y se rote hacia la cara del usuario. Si se levanta de forma paralela al suelo, el evento se ignora.
* **Significant Motion (ID 55):** A diferencia de lo que sugiere el nombre, **no detecta golpes fuertes ni impactos breves**. Según AOSP, está diseñado exclusivamente para activar la geolocalización cuando el usuario se desplaza físicamente (caminando, en auto, bicicleta). Cambios de estado cortos o impactos se descartan. No permite configurar umbrales y no es adecuado para detectar colisiones en la pulsera.

### 3.3 El "One-Shot" y el Sensor Estacionario (Stationary Detect - ID 75)

Los sensores de evento especiales reportan mediante un mecanismo llamado "One-Shot" o de un solo disparo.

* **Comportamiento Estacionario:** Según la especificación AOSP, el dispositivo debe estar con aceleración constante (gravedad 1G) y giro 0 rad/s durante **exactamente 5 segundos** ininterrumpidos. Cualquier micro vibración en la mesa resetea el contador a cero.
* **Re-armado (El gran secreto):** Cuando pasan los 5 segundos, el sensor escupe 1 único paquete (sin payload) avisando el evento y **se apaga lógicamente**. Para que vuelva a dispararse, el usuario debe mover la placa (para sacarla del estado estacionario) y la placa debe volver a quedar quieta. En el código (`test_virtual_sensors.cpp`), se descubrió que es necesario volver a invocar `configureSensor()` cada vez que se dispara un sensor One-Shot para re-habilitarlo (auto-rearmado).

### 3.4 Resultados de Validación de Sensores Virtuales

Durante las pruebas de validación con el test de eventos en buffer (`test_eventos_imu`), se confirmó la excelente precisión y funcionamiento de los siguientes algoritmos de Bosch:

* **Step Detector (Pasos):** Mostró una precisión exacta durante la caminata de prueba, registrando 10 eventos detectados al dar exactamente 10 pasos.
* **Wrist Tilt (Giro de Muñeca):** Detectó correctamente y sin falsos positivos el movimiento típico de levantar y girar el brazo para consultar un smartwatch.
* **Stationary Detect (Reposo Estacionario):** Al ser un sensor "One-Shot" (se desactiva tras dispararse) que requiere 5 segundos ininterrumpidos de quietud total, se verificó su correcto auto-rearmado por software. En una prueba extendida de 30 segundos, el sistema logró contabilizar exactamente 5 disparos, confirmando la latencia de 5 segundos de evaluación sumado al tiempo ínfimo de rearme dinámico.

## 4. Notas del Sistema Operativo (Mbed OS)

### 4.1 Memoria RAM y Stack Overflow en Hilos

Durante el desarrollo de buffers grandes (ej. FIFO interna de 500 datos), se detectaron crasheos abruptos del microcontrolador.

* **Descubrimiento:** Al instanciar de forma local (dentro de una función) objetos con buffers grandes, el compilador los coloca en la memoria Stack (Pila) del hilo en ejecución. En el framework de Arduino sobre Mbed OS, el hilo principal (`main thread`) tiene un límite de Stack precompilado muy estricto (generalmente 4KB a 8KB). Exceder este tamaño genera un "Stack Overflow" fatal.
* **Solución:** Los arrays y buffers pesados nunca deben declararse como variables locales estándar. Para evitar corromper la pila del hilo de Mbed, las clases con gran footprint de memoria deben instanciarse globalmente, dinámicamente con `new` (en el Heap), o declarándolas `static` de forma local (enviándolas al segmento BSS/Data).

---

## 5. Limitaciones de Acceso a Datos y la FIFO

### 5.1 Latencias Compartidas y el Sabotaje del Batching (Multi-Sensor)

Una de las confusiones más grandes de la arquitectura del BHI260 surge al combinar múltiples sensores en la misma cola (Non-Wakeup FIFO).

* **El Problema del IRQ compartido:** Si se configuran varios sensores con alta latencia (ej. 5 segundos) pero **uno solo de ellos** se configura con latencia `0` (ej. Temperatura a 1 Hz, latency=0), este sensor dominará el comportamiento del chip. En este ejemplo, el sensor de Temperatura forzará la generación de una interrupción física cada 1 segundo (ignorando las latencias largas de los otros sensores).
* **Bloqueo del Update (updateFifoData es Total):** Al atender la interrupción y llamar a `bhi->updateFifoData()`, **el BHI260 vacía TODO el contenido de la FIFO de hardware de forma bloqueante**, sin importar de qué sensor provenga. Como resultado, la IMU y la Presión escupirán los datos del último segundo en lugar de esperar a juntar los 5 segundos de latencia que tenían asignados originalmente.
* **Conclusión:** La lectura de datos en FIFO (Data Batching) deja de ser uniforme (se generan "micro-ráfagas" por culpa de otros sensores). Para testear o aprovechar la latencia real, **todos** los sensores concurrentes deben configurarse con latencia alta.

### 5.2 Capacidad Real de Extracción de Datos

A diferencia de otros drivers, el método base `bhy2_get_and_process_fifo` (usado internamente por `updateFifoData()`) **no tiene límite de extracción**.

* **Bucle Oculto:** Aunque se procesen "chunks" (bloques) usando un buffer de memoria RAM interno (ej. de 1024 bytes), el algoritmo itera en un bucle `while (fifos.remain_length)` hasta drenar absolutamente toda la memoria del sensor Bosch en esa misma llamada.
* **Precaución:** Como este método es totalmente bloqueante y ejecuta hasta vaciar todo el histórico acumulado, puede demorar bastantes milisegundos si la latencia del sensor era alta y se acumularon miles de bytes (por ejemplo, 5 segundos de IMU).

### 5.3 Estado Retenido de las Interrupciones (IRQ)

Las interrupciones (línea D14) del BHI260 están manejadas por nivel.

* **Desactivar IRQs por Software:** Si en el código se deshabilitan temporalmente las interrupciones (ej: `bhi->disableInterrupt()`) y durante ese tiempo el sensor levanta la línea física avisando que hay datos, el hardware se quedará "trabado" en estado ALTO.
* **Limpieza Obligatoria:** Si el sensor levantó el IRQ, no va a volver a generar un flanco (que es lo que Mbed necesita para disparar el callback) hasta que la FIFO sea leída. Es imperativo procesar y vaciar la FIFO explícitamente ("procesar para limpiar la IRQ") antes de re-habilitar la escucha, de lo contrario la línea nunca baja y se pierden los eventos futuros.

### 5.4 Capacidad real de la FIFO de Hardware (Depth Test)

Mediante el test `runFifoDepthTest()`, forzando el desborde a alta frecuencia (800 Hz) con el Host suspendido, se determinó de forma empírica la capacidad máxima real de almacenamiento del sensor.

* **Resultado:** El buffer interno (Non-Wakeup FIFO) del BHI260AP soporta un máximo de **2022 muestras** continuas de acelerómetro antes de empezar a sobrescribir o descartar datos viejos.
* **Cálculo de Memoria:** Siendo que cada paquete de acelerómetro ocupa 7 bytes (1 de cabecera/ID + 6 de payload X, Y, Z), se confirma que la capacidad de memoria física asignada a la FIFO dentro del hardware de Bosch ronda los **14,154 bytes** (aprox. 14 KB).

### 5.5 Parametrización de Buffers Circulares (Drivers)

Para evitar hardcodear números mágicos en el tamaño de los arreglos estáticos de los drivers C++ (como `imuDriver` o `pressureDriver`) y prevenir desbordamientos, se optó por parametrizar el tamaño de los buffers circulares en base al tiempo físico que se desea retener en memoria.

* **Fórmula Implementada:** `TAMAÑO_FIFO = (FRECUENCIA_HZ * SEGUNDOS_DE_RETENCION)`.
* **Beneficio:** Mediante macros globales (`FREQ_IMU`, `LEN_BUFFER_IMU_SEG`, etc.) definidas en los headers de cada driver, el compilador calcula automáticamente cuántos slots de memoria se necesitan. Por ejemplo, para retener **16 segundos** de datos de un acelerómetro corriendo a **50 Hz**, el buffer se dimensiona automáticamente en **800** posiciones.
* **Latencia vs Capacidad de Buffer:** Es importante destacar que el valor de latencia (latency) configurado en el sensor de hardware *no dicta ni debe ser igual* al tiempo de retención del buffer de software. El buffer de software existe para retener el historial y permitir que el hilo de procesamiento trabaje desacoplado, a su propio ritmo. Si el buffer es lo suficientemente grande (ej. 16 segundos), soporta que el hardware interrumpa múltiples veces (ej. latencias cortas) sin perder los datos viejos antes de que la tarea de fondo (pipeline) logre procesarlos.

---

### 5.6 Tiempos de Descarga del Buffer (I2C)

Se perfilaron los tiempos de transferencia de la FIFO de hardware (a trav�s de I2C) hacia los buffers de software del microcontrolador al ejecutar hi->updateFifoData().

* **Escenario de latencia est�ndar:** Para descargar los datos acumulados durante una ventana de **3 segundos**, la transacci�n y copia a trav�s del bus demora aproximadamente **10 ms**.
* **Escenario post Wake-Up (Buffer Completo):** Cuando el microcontrolador despierta luego de un tiempo prolongado y la FIFO de hardware est� cerca de su m�xima capacidad, el tiempo de lectura y descarga total asciende a aproximadamente **62 ms**.

## 6. Batería de Pruebas (Testing Suite)

Durante el desarrollo se crearon distintos entornos aislados de prueba (en la carpeta `/test/`) para validar funciones específicas del hardware y del software antes de integrarlas al `main`. A continuación se detalla qué hace y qué demuestra cada test:

### 6.1 `test_virtual_sensors.cpp`

* **Objetivo:** Explorar y validar el comportamiento lógico de todos los sensores virtuales (Gestos) disponibles en el BHI260.
* **Funcionamiento:** Se suscriben múltiples sensores de evento (como *Step Counter*, *Wrist Tilt*, *Device Orientation*, etc.). En el bucle principal se hace un barrido continuo y se loguea por puerto serie qué evento saltó y con qué ID. Adicionalmente, implementa una lógica de auto-suscripción cruzada (si detecta *Motion* prende el *Stationary*, y viceversa).
* **Qué prueba:** Demuestra cómo interactúan los eventos de "One-Shot" (que se apagan solos al dispararse) y cómo la inteligencia artificial de Bosch clasifica los movimientos reales frente a la especificación estándar (AOSP).

### 6.2 `test_eventos_imu.cpp`

* **Objetivo:** Poner a prueba la integración de los gestos mediante el modo "Host Suspend" utilizando nuestro `BHI260Driver`.
* **Funcionamiento:** Se configuran eventos específicos en la **Wakeup FIFO**. Luego se suspende el flujo de datos principal. Cuando el sensor detecta el gesto (ej. Giro de muñeca), despierta físicamente al microcontrolador a través del pin de interrupción.
* **Qué prueba:** Valida que el mecanismo de interrupción Wakeup funciona correctamente y que el microcontrolador puede ignorar el ruido (sensores de background) manteniéndose en reposo, consumiendo energía únicamente cuando el usuario realiza el evento esperado.

### 6.3 `test_imu_fifo.cpp`

* **Objetivo:** Testear al extremo la memoria interna de la IMU, la sobreescritura de los buffers, el "Watermark" y los límites físicos.
* **Funcionamiento:** Este test arranca la IMU a una frecuencia altísima (ej. 800 Hz) y duerme intencionalmente al microcontrolador sin leer la FIFO, forzando un cuello de botella. Luego, despierta al micro y extrae toda la memoria de golpe.
* **Qué prueba:** Sirvió empíricamente para descubrir que la RAM interna de la Non-Wakeup FIFO soporta hasta ~14 KB de datos. También probó el sistema de prevención de desbordes (Watermark Interrupt) del Bosch y verificó que nuestra clase C++ (Buffer Circular Wrapper) sabe lidiar correctamente con el desborde sobrescribiendo los datos más viejos mediante `isFull()` y `hasOverflowed()`.

### 6.4 `test_sensors_drivers.cpp` (El Test de Integración Multi-Sensor)

* **Objetivo:** Validar la coexistencia pacífica y orquestada de **todos** los sensores (IMU, Presión, Temperatura y Gestos) utilizando un único sistema de interrupciones.
* **Funcionamiento:** Instancia las clases Wrapper creadas (`ImuSensorDriver`, `PressureSensorDriver`, `TemperatureSensorDriver`, `EventSensorDriver`). Arranca inicialmente con la FIFO continua suspendida. Si un evento de *Motion* (Any Motion Wake-Up) despierta a la placa, automáticamente se habilita la FIFO Non-Wakeup, y las clases wrapper empiezan a acumular y vaciar los datos de la IMU y Barómetro. Si se detecta un evento *Stationary*, se vuelve a dormir el flujo.
* **Qué prueba:** Demuestra la arquitectura final que usará la pulsera. Prueba que una sola sub-rutina de interrupción (`isrFlag`) es capaz de vaciar el hardware de Bosch por completo de forma agnóstica, rellenando ordenadamente los buffers individuales de cada driver de aplicación y permitiendo encender/apagar el modo ráfaga según el estado físico del usuario para maximizar el ahorro de batería.

---

## 7. Protocolo de Comunicación y Robustez

### 7.1 Arquitectura del Frame (Trama de Datos)

Para comunicar la pulsera con el exterior (PC o dispositivos móviles) se diseñó un protocolo binario minimalista y rápido. La trama está compuesta por:

* `START_BYTE` (0xAA): Byte mágico de sincronización.
* `TYPE` (1 byte): Identificador del tipo de mensaje (IMU, Presión, Alarma, etc.).
* `LENGTH` (2 bytes, Little Endian): Longitud en bytes del payload.
* `HEADER_CRC` (1 byte): Suma de comprobación (CRC-8) exclusiva de los primeros 4 bytes.
* `PAYLOAD` (N bytes): Datos binarios en crudo.
* `PAYLOAD_CRC` (1 byte): Suma de comprobación (CRC-8) exclusiva del Payload (solo presente si `LENGTH` > 0).

### 7.2 Implementación de CRC-8 (Lookup Table)

Se optó por utilizar el algoritmo **CRC-8** (con polinomio 0x07) implementado mediante el método de **Lookup Table**.

* **Funcionamiento y Motivo:** En lugar de realizar la división polinómica bit a bit en tiempo real, se utilizan los 256 resultados posibles pre-calculados y guardados en una tabla en la memoria Flash. De esta forma, el cálculo "on the fly" se reduce a una simple operación XOR y una búsqueda en memoria por cada byte. Esto garantiza robustez extrema frente a ruido e interferencias conservando una altísima velocidad de despacho.

### 7.3 Separación Crítica: Header CRC vs Payload CRC

El protocolo implementa dos verificaciones de CRC independientes dentro del mismo mensaje, separando la cabecera (Header) de la carga útil (Payload).

* **Problema Original:** Si solo se validaba el mensaje al final y un error de ruido eléctrico afectaba a los bytes de "Longitud" (LENGTH), el receptor asumía un tamaño falso gigante (ej. 65000 bytes). Al esperar tantos bytes, el receptor perdía sincronización o se trababa leyendo basura (y falsos bytes de inicio) del buffer serial, causando un colapso en cascada de los paquetes subsecuentes.
* **Solución Aplicada:** Al validar primero el `HEADER_CRC` de forma aislada, el receptor puede saber de forma confiable e inmediata si la información de "Longitud" es legítima. Si el Header CRC falla, el receptor ignora el supuesto largo, **ejecuta una purga inmediata del buffer serie (flushing)**, y aborta la trama sin intentar leer la carga, protegiendo al sistema y permitiendo una re-sincronización instantánea con el próximo paquete válido.

---

## 8. RTOS y Arquitectura Multi-Tarea

### 8.1 Comunicación Inter-Task y Sincronización (Mailboxes y Mutexes)

Para coordinar el sistema de tiempo real (Mbed OS), se implementó un sistema de comunicación entre tareas basado en el paso de mensajes (Message Passing).

* **Wrapper Tipado:** Se creó un wrapper C++ tipo template (`MessageQueue`) sobre la clase nativa `rtos::Mail`. Esto permite aislar a las tareas de las implementaciones subyacentes del SO, automatizando además la inyección de timestamps (`Kernel::Clock::now()`) de forma transparente para evitar deprecaciones del Mbed OS 6.
* **Vocabulario Aislado:** Para prevenir que una tarea envíe un comando equivocado (Namespace Pollution), cada tarea define sus propios IDs de mensajes en un `enum` interno (`protected`).
* **Prueba de Concurrencia Exitosa:** Para validar el diseño del Sprint, se realizó una prueba inyectando un evento `CMD_TEST` masivo desde la tarea orquestadora (`SystemTask`) hacia las tareas de Comunicaciones, Alarma y Notificaciones de forma simultánea. Las tareas hijas recibieron el comando y ejecutaron una respuesta directa ("RESPUESTA DESDE...") al buzón del orquestador.
* **Control de Colisión (Mutex):** Como Mbed/Arduino OS no garantiza Thread-Safety nativo en las llamadas a `Serial.print()`, se comprobó empíricamente que la concurrencia generaba solapamiento de caracteres. Esto se mitigó validando el uso de `rtos::Mutex` durante la prueba de ping-pong, demostrando que el RTOS administra los bloqueos y prioridades de los hilos correctamente, abriendo la puerta al desarrollo seguro del resto de las lógicas.

---

## 9. Tarea de Procesamiento de Ca�das (AlarmsEventsTask)

### 9.1 Refactorizaci�n a Procesamiento Bajo Demanda (On-Demand)

Se redise�� el bucle principal (
un) de la tarea para procesar los datos �nicamente bajo demanda y no de forma continua:

- **CMD_PROCESS_IMU:** Al recibir este mensaje, la tarea extrae el segmento de datos actual del buffer y lo procesa en ese mismo instante. Luego vuelve al estado de bloqueo pasivo esperando el siguiente comando.
- La tarea permanece suspendida (timeout osWaitForever) la mayor parte del tiempo, lo cual asegura el ahorro de bater�a hasta que una interrupci�n o el SystemTask solicite un procesamiento.

### 9.2 Procesamiento Matem�tico Optimizado

- Se integr� la funci�n pprox_2d_improved y se implement� suma_pitagorica para calcular la magnitud del vector de aceleraci�n 3D en base a valores absolutos.
- Referencia del algoritmo: [Alpha max plus beta min algorithm](https://en.wikipedia.org/wiki/Alpha_max_plus_beta_min_algorithm)
- Esta aproximaci�n evita el uso de operaciones costosas (como la multiplicaci�n y la ra�z cuadrada) en el microcontrolador.
- **Rendimiento medido:** El tiempo de procesamiento para un buffer de 6 segundos sin reutilizar muestras previas (es decir, recalculando absolutamente todo el buffer) es de **2700 us** con la optimizaci�n de valor m�dulo. En contraposici�n, si se utiliza la expresi�n real (ra�z cuadrada de la sumatoria de los cuadrados), el tiempo asciende a **50000 us**. Finalmente, al precalcular el arreglo de m�dulos antes de recorrer la ventana deslizante (evitando calcular m�ltiples veces la misma muestra), el tiempo se reduce dr�sticamente a **800 us**.

### 9.3 Extracci�n de la "Ventana de Impacto"

- Se defini� que el buffer de la IMU (IMU_FIFO_SIZE = 800 muestras) estar�, en teor�a, siempre lleno durante la ejecuci�n continua.
- Se lee un segmento de 6 segundos (IMPACT_WINDOW_SIZE = 300 muestras) ubicado exactamente en el **medio** del hist�rico del buffer, extray�ndolo con un �ndice precalculado por el compilador: START_IMPACT_WINDOW_INDEX = (IMU_FIFO_SIZE / 2) - (IMPACT_WINDOW_SIZE / 2).

### 9.4 Detecci�n por "Ventana Deslizante" (Sliding Window)

- Se barre la ventana de impacto (300 muestras) utilizando una sub-ventana deslizante de tama�o 8 (SLIDING_WINDOW_SIZE = 8).
- Se utiliza un desplazamiento de bits (>> 3) en lugar de una divisi�n matem�tica para promediar la magnitud de la sub-ventana de forma extremadamente r�pida.
- Si el promedio baja del THRESHOLD_FREE_FALL, se levanta una bandera.
- Si supera el THRESHOLD_IMPACT, se levanta otra bandera.
- Si ambas banderas se detectan en la misma ventana de impacto de 6 segundos, se cumple la CONDICION DE CAIDA y se interrumpe la b�squeda.

### 9.5 Validación Inicial "Wake Up" y Procesamiento Pre-Fall

- Se implemento un comando especial CMD_PROCESS_IMU_WAKEUP (enviado por el SystemTask cuando detecta que el buffer se lleno por completo).
- **Justificacion de Diseño:** En la primer llamada luego de un *wake up*, es necesario procesar el *pre-fall* desde el inicio del buffer, por si el evento de caida sucedio inmediatamente una vez se desperto el equipo y no dio tiempo a loguear el *pre-fall* en el flujo habitual.
- Para ahorrar memoria RAM, se reutiliza el mismo buffer (_imu_window_buffer). Primero se procesan los 6 segundos iniciales, y si no hay alarma, se procesan los 6 segundos centrales, arrastrando las banderas de estado (all_flags) con una compuerta *OR* a traves de ambas etapas.
- Dependiendo de si se evaluo esta condicion de *Wake Up*, el mensaje final de alarma (a traves de CMD_TX_ALARM) adjuntar� el flag PRE_FALL_PROCESSED.
- **Rendimiento:** Al procesar dos ventanas secuenciales en el peor de los casos (cuando no se detecta alarma tempranamente), esta funcion especifica demora el doble de tiempo (aprox. **1700 us** frente a los 800 us regulares).
- **Limitacion:** Por la implementacion actual de reutilizacion del mismo buffer, SOLO soporta tiempos de pre_fall menores al tiempo de la ventana de procesamiento (menos 1 segundo de margen), es decir < WINDOW_PROCESAMIENTO - 1Seg.

## Decisiones Arquitectónicas: Tareas Cíclicas y Manejo de Interrupciones

### 1. Filosofía de Creación de Tareas (Overhead vs. Aislamiento)

Al decidir cómo manejar eventos cíclicos (ping al PMIC, Keep Alive, reporte de Métricas), se evaluó el balance entre el "overhead" de memoria y el aislamiento de fallos:

* **Aislamiento y Prioridad (Por qué crear PmicTask):** El ping al PMIC es una operación crítica a nivel de hardware. Si el PMIC no recibe un ping I2C en 50s, corta la energía de la placa. Se asignó esta responsabilidad a una tarea exclusiva (PmicTask) que simplemente hace sleep_for(40000) y envía el ping. Al aislarla, nos aseguramos de que ningún bug o retraso en otras tareas (ej. procesar sensores) provoque un reinicio de la pulsera.
* **Consumo de RAM (Por qué NO crear más tareas):** Cada tarea nueva en Mbed OS consume ~1KB de RAM para su Stack. Dado que el MCU nRF52832 cuenta con sólo 64KB de RAM (y que tecnologías pendientes como BLE o LittleFS requerirán gran parte de ella), crear una tarea dedicada para cada acción cíclica menor (ej. un task_keepalive, un task_bateria) no es escalable ni viable.
* **La Solución Híbrida:** Todo evento no crítico, de baja prioridad o ligado a la interfaz de usuario / telemetría se agrupó bajo NotifyTelemetryTask. Esta tarea no genera eventos por su cuenta; se limita a reaccionar a la cola de mensajes para leer estado (Batería, Repositorios) y enviarlo a CommLinkTask.

### 2. Patrón "Central Dispatcher" (Orquestación en SystemTask)

Para evitar que NotifyTelemetryTask o CommLinkTask calcularan sus propios timeouts dinámicamente o llevaran lógica de scheduling, implementamos un esquema donde SystemTask es el único orquestador de los eventos asincrónicos o basados en tiempo.

* **Implementación con mbed::Ticker:** Se instanciaron Timers de Hardware directamente dentro de la clase SystemTask (ej. _keepAliveTicker y _metricsTicker).
* **Flujo de Ejecución:**
  1. El temporizador cuenta asincrónicamente usando el hardware del microcontrolador (Cero uso de CPU de la tarea).
  2. Cuando expira, se dispara una interrupción de hardware (ISR), congelando temporalmente el RTOS.
  3. La ISR, cuyo método "callback" pertenece a SystemTask, envía un mensaje de ejecución *directamente a la cola de la tarea responsable* usando el envoltorio
     otifyTask. Por ejemplo, envía CMD_TX_KEEP_ALIVE a CommLinkTask.
  4. La ISR termina inmediatamente. El Scheduler de Mbed OS detecta el mensaje nuevo y despierta a la tarea correspondiente para que lo procese en "background" con prioridad baja.
* **Beneficios de este Modelo:**
  * **Stateless:** Tareas como NotifyTelemetryTask quedan libres de variables de tiempo. Su bucle
    un() es un simple, pasivo y eficiente _mailbox.receive(osWaitForever).
  * **Control Centralizado:** SystemTask tiene la autoridad de apagar o prender los Tickers (mediante ttach() y detach()) dependiendo del estado global de la pulsera. Por ejemplo, si se desconecta el Bluetooth, SystemTask apaga el timer de Keep Alive sin necesidad de enviarle comandos a nadie. El sistema ahorra energía automáticamente.
  * **Eficiencia (Compromiso Aceptado):** Al hacer que la interrupción envíe el mensaje directo al destino, nos ahorramos un "doble salto" (IRQ -> SystemTask -> CommTask), ganando eficiencia operativa en el RTOS a costa de una brevísima "excepción" a la regla de que SystemTask recibe todo, compensado porque SystemTask es quien gobierna dicho Ticker.

### 3. Encapsulamiento de Datos y Claridad Semántica

Se aplicó el principio de responsabilidad única para el pasaje de estructuras complejas a través de colas de mensajes (ej. MetricsData).

* En lugar de procesar los datos directamente al instanciar el envío, se crearon métodos descriptivos como updateMetrics() que solo se encargan de vaciar el StepCounterRepository y actualizar el modelo interno de la clase.
* Posteriormente, en el bloque Switch de la tarea, se toma la dirección de memoria de ese modelo estático (&_shared_metrics_data) y se envía explícitamente hacia el bus de comunicaciones de CommLinkTask. Esto garantiza trazabilidad: cualquier desarrollador puede ver exactamente qué variable está viajando en el paquete sin tener que adentrarse en implementaciones ocultas.
