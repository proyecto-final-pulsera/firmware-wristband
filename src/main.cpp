#include <Arduino.h>
#include "app/app.h"
#include "rtos.h"
// #include <ArduinoBLE.h>
// #include <Nicla_System.h>

void setup() {
    Serial.begin(115200);
    
    while(!Serial);

    // Inicializar el sistema base de Nicla (necesario para BLE y sensores)
    // TODO Checkear que no rompa nada sacarlo: 
    // nicla::begin();

    App::init();
}

void loop() {

    // TODO Checkear que no rompa nada: 
    rtos::ThisThread::sleep_for(osWaitForever);
}
