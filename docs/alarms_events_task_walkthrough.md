# Walkthrough: Tarea de Procesamiento (`AlarmsEventsTask`)

## Estado Actual y Progreso

Hemos comenzado con la implementación de la **Tarea 6** de nuestro *Sprint Actual*, que se ha dividido arquitectónicamente en dos grandes bloques. Hasta el momento, nos hemos enfocado 100% en la `AlarmsEventsTask` (La Tarea de Procesamiento).

### 1. Refactorización a Procesamiento Bajo Demanda (On-Demand)
Se rediseñó el bucle principal (`run`) de la tarea para procesar los datos únicamente bajo demanda y no de forma continua:
- **`CMD_PROCESS_IMU`:** Al recibir este mensaje, la tarea extrae el segmento de datos actual del buffer y lo procesa en ese mismo instante. Luego vuelve al estado de bloqueo pasivo esperando el siguiente comando.
- La tarea permanece suspendida (timeout `osWaitForever`) la mayor parte del tiempo, lo cual asegura el ahorro de batería hasta que una interrupción o el SystemTask solicite un procesamiento.

### 2. Procesamiento Matemático Optimizado
- Se integró la función `approx_2d_improved` y se implementó `suma_pitagorica` para calcular la magnitud del vector de aceleración 3D en base a valores absolutos. 
- Referencia del algoritmo: [Alpha max plus beta min algorithm](https://en.wikipedia.org/wiki/Alpha_max_plus_beta_min_algorithm)
- Esta aproximación evita el uso de operaciones costosas (como la multiplicación y la raíz cuadrada) en el microcontrolador.

### 3. Extracción de la "Ventana de Impacto"
- Se definió que el buffer de la IMU (`IMU_FIFO_SIZE` = 800 muestras) estará, en teoría, siempre lleno durante la ejecución continua.
- Se lee un segmento de 6 segundos (`IMPACT_WINDOW_SIZE` = 300 muestras) ubicado exactamente en el **medio** del histórico del buffer, extrayéndolo con un índice precalculado por el compilador: `START_IMPACT_WINDOW_INDEX = (IMU_FIFO_SIZE / 2) - (IMPACT_WINDOW_SIZE / 2)`.
- Quedó anotado un `TODO` para optimizar el copiado y aplicar un desplazamiento que evite procesar datos redundantes en el futuro.

### 4. Detección por "Ventana Deslizante" (Sliding Window)
- Se barre la ventana de impacto (300 muestras) utilizando una sub-ventana deslizante de tamaño 8 (`SLIDING_WINDOW_SIZE = 8`).
- Se utiliza un desplazamiento de bits (`>> 3`) en lugar de una división matemática para promediar la magnitud de la sub-ventana de forma extremadamente rápida.
- Si el promedio baja del `THRESHOLD_FREE_FALL`, se levanta una bandera.
- Si supera el `THRESHOLD_IMPACT`, se levanta otra bandera.
- Si ambas banderas se detectan en la misma ventana de impacto de 6 segundos, se cumple la `CONDICION DE CAIDA` y se interrumpe la búsqueda.

## Próximos Pasos (Pendientes)
* **Validación Inicial "Recién Despertado"**: Se deberá contemplar un escenario borde donde el micro recién sale del modo reposo y no debe analizar solo el centro, sino el buffer **completo**.
* **Calibración de Umbrales**: Reemplazar los valores provisorios de los THRESHOLD con valores empíricos obtenidos de pruebas físicas en la pulsera.
* **Implementación de `SystemTask`**: Construir la tarea coordinadora que orquestará a los periféricos, gestionará las IRQ de hardware y será la encargada de enviar los mensajes `CMD_PROCESS_IMU` a esta Tarea de Procesamiento.
