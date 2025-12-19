#pragma once
#include "Memory.h"
#include "Config.h"
#include <atomic>
#include <thread>

class TriggerBot {
private:
    Memory& memory;
    Config& config;
    std::atomic<bool> running;
    std::thread botThread;
    int totalShots;
    int loopCount;

public:
    TriggerBot(Memory& mem, Config& cfg);
    ~TriggerBot();

    void Run();
    void Stop();

private:
    void ProcessTrigger();
    void Shoot();
    uintptr_t GetEntityFromHandle(DWORD handle);
};