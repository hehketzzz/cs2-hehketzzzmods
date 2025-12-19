#define _CRT_SECURE_NO_WARNINGS

#include "Memory.h"
#include "ESP.h"
#include "Aimbot.h"
#include "GUI.h"
#include "Config.h"
#include "TriggerBot.h"
#include <thread>
#include <atomic>
#include <iostream>
#include "NoFlash.h"

// Для обработки сообщений окна
#include <Windows.h>

Config config;
std::atomic<bool> running(true);

int main() {
    // Для отладки в консоли
    AllocConsole();
    FILE* f;
    freopen_s(&f, "CONOUT$", "w", stdout);
    freopen_s(&f, "CONOUT$", "w", stderr);

    std::cout << "=== CS2 Cheat ===" << std::endl;
    std::cout << "Initializing..." << std::endl;

    // Инициализируем память
    Memory memory;
    if (!memory.Attach()) {
        std::cout << "Failed to attach to CS2!" << std::endl;
        MessageBoxA(NULL, "Failed to attach to CS2!", "Error", MB_OK);
        Sleep(3000);
        return 1;
    }

    std::cout << "Successfully attached to CS2!" << std::endl;

    // Инициализируем GUI
    GUI gui(config);
    if (!gui.Initialize()) {
        std::cout << "Failed to initialize GUI!" << std::endl;
        MessageBoxA(NULL, "Failed to initialize GUI!", "Error", MB_OK);
        return 1;
    }

    // Инициализируем ESP
    ESP esp(memory, config);
    if (!esp.Initialize()) {
        std::cout << "Failed to initialize ESP!" << std::endl;
        MessageBoxA(NULL, "Failed to initialize ESP!", "Error", MB_OK);
        return 1;
    }

    // Инициализируем Aimbot
    Aimbot aimbot(memory, config);

    std::cout << "All systems initialized!" << std::endl;
    std::cout << "Hotkeys:" << std::endl;
    std::cout << "  INSERT - Toggle Menu" << std::endl;
    std::cout << "  END - Exit" << std::endl;

    TriggerBot triggerbot(memory, config);
    NoFlash noflash(memory, config);
    // Запускаем потоки
    std::thread espThread([&]() { esp.Run(); });
    std::thread aimbotThread([&]() { aimbot.Run(); });
    std::thread triggerbotThread([&]() { triggerbot.Run(); });  // <-- ДОБАВЬТЕ ЭТО
    std::thread noflashThread([&]() { noflash.Start(); });

    // Главный цикл
    MSG msg = { 0 };
    while (running) {
        // Обрабатываем сообщения Windows
        while (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);

            if (msg.message == WM_QUIT) {
                running = false;
                break;
            }
        }

        if (!running) break;

        // Проверка выхода
        if (GetAsyncKeyState(VK_END) & 1) {
            std::cout << "Exit key pressed!" << std::endl;
            running = false;
            break;
        }

        // Переключение меню
        if (GetAsyncKeyState(VK_INSERT) & 1) {
            gui.ToggleMenu();
            Sleep(200);
        }

        // Рендер GUI
        gui.Render();

        // Небольшая задержка
        Sleep(10);
    }

    // Очистка
    running = false;

    if (espThread.joinable()) espThread.join();
    if (aimbotThread.joinable()) aimbotThread.join();

    esp.Cleanup();
    gui.Shutdown();

    std::cout << "Cleanup complete!" << std::endl;
    Sleep(1000);

    return 0;
}