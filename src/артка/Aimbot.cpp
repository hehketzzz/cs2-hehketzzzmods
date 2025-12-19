#include "Aimbot.h"
#include <Windows.h>
#include <iostream>
#include <cmath>
#include <cfloat>

Aimbot::Aimbot(Memory& mem, Config& cfg)
    : memory(mem), config(cfg), aimCount(0) {

    HWND desktop = GetDesktopWindow();
    RECT desktopRect;
    GetClientRect(desktop, &desktopRect);
    screenSize.x = static_cast<float>(desktopRect.right);
    screenSize.y = static_cast<float>(desktopRect.bottom);

    HWND cs2Window = memory.GetGameWindow();
    if (cs2Window) {
        RECT windowRect;
        GetClientRect(cs2Window, &windowRect);
        screenSize.x = static_cast<float>(windowRect.right - windowRect.left);
        screenSize.y = static_cast<float>(windowRect.bottom - windowRect.top);
    }
}

void Aimbot::Run() {
    std::cout << "[Aimbot] Thread started" << std::endl;

    while (true) {
        if (config.aimbotEnabled) {
            ProcessAimbot();
        }
        Sleep(5);
    }
}

void Aimbot::ProcessAimbot() {
    if (GetAsyncKeyState(config.aimbotKey) & 0x8000) {
        Memory::LocalPlayer local = memory.ReadLocalPlayer();
        if (!local.pawn || local.health <= 0) return;

        ViewMatrix viewMatrix;
        Memory::view_matrix_t rawMatrix = memory.ReadViewMatrix();
        memcpy(viewMatrix.matrix, rawMatrix.matrix, sizeof(viewMatrix.matrix));

        // Поиск цели как во втором коде
        Vector2 crosshairPos(screenSize.x / 2, screenSize.y / 2);
        Memory::Player bestTarget;
        float bestDistance = FLT_MAX;

        std::vector<Memory::Player> players = memory.ReadPlayers(local, !config.espTeam);

        for (const auto& player : players) {
            if (player.health <= 0) continue;

            Vector2 screenPos;
            if (!viewMatrix.WorldToScreen(player.head, screenSize, screenPos)) {
                continue;
            }

            float distance = crosshairPos.Distance(screenPos);

            if (distance <= config.aimbotFov && distance < bestDistance) {
                bestDistance = distance;
                bestTarget = player;
            }
        }

        if (bestTarget.pCSPlayerPawn) {
            Memory::Vector3 eyePos = GetEyePosition(local.pawn);
            Memory::Vector3 targetEyePos = GetEyePosition(bestTarget.pCSPlayerPawn);

            // Коррекция вниз на 8 единиц как во втором коде
            Memory::Vector3 enemyAimPos = Memory::Vector3(
                targetEyePos.x,
                targetEyePos.y,
                targetEyePos.z
            );

            AimAtTarget(eyePos, enemyAimPos);
            aimCount++;

            if (aimCount % 5 == 0) {
                std::cout << "[Aimbot] Aimed " << aimCount << " times" << std::endl;
            }
        }
    }
}

Memory::Player Aimbot::FindEnemyClosestToCrosshair(const Memory::LocalPlayer& local, const ViewMatrix& viewMatrix) {
    // Функция для совместимости, но не используется
    return Memory::Player();
}

void Aimbot::AimAtTarget(const Memory::Vector3& from, const Memory::Vector3& targetPos) {
    // Вычисляем delta вручную
    Memory::Vector3 delta;
    delta.x = targetPos.x - from.x;
    delta.y = targetPos.y - from.y;
    delta.z = targetPos.z - from.z;

    if (delta.x == 0 && delta.y == 0 && delta.z == 0) {
        return;
    }

    // Рассчитываем дистанцию в плоскости XY
    float distance = sqrtf(delta.x * delta.x + delta.y * delta.y);

    // Угол по горизонтали (yaw)
    float yaw = atan2f(delta.y, delta.x) * (180.0f / 3.14159265f);

    // Угол по вертикали (pitch)
    float pitch = -atan2f(delta.z, distance) * (180.0f / 3.14159265f);

    // Нормализуем углы
    yaw = NormalizeAngle(yaw);
    pitch = NormalizeAngle(pitch);

    // Ограничиваем pitch
    if (pitch > 89.0f) pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;

    QAngle newAngles(pitch, yaw, 0);

    // Основной способ: запись в dwViewAngles
    uintptr_t viewAnglesAddr = memory.GetClientBase() + 0x1E3C800;
    memory.Write<QAngle>(viewAnglesAddr, newAngles);

    static int debugCounter = 0;
    if (debugCounter++ % 50 == 0) {
        std::cout << "[Aimbot] Angles - Pitch: " << pitch << ", Yaw: " << yaw << std::endl;
    }
}

float Aimbot::NormalizeAngle(float angle) {
    while (angle > 180.0f) angle -= 360.0f;
    while (angle < -180.0f) angle += 360.0f;
    return angle;
}

Memory::Vector3 Aimbot::GetEyePosition(uintptr_t pawn) {
    return memory.GetEyePosition(pawn);
}

uintptr_t Aimbot::GetLocalPawn() {
    uintptr_t clientBase = memory.GetClientBase();
    uintptr_t pawn = memory.Read<uintptr_t>(clientBase + 0x1BEEF28);
    if (pawn) return pawn;

    return 0;
}