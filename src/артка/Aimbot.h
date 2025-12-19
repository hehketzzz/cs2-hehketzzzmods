#pragma once
#include "Memory.h"
#include "Config.h"

struct Vector2 {
    float x, y;
    Vector2() : x(0), y(0) {}
    Vector2(float x, float y) : x(x), y(y) {}

    float Distance(const Vector2& other) const {
        float dx = x - other.x;
        float dy = y - other.y;
        return sqrtf(dx * dx + dy * dy);
    }
};

struct QAngle {
    float x, y, z;
    QAngle() : x(0), y(0), z(0) {}
    QAngle(float x, float y, float z) : x(x), y(y), z(z) {}
};

struct ViewMatrix {
    float matrix[4][4];

    bool WorldToScreen(const Memory::Vector3& pos, const Vector2& screenSize, Vector2& screenPos) const {
        float _x = matrix[0][0] * pos.x + matrix[0][1] * pos.y + matrix[0][2] * pos.z + matrix[0][3];
        float _y = matrix[1][0] * pos.x + matrix[1][1] * pos.y + matrix[1][2] * pos.z + matrix[1][3];
        float w = matrix[3][0] * pos.x + matrix[3][1] * pos.y + matrix[3][2] * pos.z + matrix[3][3];

        if (w < 0.01f) return false;

        float inv_w = 1.f / w;
        _x *= inv_w;
        _y *= inv_w;

        float x = screenSize.x * 0.5f;
        float y = screenSize.y * 0.5f;

        x += 0.5f * _x * screenSize.x + 0.5f;
        y -= 0.5f * _y * screenSize.y + 0.5f;

        screenPos.x = x;
        screenPos.y = y;

        return true;
    }
};

class Aimbot {
private:
    Memory& memory;
    Config& config;
    int aimCount;
    Vector2 screenSize;

public:
    Aimbot(Memory& mem, Config& cfg);

    void Run();

private:
    void ProcessAimbot();
    Memory::Player FindEnemyClosestToCrosshair(const Memory::LocalPlayer& local, const ViewMatrix& viewMatrix);
    void AimAtTarget(const Memory::Vector3& from, const Memory::Vector3& targetPos);
    float NormalizeAngle(float angle);
    Memory::Vector3 GetEyePosition(uintptr_t pawn);
    uintptr_t GetLocalPawn();
};