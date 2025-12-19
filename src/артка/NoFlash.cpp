#include "NoFlash.h"
#include <Windows.h>
#include <iostream>

NoFlash::NoFlash(Memory& mem, Config& cfg)
    : memory(mem), config(cfg), running(false) {
}

NoFlash::~NoFlash() {
    Stop();
}

void NoFlash::Start() {
    if (running) return;

    running = true;
    noflashThread = std::thread(&NoFlash::RunLoop, this);

    std::cout << "[NoFlash] Started" << std::endl;
}

void NoFlash::Stop() {
    if (!running) return;

    running = false;
    if (noflashThread.joinable()) {
        noflashThread.join();
    }

    std::cout << "[NoFlash] Stopped" << std::endl;
}

void NoFlash::RunLoop() {
    while (running) {
        if (config.noFlash) {
            Memory::LocalPlayer local = memory.ReadLocalPlayer();
            if (local.pawn && local.health > 0) {
                // Устанавливаем flash в 0
                float zero = 0.0f;
                uintptr_t flashAddr = local.pawn + 0x160C; // m_flFlashMaxAlpha
                memory.Write<float>(flashAddr, zero);
            }
        }
        Sleep(10); // Частое обновление
    }
}