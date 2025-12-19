// ESP.h
#pragma once
#include "Memory.h"
#include "Config.h"

class ESP {
private:
    Memory& memory;
    Config& config;
    HWND overlayWindow;
    bool showTeammates;
    int lastPlayerCount;

public:
    ESP(Memory& mem, Config& cfg);
    ~ESP();

    bool Initialize();
    void Run();
    void ToggleTeammates();
    bool ShouldShowTeammates() const;
    void Cleanup();  // оепелеярхре б PUBLIC яейжхч

private:
    void UpdateOverlayPosition();
    void ClearOverlay();
    void DrawESP(HDC hdc, const std::vector<Memory::Player>& players,
        const Memory::LocalPlayer& local, const Memory::view_matrix_t& viewMatrix);
};