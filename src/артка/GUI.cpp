#include "GUI.h"

// ImGui
#pragma comment(lib, "d3d9.lib")
#define _CRT_SECURE_NO_WARNINGS
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx9.h"

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

#include <iostream>

GUI::GUI(Config& cfg)
    : config(cfg), showMenu(true), hwnd(NULL), device(NULL),
    d3d9(NULL), runningPtr(nullptr) {
    ZeroMemory(&d3dpp, sizeof(d3dpp));
}

GUI::~GUI() {
    Shutdown();
}

bool GUI::Initialize() {
    std::cout << "[GUI] Initializing..." << std::endl;

    // Создаем окно
    WNDCLASSEX wc = {
        sizeof(WNDCLASSEX), CS_CLASSDC,
        [](HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) -> LRESULT {
            if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
                return true;

            switch (msg) {
            case WM_SIZE:
                if (wParam != SIZE_MINIMIZED) {
                    // Handle resize
                }
                return 0;
            case WM_SYSCOMMAND:
                if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT menu
                    return 0;
                break;
            case WM_DESTROY:
                PostQuitMessage(0);
                return 0;
            }
            return DefWindowProc(hWnd, msg, wParam, lParam);
        },
        0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL,
        L"CS2CheatGUI", NULL
    };

    RegisterClassEx(&wc);

    hwnd = CreateWindow(
        wc.lpszClassName, L"CS2 Cheat Menu",
        WS_OVERLAPPEDWINDOW, 100, 100, 600, 400,
        NULL, NULL, wc.hInstance, NULL
    );

    if (!hwnd) {
        std::cout << "[GUI] Failed to create window!" << std::endl;
        return false;
    }

    // Initialize Direct3D
    d3d9 = Direct3DCreate9(D3D_SDK_VERSION);
    if (!d3d9) {
        std::cout << "[GUI] Failed to create Direct3D!" << std::endl;
        return false;
    }

    // Setup Direct3D
    d3dpp.Windowed = TRUE;
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;
    d3dpp.EnableAutoDepthStencil = TRUE;
    d3dpp.AutoDepthStencilFormat = D3DFMT_D16;
    d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_ONE;

    // Create Direct3D device
    if (d3d9->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hwnd,
        D3DCREATE_HARDWARE_VERTEXPROCESSING, &d3dpp, &device) < 0) {
        std::cout << "[GUI] Failed to create D3D device!" << std::endl;
        return false;
    }

    // Initialize ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.IniFilename = nullptr;
    io.LogFilename = nullptr;

    // Setup style
    ImGui::StyleColorsDark();

    // Initialize ImGui backends
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX9_Init(device);

    // Show window
    ShowWindow(hwnd, SW_SHOWDEFAULT);
    UpdateWindow(hwnd);

    std::cout << "[GUI] Initialized successfully!" << std::endl;
    return true;
}

void GUI::Render() {
    if (!showMenu) return;

    // Start new frame
    ImGui_ImplDX9_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    // Render menu
    RenderMenu();

    // End frame
    ImGui::EndFrame();

    // Clear screen
    device->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER,
        D3DCOLOR_RGBA(30, 30, 35, 255), 1.0f, 0);

    // Render ImGui
    if (device->BeginScene() >= 0) {
        ImGui::Render();
        ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
        device->EndScene();
    }

    // Present
    device->Present(NULL, NULL, NULL, NULL);
}

void GUI::Shutdown() {
    std::cout << "[GUI] Shutting down..." << std::endl;

    ImGui_ImplDX9_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    if (device) { device->Release(); device = NULL; }
    if (d3d9) { d3d9->Release(); d3d9 = NULL; }

    if (hwnd) {
        DestroyWindow(hwnd);
        hwnd = NULL;
    }

    std::cout << "[GUI] Shutdown complete" << std::endl;
}

void GUI::HandleMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam);
}

void GUI::ResetDevice() {
    // Handle device reset if needed
}

void GUI::RenderMenu() {
    // Set window position and size
    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(600, 400), ImGuiCond_Always);

    // Begin main window
    ImGui::Begin("CS2 Cheat", &showMenu,
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoCollapse);

    // Title
    ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "CS2 Cheat v1.0");
    ImGui::SameLine();
    ImGui::Text("| INSERT: Hide/Show | END: Exit");
    ImGui::Separator();

    // Tabs
    if (ImGui::BeginTabBar("MainTabs")) {
        // ESP Tab
        if (ImGui::BeginTabItem("ESP")) {
            RenderESP();
            ImGui::EndTabItem();
        }

        // Aimbot Tab
        if (ImGui::BeginTabItem("Aimbot")) {
            RenderAimbot();
            ImGui::EndTabItem();
        }

        // Misc Tab
        if (ImGui::BeginTabItem("Misc")) {
            RenderMisc();
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    // Status bar
    ImGui::Separator();
    ImGui::Spacing();
    ImGui::Text("Status: ");
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "ACTIVE");
    ImGui::SameLine(ImGui::GetWindowWidth() - 120);
    ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);

    ImGui::End();
}

void GUI::RenderESP() {
    // Enable ESP
    ImGui::Checkbox("Enable ESP", &config.espEnabled);
    ImGui::SameLine();
    ImGui::TextColored(config.espEnabled ? ImVec4(0.0f, 1.0f, 0.0f, 1.0f) : ImVec4(1.0f, 0.0f, 0.0f, 1.0f),
        "(%s)", config.espEnabled ? "ON" : "OFF");

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // ESP settings
    ImGui::Checkbox("Show Boxes", &config.espBox);
    ImGui::SameLine(200);
    ImGui::Checkbox("Show Health", &config.espHealth);

    ImGui::Checkbox("Show Names", &config.espName);
    ImGui::SameLine(200);
    ImGui::Checkbox("Show Teammates", &config.espTeam);

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
}

void GUI::RenderAimbot() {
    // Enable Aimbot
    ImGui::Checkbox("Enable Aimbot", &config.aimbotEnabled);
    ImGui::SameLine();
    ImGui::TextColored(config.aimbotEnabled ? ImVec4(0.0f, 1.0f, 0.0f, 1.0f) : ImVec4(1.0f, 0.0f, 0.0f, 1.0f),
        "(%s)", config.aimbotEnabled ? "ON" : "OFF");

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Aim settings
    ImGui::SliderFloat("Field of View", &config.aimbotFov, 1.0f, 600.0f, "%.0f°");

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Key
    ImGui::Text("Aimbot Key: ");
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "X");
    ImGui::SameLine();
    ImGui::Text("(Hold to use)");
}

void GUI::RenderMisc() {
    ImGui::Text("Miscellaneous Features");
    ImGui::Separator();
    ImGui::Spacing();

    // Features
    ImGui::Checkbox("Trigger Bot", &config.triggerBotEnabled);
    ImGui::SameLine();
    ImGui::TextColored(config.triggerBotEnabled ? ImVec4(0.0f, 1.0f, 0.0f, 1.0f) : ImVec4(1.0f, 0.0f, 0.0f, 1.0f),
        "(%s)", config.triggerBotEnabled ? "ON" : "OFF");

    ImGui::Checkbox("No Flash", &config.noFlash);
    ImGui::SameLine();
    ImGui::TextColored(config.noFlash ? ImVec4(0.0f, 1.0f, 0.0f, 1.0f) : ImVec4(1.0f, 0.0f, 0.0f, 1.0f),
        "(%s)", config.noFlash ? "ON" : "OFF");
}