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

## 9. Tarea de Procesamiento de Caídas (AlarmsEventsTask)

### 9.1 Refactorización a Procesamiento Bajo Demanda (On-Demand)

Se rediseñó el bucle principal (`run()`) de la tarea para procesar los datos únicamente bajo demanda y no de forma continua:

- **CMD_PROCESS_IMU:** Al recibir este mensaje, la tarea extrae el segmento de datos actual del buffer y lo procesa en ese mismo instante. Luego vuelve al estado de bloqueo pasivo esperando el siguiente comando.
- La tarea permanece suspendida (timeout `osWaitForever`) la mayor parte del tiempo, lo cual asegura el ahorro de batería hasta que una interrupción o el `SystemTask` solicite un procesamiento.

### 9.2 Procesamiento Matemático Optimizado

- Se integró la función `approx_2d_improved` y se implementó `suma_pitagorica` para calcular la magnitud del vector de aceleración 3D en base a valores absolutos.
- Referencia del algoritmo: [Alpha max plus beta min algorithm](https://en.wikipedia.org/wiki/Alpha_max_plus_beta_min_algorithm)
- Esta aproximación evita el uso de operaciones costosas (como la multiplicación y la raíz cuadrada) en el microcontrolador.
- **Rendimiento medido:** El tiempo de procesamiento para un buffer de 6 segundos sin reutilizar muestras previas (es decir, recalculando absolutamente todo el buffer) es de **2700 us** con la optimización de valor módulo. En contraposición, si se utiliza la expresión real (raíz cuadrada de la sumatoria de los cuadrados), el tiempo asciende a **50000 us**. Finalmente, al precalcular el arreglo de módulos antes de recorrer la ventana deslizante (evitando calcular múltiples veces la misma muestra), el tiempo se reduce drásticamente a **800 us**.

### 9.3 Extracción de la "Ventana de Impacto"

- Se definió que el buffer de la IMU (`IMU_FIFO_SIZE` = 800 muestras) estará, en teoría, siempre lleno durante la ejecución continua.
- Se lee un segmento de 6 segundos (`IMPACT_WINDOW_SIZE` = 300 muestras) ubicado exactamente en el **medio** del histórico del buffer, extrayéndolo con un índice precalculado por el compilador: `START_IMPACT_WINDOW_INDEX = (IMU_FIFO_SIZE / 2) - (IMPACT_WINDOW_SIZE / 2)`.

### 9.4 Detección por "Ventana Deslizante" (Sliding Window)

- Se barre la ventana de impacto (300 muestras) utilizando una sub-ventana deslizante de tamaño 8 (`SLIDING_WINDOW_SIZE = 8`).
- Se utiliza un desplazamiento de bits (`>> 3`) en lugar de una división matemática para promediar la magnitud de la sub-ventana de forma extremadamente rápida.
- Si el promedio baja del `THRESHOLD_FREE_FALL`, se levanta una bandera.
- Si supera el `THRESHOLD_IMPACT`, se levanta otra bandera.
- Si ambas banderas se detectan en la misma ventana de impacto de 6 segundos, se cumple la CONDICION DE CAIDA y se interrumpe la búsqueda.

### 9.5 Validación Inicial "Wake Up" y Procesamiento Pre-Fall

- Se implementó un comando especial `CMD_PROCESS_IMU_WAKEUP` (enviado por el `SystemTask` cuando detecta que el buffer se llenó por completo).
- **Justificación de Diseño:** En la primera llamada luego de un *wake up*, es necesario procesar el *pre-fall* desde el inicio del buffer, por si el evento de caída sucedió inmediatamente una vez se despertó el equipo y no dio tiempo a loguear el *pre-fall* en el flujo habitual.
- Para ahorrar memoria RAM, se reutiliza el mismo buffer (`_imu_window_buffer`). Primero se procesan los 6 segundos iniciales, y si no hay alarma, se procesan los 6 segundos centrales, arrastrando las banderas de estado (`fall_flags`) con una compuerta *OR* a través de ambas etapas.
- Dependiendo de si se evaluó esta condición de *Wake Up*, el mensaje final de alarma (a través de `CMD_TX_ALARM`) adjuntará el flag `PRE_FALL_PROCESSED`.
- **Rendimiento:** Al procesar dos ventanas secuenciales en el peor de los casos (cuando no se detecta alarma tempranamente), esta función específica demora el doble de tiempo (aprox. **1700 us** frente a los 800 us regulares).
- **Limitación:** Por la implementación actual de reutilización del mismo buffer, SOLO soporta tiempos de *pre_fall* menores al tiempo de la ventana de procesamiento (menos 1 segundo de margen), es decir `< WINDOW_PROCESAMIENTO - 1Seg`.

### 9.6 Calibración de Sensibilidad: Umbral de Caída Libre (THRESHOLD_FREE_FALL = 2000)

En la máquina de detección de caídas en dos fases, el algoritmo busca secuencialmente una fase de **ingravidez o caída libre** (donde la aceleración resultante cae por debajo de la gravedad terrestre de 1G) y un posterior **impacto violento** (`THRESHOLD_IMPACT = 7000`) dentro de una ventana de impacto común de 6 segundos.

* **El Problema del Umbral Inicial (1500):** Originalmente, el parámetro estaba configurado en `#define THRESHOLD_FREE_FALL 1500`. En pruebas experimentales y dinámicas con sujetos reales se observó que este valor resultaba excesivamente estricto: únicamente caídas bruscas en caída libre vertical pura lograban que la aceleración cayera por debajo de 1500 unidades raw. En caídas más realistas (tropezones con rotación, resbalones donde la persona intenta estabilizarse o movimientos involuntarios de braceo), la aceleración promedio de la ventana deslizante no llegaba a perforar dicho piso. Como consecuencia, el sistema generaba **falsos negativos** (caídas reales que no eran detectadas).
* **Calibración a 2000:** Al elevar el umbral a `#define THRESHOLD_FREE_FALL 2000` en `include/app/config.h`:
  1. **Mayor Sensibilidad:** El algoritmo detecta con notable anticipación y facilidad la fase de desbalance e ingravidez corporal, capturando caídas más suaves, amortiguadas o con componentes angulares/rotacionales.
  2. **Reducción Drástica de Falsos Negativos:** En la batería de capturas con datos reales, caídas que antes eran descartadas por escaso margen ahora activan oportunamente la bandera de caída libre.
  3. **Preservación de la Especificidad:** Elevar este umbral no genera falsos positivos, debido a que la arquitectura exige que en la misma ventana de 6 segundos se supere indefectiblemente el umbral de impacto (`THRESHOLD_IMPACT = 7000`). Movimientos cotidianos de baja aceleración (ej. descansar el brazo o sentarse) no desarrollan la energía de colisión necesaria para consolidar la alarma.

---

## 10. Decisiones Arquitectónicas: Tareas Cíclicas y Manejo de Interrupciones

### 10.1 Filosofía de Creación de Tareas (Overhead vs. Aislamiento)

Al decidir cómo manejar eventos cíclicos (ping al PMIC, Keep Alive, reporte de Métricas), se evaluó el balance entre el "overhead" de memoria y el aislamiento de fallos:

* **Aislamiento y Prioridad (Por qué crear PmicTask):** El ping al PMIC es una operación crítica a nivel de hardware. Si el PMIC no recibe un ping I2C en 50s, corta la energía de la placa. Se asignó esta responsabilidad a una tarea exclusiva (`PmicTask`) que simplemente hace `sleep_for(40000)` y envía el ping. Al aislarla, nos aseguramos de que ningún bug o retraso en otras tareas (ej. procesar sensores) provoque un reinicio de la pulsera.
* **Consumo de RAM (Por qué NO crear más tareas):** Cada tarea nueva en Mbed OS consume ~1KB de RAM para su Stack. Dado que el MCU nRF52832 cuenta con sólo 64KB de RAM (y que tecnologías pendientes como BLE o LittleFS requerirán gran parte de ella), crear una tarea dedicada para cada acción cíclica menor (ej. un `task_keepalive`, un `task_bateria`) no es escalable ni viable.
* **La Solución Híbrida:** Todo evento no crítico, de baja prioridad o ligado a la interfaz de usuario / telemetría se agrupó bajo `NotifyTelemetryTask`. Esta tarea no genera eventos por su cuenta; se limita a reaccionar a la cola de mensajes para leer estado (Batería, Repositorios) y enviarlo a `CommLinkTask`.

### 10.2 Patrón "Central Dispatcher" (Orquestación en SystemTask)

Para evitar que `NotifyTelemetryTask` o `CommLinkTask` calcularan sus propios timeouts dinámicamente o llevaran lógica de scheduling, implementamos un esquema donde `SystemTask` es el único orquestador de los eventos asincrónicos o basados en tiempo.

* **Implementación con mbed::Ticker:** Se instanciaron Timers de Hardware directamente dentro de la clase `SystemTask` (ej. `_keepAliveTicker` y `_metricsTicker`).
* **Flujo de Ejecución:**
  1. El temporizador cuenta asincrónicamente usando el hardware del microcontrolador (Cero uso de CPU de la tarea).
  2. Cuando expira, se dispara una interrupción de hardware (ISR), congelando temporalmente el RTOS.
  3. La ISR, cuyo método "callback" pertenece a `SystemTask`, envía un mensaje de ejecución *directamente a la cola de la tarea responsable* usando el envoltorio `notifyTask`. Por ejemplo, envía `CMD_TX_KEEP_ALIVE` a `CommLinkTask`.
  4. La ISR termina inmediatamente. El Scheduler de Mbed OS detecta el mensaje nuevo y despierta a la tarea correspondiente para que lo procese en "background" con prioridad baja.
* **Beneficios de este Modelo:**
  * **Stateless:** Tareas como `NotifyTelemetryTask` quedan libres de variables de tiempo. Su bucle `run()` es un simple, pasivo y eficiente `_mailbox.receive(osWaitForever)`.
  * **Control Centralizado:** `SystemTask` tiene la autoridad de apagar o prender los Tickers (mediante `attach()` y `detach()`) dependiendo del estado global de la pulsera. Por ejemplo, si se desconecta el enlace, `SystemTask` apaga el timer de Keep Alive sin necesidad de enviarle comandos a nadie. El sistema ahorra energía automáticamente.
  * **Eficiencia (Compromiso Aceptado):** Al hacer que la interrupción envíe el mensaje directo al destino, nos ahorramos un "doble salto" (IRQ -> `SystemTask` -> `CommTask`), ganando eficiencia operativa en el RTOS a costa de una brevísima "excepción" a la regla de que `SystemTask` recibe todo, compensado porque `SystemTask` es quien gobierna dicho Ticker.

### 10.3 Encapsulamiento de Datos y Claridad Semántica

Se aplicó el principio de responsabilidad única para el pasaje de estructuras complejas a través de colas de mensajes (ej. `MetricsData`).

* En lugar de procesar los datos directamente al instanciar el envío, se crearon métodos descriptivos como `updateMetrics()` que solo se encargan de vaciar el `StepCounterRepository` y actualizar el modelo interno de la clase.
* Posteriormente, en el bloque Switch de la tarea, se toma la dirección de memoria de ese modelo estático (`&_shared_metrics_data`) y se envía explícitamente hacia el bus de comunicaciones de `CommLinkTask`. Esto garantiza trazabilidad: cualquier desarrollador puede ver exactamente qué variable está viajando en el paquete sin tener que adentrarse en implementaciones ocultas.

---

## 11. Concurrencia, Thread-Safety y Mecanismo de Locks en Repositorios (ImuRepository y PressureRepository)

### 11.1 Problemática de Concurrencia: Productor vs. Consumidor en RTOS

En el firmware, `ImuRepository` y `PressureRepository` son recursos compartidos entre múltiples hilos del sistema operativo en tiempo real (Mbed OS / RTX5):

* **Hilo Productor (`AlarmsEventsTask`):** Se encarga de drenar la FIFO de hardware del BHI260AP (`updateFifoData()`) y procesar los paquetes de los sensores a través de callbacks, ejecutando continuamente llamadas a `push(nueva_muestra)`.
* **Hilo Consumidor (`CommLinkTask`):** Al producirse una alarma de caída (`CMD_TX_FALL_SENSORS`, `CMD_TX_IMU_BUFFER` o `CMD_TX_PRESSURE_BUFFER`), debe serializar y transmitir el historial completo de muestras contiguas (ej. hasta 800 muestras = 4800 bytes en el caso de la IMU) a través del enlace de comunicación físico (`comm->sendPayload()`).

La transmisión física de tramas de varios kilobytes a través de UART/USB o BLE toma un tiempo finito no despreciable (decenas de milisegundos).

### 11.2 La Condición de Carrera con Exclusión Mutua Granular Estándar

Originalmente, los métodos individuales de los repositorios contaban con su propio Mutex interno de ámbito local (`rtos::ScopedMutexLock lock(_mutex);`). 

Si `CommLinkTask` operaba con el flujo clásico desacoplado:
```cpp
// PELIGRO: Condición de carrera clásica
const DataXYZ* buf = imu->getLinearBuffer(&count); // Mutex adquirido y LIBERADO dentro del método
comm->sendPayload(MSG_IMU_BUFFER, (const uint8_t*)buf, count * sizeof(DataXYZ)); // Mutex LIBRE durante la transmisión I/O
imu->fifoFlush(); // Mutex adquirido y LIBERADO dentro del método
```

Este diseño introducía una **condición de carrera crítica (Race Condition)**:
1. Al salir de `getLinearBuffer()`, el Mutex interno se libera.
2. Mientras `sendPayload()` está transmitiendo byte a byte por el bus serie (demorando milisegundos), ocurre una interrupción del sensor o el planificador le cede la CPU a `AlarmsEventsTask`.
3. `AlarmsEventsTask` invoca `setData()` -> `push(muestra)`, modificando los elementos del array `_fifo`, el puntero `_head` y el contador `_count` **mientras `CommLinkTask` está leyendo esa misma memoria física para enviarla**.
4. Peor aún: cuando `CommLinkTask` finaliza el envío y llama a `imu->fifoFlush()`, resetea `_head = 0`, `_tail = 0`, `_count = 0`. **Esto borra y destruye de forma irreversible todas las muestras nuevas que `AlarmsEventsTask` acababa de escribir durante la transmisión**, generando baches temporales y pérdida de datos en los algoritmos subsiguientes.
5. Adicionalmente, si el buffer circular requiere rotación en memoria (`std::rotate` / `std::memmove`) para linearizarse, la intervención concurrente de un `push()` corrompe la alineación contigua de memoria, pudiendo provocar hard faults.

### 11.3 Solución: Bloqueo Manual Extendido (`lock()` / `unlock()`) y Primitivas `*Unprotected()`

Para garantizar la integridad total de la trama sin recurrir a duplicación de memoria (evitando buffers estáticos o dinámicos adicionales de 4.8 KB en un microcontrolador con solo 64 KB de RAM), la exclusión mutua debe abarcar **toda la transacción atómica**, protegiendo tanto la linearización, la transmisión física por el bus y el posterior vaciado.

Se implementó el patrón de **Bloqueo Extendido**:

1. **Control Explícito del Mutex:** Se exponen en la interfaz pública de los repositorios los métodos:
   * `void lock();` (ejecuta `_mutex.lock()`)
   * `void unlock();` (ejecuta `_mutex.unlock()`)
2. **Primitivas Desprotegidas (`*Unprotected()`):** Se diseñaron variantes de los métodos de buffer que operan asumiendo que el llamador ya posee la exclusión mutua:
   * `const DataXYZ* getLinearBufferUnprotected(uint16_t* outCount);`
   * `void fifoFlushUnprotected();`
   * `bool popUnprotected(DataXYZ& data);`
   * `DataXYZ getElementAtUnprotected(uint16_t index) const;`
   * `uint16_t copyFifoValuesUnprotected(...) const;`
   * `uint16_t getAvailableCountUnprotected() const;`

#### Patrón Canónico de Transmisión en `CommLinkTask`:

```cpp
case CMD_TX_FALL_SENSORS: {
    // --- Bloque Atómico de IMU ---
    ImuRepository* imu = ImuRepository::getInstance();
    imu->lock(); // 1. Adquiere el mutex: AlarmsEventsTask se bloquea si intenta escribir
    uint16_t imuCount = 0;
    const DataXYZ* imuBuf = imu->getLinearBufferUnprotected(&imuCount); // 2. Lineariza in-place sin re-adquirir mutex
    if (imuBuf != nullptr && imuCount > 0) {
        comm->sendPayload(MSG_IMU_BUFFER, (const uint8_t*)imuBuf, imuCount * sizeof(DataXYZ)); // 3. Envío seguro I/O
        imu->fifoFlushUnprotected(); // 4. Vacía únicamente lo transmitido
    }
    imu->unlock(); // 5. Libera el mutex: AlarmsEventsTask reanuda la escritura en una FIFO limpia

    // --- Bloque Atómico de Presión ---
    PressureRepository* pres = PressureRepository::getInstance();
    pres->lock();
    uint16_t presCount = 0;
    const float* presBuf = pres->getLinearBufferUnprotected(&presCount);
    if (presBuf != nullptr && presCount > 0) {
        comm->sendPayload(MSG_PRESSURE_BUFFER, (const uint8_t*)presBuf, presCount * sizeof(float));
        pres->fifoFlushUnprotected();
    }
    pres->unlock();

    // Envío de temperatura
    float temp = TemperatureRepository::getInstance()->getTemp();
    comm->sendPayload(MSG_TEMPERATURE, (const uint8_t*)&temp, sizeof(float));
    break;
}
```

### 11.4 Reglas de Buenas Prácticas para Desarrolladores

Al extender o interactuar con clases tipo Repositorio (`ImuRepository`, `PressureRepository`, etc.):

1. **Operaciones Puntuales / Transaccionales Cortas:**
   Si una tarea solo necesita consultar o insertar un dato aislado (ej. `push()`, `pop()`, `getElementAt()`, `isFull()`), debe utilizar **siempre la versión estándar protegida**. Éstas garantizan protección automática vía RAII (`rtos::ScopedMutexLock`) sin riesgo de olvidos de liberación.
2. **Operaciones Compuestas o Envíos Masivos (I/O):**
   Si se realiza una secuencia de lectura contigua seguida de un vaciado (`getLinearBuffer` + `sendPayload` + `fifoFlush`), **es obligatorio** envolver la secuencia completa entre `repo->lock()` y `repo->unlock()`, llamando exclusivamente a los métodos `*Unprotected()` dentro del bloque crítico.
3. **Prohibido Anidar Métodos Protegidos dentro de un `lock()` Manual:**
   Nunca debe llamarse a un método protegido estándar (ej. `fifoFlush()`) después de haber ejecutado `repo->lock()`. En Mbed OS / RTX5, si el Mutex no está configurado como recursivo, esto provocará un **Deadlock (bloqueo mutuo permanente)** inmediato de la tarea.
4. **Minimizar el Tiempo de Retención del Mutex:**
   El envío a través de `sendPayload()` dentro del bloque de lock detiene temporalmente la absorción de muestras en `AlarmsEventsTask`. La tasa de baudios de UART y la eficiencia de la interfaz deben ser lo suficientemente altas para que la transmisión concluya antes de que la FIFO de hardware del BHI260AP (~14 KB) se desborde.
