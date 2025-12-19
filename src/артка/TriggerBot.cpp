#include "TriggerBot.h"
#include <Windows.h>
#include <iostream>

TriggerBot::TriggerBot(Memory& mem, Config& cfg)
    : memory(mem), config(cfg), running(false),
    totalShots(0), loopCount(0) {
}

TriggerBot::~TriggerBot() {
    Stop();
}

void TriggerBot::Run() {
    if (running) return;

    running = true;
    botThread = std::thread([this]() {
        std::cout << "[TriggerBot] Thread started" << std::endl;

        while (running) {
            if (config.triggerBotEnabled) {
                ProcessTrigger();
            }
            Sleep(1);
        }

        std::cout << "[TriggerBot] Stopped. Total shots: " << totalShots << std::endl;
        });
}

void TriggerBot::Stop() {
    if (!running) return;

    running = false;
    if (botThread.joinable()) {
        botThread.join();
    }
}

void TriggerBot::ProcessTrigger() {
    Memory::LocalPlayer local = memory.ReadLocalPlayer();
    if (!local.pawn || local.health <= 0) return;

    // Получаем handle сущности под прицелом (работающий метод!)
    DWORD targetHandle = memory.GetCrosshairTarget(local.pawn);

    // -1 = ничего не наведено
    if (targetHandle == (DWORD)-1) {
        return;
    }

    // Получаем entity из handle
    uintptr_t targetEntity = memory.GetEntityFromHandle(targetHandle);
    if (!targetEntity) {
        return;
    }

    // Проверяем здоровье цели
    int targetHealth = memory.Read<int32_t>(targetEntity + 0x34C); // m_iHealth
    if (targetHealth <= 0) return;

    // Проверяем команду цели
    int targetTeam = memory.Read<uint8_t>(targetEntity + 0x3EB); // m_iTeamNum

    // Стреляем если враг
    if (local.team != targetTeam) {
        // Вывод в консоль для отладки
        static int debugCounter = 0;
        if (debugCounter++ % 20 == 0) {
            std::cout << "[TriggerBot] SHOOT! Enemy health: " << targetHealth
                << " | Enemy team: " << targetTeam
                << " | Handle: " << targetHandle << std::endl;
        }

        Shoot();
        totalShots++;

        // Небольшая задержка между выстрелами
        Sleep(50);
    }
}

void TriggerBot::Shoot() {
    // Быстрое нажатие мыши (как в работающем триггерботе)
    mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
    Sleep(10);
    mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
}