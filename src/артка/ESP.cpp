#include "ESP.h"
#include <Windows.h>
#include <iostream>
#include <string>

// Добавим статические переменные для обхода проблемы с объявлением
static HWND s_overlayWindow = NULL;
static int s_lastPlayerCount = 0;

ESP::ESP(Memory& mem, Config& cfg)
    : memory(mem), config(cfg), showTeammates(false) {
    // Инициализируем статические переменные через конструктор
    s_overlayWindow = NULL;
    s_lastPlayerCount = 0;
}

ESP::~ESP() {
    Cleanup();
}

bool ESP::Initialize() {
    std::cout << "[ESP] Initializing..." << std::endl;

    HWND gameWindow = memory.GetGameWindow();
    if (!gameWindow) {
        std::cout << "[ESP] No game window found!" << std::endl;
        return false;
    }

    WNDCLASSEX wc = { 0 };
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = DefWindowProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)CreateSolidBrush(RGB(0, 0, 0));
    wc.lpszClassName = L"ESPOverlayClass";

    RegisterClassEx(&wc);

    RECT targetRect;
    GetWindowRect(gameWindow, &targetRect);

    s_overlayWindow = CreateWindowEx(
        WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_LAYERED | WS_EX_TOOLWINDOW,
        L"ESPOverlayClass",
        L"ESP Overlay",
        WS_POPUP,
        targetRect.left, targetRect.top,
        targetRect.right - targetRect.left,
        targetRect.bottom - targetRect.top,
        NULL, NULL, GetModuleHandle(NULL), NULL
    );

    if (!s_overlayWindow) {
        std::cout << "[ESP] Failed to create overlay window!" << std::endl;
        return false;
    }

    SetLayeredWindowAttributes(s_overlayWindow, RGB(0, 0, 0), 0, LWA_COLORKEY);
    SetWindowLong(s_overlayWindow, GWL_EXSTYLE, GetWindowLong(s_overlayWindow, GWL_EXSTYLE) | WS_EX_LAYERED);

    ShowWindow(s_overlayWindow, SW_SHOW);
    UpdateWindow(s_overlayWindow);

    std::cout << "[ESP] Initialized successfully!" << std::endl;
    return true;
}

void ESP::Run() {
    std::cout << "[ESP] Thread started" << std::endl;

    bool localRunning = true;
    bool espWasEnabled = config.espEnabled;

    // Очищаем экран при старте
    ClearOverlay();

    while (localRunning) {
        // Проверка горячих клавиш
        if (GetAsyncKeyState(VK_F1) & 1) {
            ToggleTeammates();
            std::cout << "[ESP] Teammate ESP: " << (ShouldShowTeammates() ? "ON" : "OFF") << std::endl;
        }

        if (GetAsyncKeyState(VK_F2) & 1) {
            config.espEnabled = !config.espEnabled;
            std::cout << "[ESP] ESP " << (config.espEnabled ? "ENABLED" : "DISABLED") << std::endl;

            // При выключении очищаем экран
            if (!config.espEnabled) {
                ClearOverlay();
            }
        }

        UpdateOverlayPosition();

        if (config.espEnabled) {
            Memory::LocalPlayer local = memory.ReadLocalPlayer();
            if (!local.pawn || local.health <= 0) {
                ClearOverlay();
                Sleep(100);
                continue;
            }

            bool currentSkipTeammates = !config.espTeam && !showTeammates;
            std::vector<Memory::Player> players = memory.ReadPlayers(local, currentSkipTeammates);
            Memory::view_matrix_t viewMatrix = memory.ReadViewMatrix();

            // Отладочная информация
            if (players.size() != s_lastPlayerCount) {
                s_lastPlayerCount = players.size();
                if (s_lastPlayerCount > 0) {
                    std::cout << "[ESP] Drawing " << s_lastPlayerCount << " players" << std::endl;
                }
            }

            HDC hdc = GetDC(s_overlayWindow);
            if (hdc) {
                // Очищаем экран
                RECT clientRect;
                GetClientRect(s_overlayWindow, &clientRect);
                HBRUSH blackBrush = CreateSolidBrush(RGB(0, 0, 0));
                FillRect(hdc, &clientRect, blackBrush);
                DeleteObject(blackBrush);

                // Рисуем только если есть игроки
                if (!players.empty()) {
                    DrawESP(hdc, players, local, viewMatrix);
                }

                ReleaseDC(s_overlayWindow, hdc);
            }
        }
        else {
            // Если ESP был включен, а теперь выключен - очищаем экран
            if (espWasEnabled) {
                ClearOverlay();
                espWasEnabled = false;
            }
            Sleep(50);
        }

        espWasEnabled = config.espEnabled;
        Sleep(4);
    }

    ClearOverlay();
    Cleanup();
    std::cout << "[ESP] Stopped." << std::endl;
}

void ESP::UpdateOverlayPosition() {
    if (!s_overlayWindow) return;

    HWND gameWindow = memory.GetGameWindow();
    if (!gameWindow) return;

    RECT targetRect;
    GetWindowRect(gameWindow, &targetRect);

    // Проверяем, изменились ли координаты
    static RECT lastRect = { 0 };
    if (memcmp(&lastRect, &targetRect, sizeof(RECT)) != 0) {
        SetWindowPos(
            s_overlayWindow,
            HWND_TOPMOST,
            targetRect.left,
            targetRect.top,
            targetRect.right - targetRect.left,
            targetRect.bottom - targetRect.top,
            SWP_SHOWWINDOW
        );
        lastRect = targetRect;
    }

    SetWindowPos(s_overlayWindow, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
}

void ESP::ClearOverlay() {
    if (!s_overlayWindow) return;

    HDC hdc = GetDC(s_overlayWindow);
    if (hdc) {
        RECT clientRect;
        GetClientRect(s_overlayWindow, &clientRect);

        HBRUSH blackBrush = CreateSolidBrush(RGB(0, 0, 0));
        FillRect(hdc, &clientRect, blackBrush);
        DeleteObject(blackBrush);

        ReleaseDC(s_overlayWindow, hdc);

        InvalidateRect(s_overlayWindow, &clientRect, TRUE);
        UpdateWindow(s_overlayWindow);
    }
}

void ESP::Cleanup() {
    if (s_overlayWindow) {
        ClearOverlay();
        DestroyWindow(s_overlayWindow);
        s_overlayWindow = NULL;
        std::cout << "[ESP] Overlay window destroyed" << std::endl;
    }
}

void ESP::DrawESP(HDC hdc, const std::vector<Memory::Player>& players,
    const Memory::LocalPlayer& local, const Memory::view_matrix_t& viewMatrix) {

    HPEN enemyPen = CreatePen(PS_SOLID, 2, RGB(255, 0, 0));
    HPEN teamPen = CreatePen(PS_SOLID, 2, RGB(0, 255, 0));
    HPEN textPen = CreatePen(PS_SOLID, 1, RGB(255, 255, 255));
    HBRUSH transparentBrush = (HBRUSH)GetStockObject(HOLLOW_BRUSH);

    HPEN oldPen = (HPEN)SelectObject(hdc, enemyPen);
    HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, transparentBrush);

    SetBkMode(hdc, TRANSPARENT);
    HFONT font = CreateFont(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Arial");
    HFONT oldFont = (HFONT)SelectObject(hdc, font);

    for (const auto& player : players) {
        Memory::Vector3 screenFeet = memory.WorldToScreen(player.position, viewMatrix);
        Memory::Vector3 screenHead = memory.WorldToScreen(player.head, viewMatrix);

        if (screenFeet.z < 0.01f || screenHead.z < 0.01f) continue;

        bool isEnemy = (player.team != local.team);

        if (isEnemy) {
            SelectObject(hdc, enemyPen);
            SetTextColor(hdc, RGB(255, 50, 50));
        }
        else {
            SelectObject(hdc, teamPen);
            SetTextColor(hdc, RGB(50, 255, 50));
        }

        float height = screenFeet.y - screenHead.y;
        float width = height / 2.0f;

        float x = screenHead.x - width / 2;
        float y = screenHead.y;

        if (config.espBox) {
            Rectangle(hdc, (int)x, (int)y, (int)(x + width), (int)(y + height));
        }

        // Health bar
        if (config.espHealth) {
            int healthBarWidth = 4;
            int healthBarHeight = (int)height;
            int healthBarX = (int)x - healthBarWidth - 2;
            int healthBarY = (int)y;

            HPEN blackPen = (HPEN)SelectObject(hdc, GetStockObject(BLACK_PEN));
            HBRUSH blackBrush = (HBRUSH)SelectObject(hdc, GetStockObject(BLACK_BRUSH));
            Rectangle(hdc, healthBarX, healthBarY,
                healthBarX + healthBarWidth, healthBarY + healthBarHeight);

            int healthHeight = (healthBarHeight * player.health) / 100;
            int healthY = healthBarY + healthBarHeight - healthHeight;

            COLORREF healthColor;
            if (player.health > 50) {
                healthColor = RGB(0, 255, 0);
            }
            else if (player.health > 25) {
                healthColor = RGB(255, 255, 0);
            }
            else {
                healthColor = RGB(255, 0, 0);
            }

            HBRUSH healthBrush = CreateSolidBrush(healthColor);
            HPEN healthPen = CreatePen(PS_SOLID, 1, healthColor);

            SelectObject(hdc, healthPen);
            SelectObject(hdc, healthBrush);
            Rectangle(hdc, healthBarX, healthY,
                healthBarX + healthBarWidth, healthBarY + healthBarHeight);

            SelectObject(hdc, blackPen);
            SelectObject(hdc, blackBrush);
            DeleteObject(healthBrush);
            DeleteObject(healthPen);
        }

        // Name and health text
        if (config.espName) {
            std::string nameText = player.name;
            std::string healthText = std::to_string(player.health) + " HP";

            SIZE textSize;
            GetTextExtentPoint32A(hdc, nameText.c_str(), (int)nameText.length(), &textSize);
            TextOutA(hdc, (int)(x + width / 2 - textSize.cx / 2), (int)(y - 20),
                nameText.c_str(), (int)nameText.length());

            GetTextExtentPoint32A(hdc, healthText.c_str(), (int)healthText.length(), &textSize);
            TextOutA(hdc, (int)(x + width / 2 - textSize.cx / 2), (int)(y + height + 2),
                healthText.c_str(), (int)healthText.length());
        }
    }

    SelectObject(hdc, oldPen);
    SelectObject(hdc, oldBrush);
    SelectObject(hdc, oldFont);

    DeleteObject(font);
    DeleteObject(enemyPen);
    DeleteObject(teamPen);
    DeleteObject(textPen);
}

void ESP::ToggleTeammates() {
    showTeammates = !showTeammates;
}

bool ESP::ShouldShowTeammates() const {
    return showTeammates;
}