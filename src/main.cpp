#include <Arduino.h>
#include "app/app.h"
#include "drivers/battery_driver.h"
#include "rtos.h"
#include "../test/test_sensors_drivers.h"
// #define VIBRATOR_PWM_PIN    5
// #define VIBRATOR_ENABLE_PIN 3   
// mbed::PwmOut* _pwm_pin;
// mbed::DigitalOut* _enable_pin;
void setup() {
    Serial.begin(115200);
    
    // //Inicializar toda la arquitectura de la app (Drivers, Tareas, ISRs)
    //  _pwm_pin = new mbed::PwmOut(digitalPinToPinName(VIBRATOR_PWM_PIN));
    // // Pasamos un 0 como segundo parámetro para que nazca en LOW y evitar glitches
    // _enable_pin = new mbed::DigitalOut(digitalPinToPinName(VIBRATOR_ENABLE_PIN), 0);
    
    // // Configurar frecuencia de PWM: 20kHz -> periodo = 1/20000 = 50 microsegundos
    // _pwm_pin->period_us(50);
    
    // // Inicia deshabilitado
    // _pwm_pin->write(0.0f);
    
    App::init();
    
    // runSensorsDriversTest();
}

void loop() {
    rtos::ThisThread::sleep_for(std::chrono::milliseconds(50));
}
