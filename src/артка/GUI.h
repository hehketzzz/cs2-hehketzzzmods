#pragma once
#include "Config.h"
#include <Windows.h>
#include <d3d9.h>
#include <atomic>

class GUI {
private:
    Config& config;
    HWND hwnd;
    LPDIRECT3DDEVICE9 device;
    LPDIRECT3D9 d3d9;
    D3DPRESENT_PARAMETERS d3dpp;
    bool showMenu;
    std::atomic<bool>* runningPtr;

public:
    GUI(Config& cfg);
    ~GUI();

    bool Initialize();
    void Render();
    void Shutdown();

    void ToggleMenu() { showMenu = !showMenu; }
    bool IsMenuVisible() const { return showMenu; }

    // Для оконных сообщений
    void HandleMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

private:
    void ResetDevice();
    void RenderMenu();
    void RenderESP();
    void RenderAimbot();
    void RenderMisc();
};