#include "Memory.h"
#include <iostream>

Memory::Memory() : process(NULL), clientBase(0), pid(0), gameWindow(NULL) {
    gameRect = { 0 };
}

Memory::~Memory() {
    if (process) {
        CloseHandle(process);
    }
}

bool Memory::Attach() {
    std::cout << "[Memory] Attaching to CS2..." << std::endl;

    if (!FindGameWindow()) {
        std::cout << "[Memory] Game window not found!" << std::endl;
        return false;
    }

    if (!FindProcessPID(L"cs2.exe")) {
        std::cout << "[Memory] Process cs2.exe not found!" << std::endl;
        return false;
    }

    std::cout << "[Memory] Found PID: " << pid << std::endl;

    process = OpenProcess(PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_QUERY_INFORMATION, FALSE, pid);
    if (!process) {
        std::cout << "[Memory] Cannot open process. Error: " << GetLastError() << std::endl;
        return false;
    }

    clientBase = GetModuleBase(L"client.dll");
    if (!clientBase) {
        std::cout << "[Memory] Cannot find client.dll" << std::endl;
        CloseHandle(process);
        return false;
    }

    std::cout << "[Memory] Client.dll base: 0x" << std::hex << clientBase << std::dec << std::endl;
    std::cout << "[Memory] Game window found!" << std::endl;

    return true;
}

bool Memory::FindGameWindow() {
    gameWindow = FindWindowW(NULL, L"Counter-Strike 2");
    if (!gameWindow) return false;

    GetClientRect(gameWindow, &gameRect);
    return true;
}

bool Memory::FindProcessPID(const wchar_t* processName) {
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) return false;

    PROCESSENTRY32W pe;
    pe.dwSize = sizeof(PROCESSENTRY32W);

    if (Process32FirstW(snapshot, &pe)) {
        do {
            if (_wcsicmp(pe.szExeFile, processName) == 0) {
                pid = pe.th32ProcessID;
                CloseHandle(snapshot);
                return true;
            }
        } while (Process32NextW(snapshot, &pe));
    }

    CloseHandle(snapshot);
    return false;
}

uintptr_t Memory::GetModuleBase(const wchar_t* moduleName) {
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
    if (snapshot == INVALID_HANDLE_VALUE) return 0;

    MODULEENTRY32W me;
    me.dwSize = sizeof(MODULEENTRY32W);

    if (Module32FirstW(snapshot, &me)) {
        do {
            if (_wcsicmp(me.szModule, moduleName) == 0) {
                CloseHandle(snapshot);
                return (uintptr_t)me.modBaseAddr;
            }
        } while (Module32NextW(snapshot, &me));
    }

    CloseHandle(snapshot);
    return 0;
}