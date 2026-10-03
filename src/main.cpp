#include <Arduino.h>
#include "app/app.h"
#include "drivers/battery_driver.h"
#include "rtos.h"
#include "../test/test_sensors_drivers.h"


void setup() {
    Serial.begin(115200);
    
    App::init();

}

void loop() {
    rtos::ThisThread::sleep_for(std::chrono::milliseconds(50));
}
