#pragma once
#include "Memory.h"
#include "Config.h"
#include <thread>  // Добавьте эту строку

class NoFlash {
private:
    Memory& memory;
    Config& config;
    bool running;
    std::thread noflashThread;

    void RunLoop();

public:
    NoFlash(Memory& mem, Config& cfg);
    ~NoFlash();

    void Start();
    void Stop();
};