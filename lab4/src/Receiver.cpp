#include <windows.h>
#include <iostream>
#include <string>
#include <stdexcept>
#include <cstring>

#include "shared.h"
#include "win_handle.h"

using std::cout;
using std::cin;
using std::cerr;
using std::string;

static string  g_fileName;
static WinHandle g_hFile;
static WinHandle g_hMutex;
static WinHandle g_hSemFull;
static WinHandle g_hSemEmpty;
static WinHandle g_hReadyEvent;

static void createSyncObjects() {
    g_hMutex = WinHandle::checked(
        CreateMutexA(nullptr, FALSE, MUTEX_NAME), "CreateMutex");

    g_hSemFull = WinHandle::checked(
        CreateSemaphoreA(nullptr, 0, 1, SEM_FULL_NAME), "CreateSemaphore(full)");

    g_hSemEmpty = WinHandle::checked(
        CreateSemaphoreA(nullptr, 1, 1, SEM_EMPTY_NAME), "CreateSemaphore(empty)");

    g_hReadyEvent = WinHandle::checked(
        CreateEventA(nullptr, FALSE, FALSE, READY_EVENT), "CreateEvent(ready)");
}

static void createBinaryFile() {
    Message emptyMsg{};
    emptyMsg.valid = 0;

    g_hFile = WinHandle::checked(
        CreateFileA(g_fileName.c_str(),
                    GENERIC_READ | GENERIC_WRITE,
                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                    nullptr, CREATE_ALWAYS,
                    FILE_ATTRIBUTE_NORMAL, nullptr),
        "CreateFile");

    DWORD written = 0;
    if (!WriteFile(g_hFile.get(), &emptyMsg, static_cast<DWORD>(sizeof(Message)), &written, nullptr))
        throw std::runtime_error(string("WriteFile failed, error=") + std::to_string(GetLastError()));
}

static void launchSender() {
    string senderPath = "Sender.exe";
    string cmdLine    = senderPath + " " + g_fileName;

    STARTUPINFOA si{};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi{};

    if (!CreateProcessA(nullptr,
                        cmdLine.data(),
                        nullptr, nullptr, FALSE,
                        CREATE_NEW_CONSOLE,
                        nullptr, nullptr, &si, &pi))
        throw std::runtime_error(string("CreateProcess failed, error=") + std::to_string(GetLastError()));

    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    cout << "[Receiver] Sender process launched.\n";
}

static Message readMessage() {
    if (WaitForSingleObject(g_hSemFull.get(), INFINITE) != WAIT_OBJECT_0)
        throw std::runtime_error("WaitForSingleObject(semFull) failed");

    if (WaitForSingleObject(g_hMutex.get(), INFINITE) != WAIT_OBJECT_0)
        throw std::runtime_error("WaitForSingleObject(mutex) failed");

    SetFilePointer(g_hFile.get(), 0, nullptr, FILE_BEGIN);
    Message msg{};
    DWORD read = 0;
    ReadFile(g_hFile.get(), &msg, static_cast<DWORD>(sizeof(Message)), &read, nullptr);

    msg.valid = 0;
    SetFilePointer(g_hFile.get(), 0, nullptr, FILE_BEGIN);
    DWORD written = 0;
    WriteFile(g_hFile.get(), &msg, static_cast<DWORD>(sizeof(Message)), &written, nullptr);

    ReleaseMutex(g_hMutex.get());
    ReleaseSemaphore(g_hSemEmpty.get(), 1, nullptr);
    return msg;
}

int main() {
    try {
        cout << "=== Receiver ===\n";
        cout << "Enter binary file name: ";
        cin >> g_fileName;

        createSyncObjects();
        createBinaryFile();
        launchSender();

        cout << "[Receiver] Waiting for Sender ready signal...\n";
        if (WaitForSingleObject(g_hReadyEvent.get(), 10000) != WAIT_OBJECT_0)
            throw std::runtime_error("Sender did not signal ready in time");
        cout << "[Receiver] Sender is ready.\n";

        while (true) {
            cout << "\nCommands: [r]ead  [q]uit\n> ";
            char cmd{};
            cin >> cmd;
            if (cmd == 'q' || cmd == 'Q') break;
            if (cmd == 'r' || cmd == 'R') {
                cout << "[Receiver] Waiting for message...\n";
                Message msg = readMessage();
                cout << "[Receiver] Got: " << msg.text << "\n";
            }
        }
        cout << "[Receiver] Exiting.\n";
    } catch (const std::exception& ex) {
        cerr << "[Receiver] ERROR: " << ex.what() << "\n";
        return 1;
    }
    return 0;
}
