#pragma once
#include <Arduino.h>
#include "Nicla_System.h"

class LedDriver {
private:
    LedDriver();
    static LedDriver* _instance;
    
    bool _ledState;
    RGBColors _ledColor;
    
    // Prohibir copia
    LedDriver(const LedDriver&) = delete;
    LedDriver& operator=(const LedDriver&) = delete;

public:
    static LedDriver* getInstance();

    void init();
    
    void setLedState(bool active);
    void setLedColor(RGBColors color);
    
    void setLedNotif();
    void setLedAlarm();
    void setLedWarn();
    
    void toggleLed();
};
