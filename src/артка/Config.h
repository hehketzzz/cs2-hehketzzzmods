#pragma once
#include <string>

struct Config {
    // ESP настройки
    bool espEnabled = true;
    bool espBox = true;
    bool espHealth = true;
    bool espName = true;
    bool espTeam = false;
    bool espSkeleton = false;
    bool espDistance = false;
    float espMaxDistance = 500.0f;

    // Цвета ESP
    float espEnemyColor[3] = { 1.0f, 0.0f, 0.0f }; // RGB красный
    float espTeamColor[3] = { 0.0f, 1.0f, 0.0f };  // RGB зеленый
    float espBoxColor[3] = { 1.0f, 1.0f, 1.0f };   // RGB белый

    // Aimbot настройки
    bool aimbotEnabled = false;
    bool aimbotTeamCheck = true;
    bool aimbotVisibleCheck = false;
    bool aimbotAutoFire = false;
    float aimbotFov = 10.0f;
    float aimbotSmooth = 0.5f;
    int aimbotBone = 0; // 0=голова, 1=грудь, 2=таз
    int aimbotKey = 'X';

    // Misc настройки
    bool bhopEnabled = false;
    bool triggerBotEnabled = false;
    bool radarHack = false;
    bool noFlash = false;

    // Позиция окна
    float windowPos[2] = { 100.0f, 100.0f };
    float windowSize[2] = { 600.0f, 400.0f };

    // Метод сброса настроек
    void Reset() {
        *this = Config();
    }
};