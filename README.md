
# Firmware — GrandsafeBand - Wristband

Firmware de la pulsera desarrollada para el proyecto de detección de caídas.

El sistema está basado en la  **Arduino Nicla Sense ME** , utilizando un **nRF52832** como microcontrolador y un **Bosch BHI260AP** como sensor inercial principal.

### Hardware

* **MCU:** Nordic nRF52832
* **IMU:** Bosch BHI260AP — acelerómetro + giroscopio
* **Barómetro:** Bosch BMP390
* **Magnetómetro:** Bosch BMM150
* **Sensor ambiental:** Bosch BME688
* **Conectividad:** Bluetooth Low Energy (BLE)

### Software

* C/C++
* ArduinoCore-mbed / Mbed OS
* Bosch BHY2 Sensor API
* PlatformIO + VS Code

El firmware se encarga de la adquisición y procesamiento de datos de los sensores, comunicación BLE y gestión de energía del dispositivo.
