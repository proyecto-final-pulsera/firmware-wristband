#pragma once
#include <mbed.h>
#include "app/config.h"

class PmicTask {
private:
    // Hilo desactivado temporalmente para ahorrar RAM (stack + TCB).
    // El ping periodico al PMIC se delega a SystemTask.
    // PmicTask() : _thread(osPriorityNormal, STACK_SIZE_PMIC_TASK) {}
    PmicTask() {}
    ~PmicTask() {}
    PmicTask(const PmicTask&) = delete;
    PmicTask& operator=(const PmicTask&) = delete;

    // rtos::Thread _thread;
    void run();

public:
    static PmicTask& getInstance() {
        static PmicTask instance;
        return instance;
    }
    void init();
};
