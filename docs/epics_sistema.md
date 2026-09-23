# Épicas del Sistema - Mediano y Largo Plazo

Estas tareas representan los grandes bloques de trabajo (*Epics*) que deberán ser refinados y desglosados en tareas más pequeñas y estimables a medida que el proyecto avance.

---

### Épica 1: Interacción App Cuidador - Servidor
* **Alcance:** App Móvil / Servidor.
* **Objetivo:** Conectar la interfaz del cuidador con el backend.
* **Notas para futuro desglose:** 
  * Requerirá dividirse en *endpoints* específicos: Autenticación/Login, Recepción de Alertas en tiempo real (FCM/WebSockets) y Consulta de Estado (batería, conexión de la pulsera).

### Épica 2: Seguridad y Gestión de Secretos en la App
* **Alcance:** App Móvil / DevOps.
* **Objetivo:** Evitar la exposición de datos sensibles (API Keys, configuraciones de base de datos) en el repositorio Git.
* **Notas para futuro desglose:** 
  * Implementar variables de entorno (`.env`).
  * Configurar `.gitignore`.
  * Revocar y rotar cualquier clave que ya haya sido *pusheada* por error en el pasado.

### Épica 3: Validación del Pipeline de Inferencia
* **Alcance:** Servidor / Machine Learning / Firmware.
* **Objetivo:** Asegurar que los datos crudos emitidos por la pulsera en tiempo real mantengan la misma calidad, escala y forma que los datos teóricos usados para entrenar el modelo.
* **Notas para futuro desglose:** 
  * Armar un script que inyecte un *stream* de la pulsera (vía serie) hacia la inferencia.
  * Comparar estadísticamente (escala, *jitter*, frecuencia de muestreo) contra el *dataset* ideal de entrenamiento.

### Épica 4: Expansión del Dataset de Entrenamiento
* **Alcance:** Data Science / Recolección en campo.
* **Objetivo:** Obtener un set de eventos más exhaustivo para robustecer el modelo.
* **Notas para futuro desglose:** 
  * Para evitar tareas infinitas, se deberá definir una cuota dura. Ej: "50 caídas simuladas por 3 usuarios distintos" y "100 eventos de Actividades de la Vida Diaria (ADL)".

### Épica 5: Alineación Espacial del Acelerómetro (Servidor)
* **Alcance:** Servidor / Procesamiento de señales.
* **Objetivo:** Modificar la orientación de los datos del sensor físico para que sus ejes coincidan con el marco de referencia esperado por el modelo.
* **Notas para futuro desglose:** 
  * Implementar matriz de rotación matemática.
  * Aplicar TDD (*Test Driven Development*): crear tests unitarios con vectores de entrada/salida conocidos antes de integrarlo al pipeline.

### Épica 6: Integración del Stack BLE
* **Alcance:** Firmware / App.
* **Objetivo:** Implementar la comunicación Bluetooth Low Energy completa entre la pulsera y el teléfono/gateway.
* **Notas para futuro desglose:** 
  * Deberá dividirse en:
    1. *Advertising* y control de conexión (GAP).
    2. Creación de Servicios y Características (GATT).
    3. Lógica de Seguridad y Emparejamiento (*Bonding* - relacionado a la Tarea 12 de Firmware).
    4. Transmisión de *payloads* de sensores (*Notifications*).
