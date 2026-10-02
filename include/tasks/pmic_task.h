#pragma once
#include <mbed.h>

class PmicTask {
private:
    PmicTask() {}
    ~PmicTask() {}
    PmicTask(const PmicTask&) = delete;
    PmicTask& operator=(const PmicTask&) = delete;

    rtos::Thread _thread;
    void run();

public:
    static PmicTask& getInstance() {
        static PmicTask instance;
        return instance;
    }
    void init();
};
