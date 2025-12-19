#pragma once
#include <Windows.h>
#include <string>
#include <vector>
#include <TlHelp32.h>

class Memory {
public:
    HANDLE process;
    uintptr_t clientBase;
    DWORD pid;
    HWND gameWindow;
    RECT gameRect;

    // Структуры
    struct Vector3 {
        float x, y, z;

        Vector3() : x(0), y(0), z(0) {}
        Vector3(float x, float y, float z) : x(x), y(y), z(z) {}

        float Distance2D(const Vector3& other) const {
            float dx = x - other.x;
            float dy = y - other.y;
            return sqrtf(dx * dx + dy * dy);
        }

        std::string ToString() const {
            char buffer[256];
            sprintf_s(buffer, "%.1f, %.1f, %.1f", x, y, z);
            return std::string(buffer);
        }
    };

    struct view_matrix_t {
        float matrix[4][4];
    };

    struct Player {
        std::string name;
        int health;
        int team;
        Vector3 position;
        Vector3 head;
        uintptr_t entity;
        uintptr_t pCSPlayerPawn;

        Player() : health(0), team(0), entity(0), pCSPlayerPawn(0) {}
    };

    struct LocalPlayer {
        uintptr_t controller;
        uintptr_t pawn;
        Vector3 position;
        int team;
        int health;

        LocalPlayer() : controller(0), pawn(0), team(0), health(0) {}
    };

private:
    // Оффсеты (как в работающем триггерботе)
    struct Offsets {
        // Основные
        const uintptr_t dwEntityList = 0x1D13CE8;
        const uintptr_t dwLocalPlayerController = 0x1E1DC18;
        const uintptr_t dwLocalPlayerPawn = 0x1BEEF28;
        const uintptr_t dwViewMatrix = 0x1E323D0;
        const uintptr_t dwViewAngles = 0x1E3C800;

        // Для игроков
        const uintptr_t m_iTeamNum = 0x3EB;
        const uintptr_t m_iHealth = 0x34C;
        const uintptr_t m_hPlayerPawn = 0x6B4;
        const uintptr_t m_iszPlayerName = 0x6E8;
        const uintptr_t m_vOldOrigin = 0x15A0;
        const uintptr_t m_hController = 0x15B8;
        const uintptr_t m_vecViewOffset = 0xD80;

        // Для триггербота (ВАЖНО!)
        const uintptr_t m_iIDEntIndex = 0x3ECC;  // Handle сущности под прицелом

        const int ENTITY_LIST_ENTRY_SIZE = 0x70;
    } offsets;

public:
    Memory();
    ~Memory();

    bool Attach();

    // Чтение памяти - определение в .h для правильной работы шаблонов
    template<typename T>
    T Read(uintptr_t address) {
        T buffer;
        if (process && address) {
            ReadProcessMemory(process, (LPCVOID)address, &buffer, sizeof(T), NULL);
        }
        return buffer;
    }

    template<typename T>
    bool Write(uintptr_t address, const T& value) {
        if (process && address) {
            SIZE_T bytesWritten;
            return WriteProcessMemory(process, (LPVOID)address, &value, sizeof(T), &bytesWritten) != 0;
        }
        return false;
    }

    std::string ReadString(uintptr_t address, size_t size = 128) {
        char buffer[256] = { 0 };
        if (process && address) {
            ReadProcessMemory(process, (LPCVOID)address, buffer, size, NULL);
        }
        return std::string(buffer);
    }

    // Преобразование 3D в 2D (как в работающем ESP)
    Vector3 WorldToScreen(const Vector3& position, const view_matrix_t& viewMatrix) {
        float _x = viewMatrix.matrix[0][0] * position.x + viewMatrix.matrix[0][1] * position.y + viewMatrix.matrix[0][2] * position.z + viewMatrix.matrix[0][3];
        float _y = viewMatrix.matrix[1][0] * position.x + viewMatrix.matrix[1][1] * position.y + viewMatrix.matrix[1][2] * position.z + viewMatrix.matrix[1][3];

        float w = viewMatrix.matrix[3][0] * position.x + viewMatrix.matrix[3][1] * position.y + viewMatrix.matrix[3][2] * position.z + viewMatrix.matrix[3][3];

        if (w < 0.01f) return Vector3(0, 0, 0);

        float inv_w = 1.f / w;
        _x *= inv_w;
        _y *= inv_w;

        float x = gameRect.right * .5f;
        float y = gameRect.bottom * .5f;

        x += 0.5f * _x * gameRect.right + 0.5f;
        y -= 0.5f * _y * gameRect.bottom + 0.5f;

        return Vector3(x, y, w);
    }

    // Чтение локального игрока
    LocalPlayer ReadLocalPlayer() {
        LocalPlayer local;

        local.controller = Read<uintptr_t>(clientBase + offsets.dwLocalPlayerController);
        if (!local.controller) return local;

        local.team = Read<uint8_t>(local.controller + offsets.m_iTeamNum);

        uint32_t localPlayerPawn = Read<uint32_t>(local.controller + offsets.m_hPlayerPawn);
        if (!localPlayerPawn) return local;

        uintptr_t entity_list = Read<uintptr_t>(clientBase + offsets.dwEntityList);
        if (!entity_list) return local;

        uintptr_t localList_entry2 = Read<uintptr_t>(entity_list + 0x8 * ((localPlayerPawn & 0x7FFF) >> 9) + 16);
        local.pawn = Read<uintptr_t>(localList_entry2 + offsets.ENTITY_LIST_ENTRY_SIZE * (localPlayerPawn & 0x1FF));

        if (local.pawn) {
            local.position = Read<Vector3>(local.pawn + offsets.m_vOldOrigin);
            local.health = Read<int32_t>(local.pawn + offsets.m_iHealth);
        }

        return local;
    }

    // Чтение всех игроков (со скипом своей команды)
    std::vector<Player> ReadPlayers(const LocalPlayer& local, bool skipTeammates = true) {
        std::vector<Player> players;

        if (!local.controller) return players;

        uintptr_t entity_list = Read<uintptr_t>(clientBase + offsets.dwEntityList);
        if (!entity_list) return players;

        Vector3 localOrigin = local.position;

        for (int playerIndex = 0; playerIndex <= 64; playerIndex++) {
            Player player;

            uintptr_t list_entry = Read<uintptr_t>(entity_list + (8 * (playerIndex & 0x7FFF) >> 9) + 16);
            if (!list_entry) break;

            player.entity = Read<uintptr_t>(list_entry + offsets.ENTITY_LIST_ENTRY_SIZE * (playerIndex & 0x1FF));
            if (!player.entity) continue;

            if (player.entity == local.controller) continue;

            player.team = Read<uint8_t>(player.entity + offsets.m_iTeamNum);

            // Скипаем свою команду
            if (skipTeammates && player.team == local.team) continue;

            uint32_t playerPawn = Read<uint32_t>(player.entity + offsets.m_hPlayerPawn);
            if (!playerPawn) continue;

            uintptr_t list_entry2 = Read<uintptr_t>(entity_list + 0x8 * ((playerPawn & 0x7FFF) >> 9) + 16);
            player.pCSPlayerPawn = Read<uintptr_t>(list_entry2 + offsets.ENTITY_LIST_ENTRY_SIZE * (playerPawn & 0x1FF));
            if (!player.pCSPlayerPawn) continue;

            player.health = Read<int32_t>(player.pCSPlayerPawn + offsets.m_iHealth);
            if (player.health <= 0 || player.health > 100) continue;

            uint32_t controllerHandle = Read<uint32_t>(player.pCSPlayerPawn + offsets.m_hController);
            if (!controllerHandle) continue;

            int index = controllerHandle & 0x7FFF;
            int segment = index >> 9;
            int entry = index & 0x1FF;

            uintptr_t controllerListSegment = Read<uintptr_t>(entity_list + 0x8 * segment + 0x10);
            uintptr_t controller = Read<uintptr_t>(controllerListSegment + offsets.ENTITY_LIST_ENTRY_SIZE * entry);

            if (controller) {
                player.name = ReadString(controller + offsets.m_iszPlayerName);
                if (player.name.empty()) {
                    player.name = "Bot";
                }
            }
            else {
                player.name = "Unknown";
            }

            player.position = Read<Vector3>(player.pCSPlayerPawn + offsets.m_vOldOrigin);

            // Позиция головы
            player.head = Vector3(player.position.x, player.position.y, player.position.z + 75.f);

            if (player.position.x == 0 && player.position.y == 0 && player.position.z == 0) continue;
            if (player.position.x == localOrigin.x &&
                player.position.y == localOrigin.y &&
                player.position.z == localOrigin.z) continue;

            players.push_back(player);
        }

        return players;
    }

    // Чтение матрицы вида
    view_matrix_t ReadViewMatrix() {
        return Read<view_matrix_t>(clientBase + offsets.dwViewMatrix);
    }

    // Для триггербота - получение entity из handle
    uintptr_t GetEntityFromHandle(DWORD handle) {
        if (handle == 0 || handle == (DWORD)-1) return 0;

        uintptr_t entity_list = Read<uintptr_t>(clientBase + offsets.dwEntityList);
        if (!entity_list) return 0;

        // Формула из работающего триггербота
        int index = handle & 0x7FFF;
        int segment = index >> 9;
        int entry = index & 0x1FF;

        uintptr_t listSegment = Read<uintptr_t>(entity_list + 0x8 * segment + 0x10);
        if (!listSegment) return 0;

        return Read<uintptr_t>(listSegment + offsets.ENTITY_LIST_ENTRY_SIZE * entry);
    }

    // Геттеры
    HWND GetGameWindow() const { return gameWindow; }
    RECT GetGameRect() const { return gameRect; }
    uintptr_t GetClientBase() const { return clientBase; }
    DWORD GetPID() const { return pid; }

    // Получение позиции глаз (для аимбота)
    Vector3 GetEyePosition(uintptr_t pawn) {
        Vector3 footPos = Read<Vector3>(pawn + offsets.m_vOldOrigin);
        Vector3 viewOffset = Read<Vector3>(pawn + offsets.m_vecViewOffset);
        return Vector3(
            footPos.x + viewOffset.x,
            footPos.y + viewOffset.y,
            footPos.z + viewOffset.z
        );
    }

    // Для триггербота - получение handle сущности под прицелом
    DWORD GetCrosshairTarget(uintptr_t localPawn) {
        return Read<DWORD>(localPawn + offsets.m_iIDEntIndex);
    }

private:
    // Вспомогательные методы
    bool FindGameWindow();
    bool FindProcessPID(const wchar_t* processName);
    uintptr_t GetModuleBase(const wchar_t* moduleName);
};