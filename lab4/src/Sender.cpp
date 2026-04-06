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

static WinHandle g_hFile;
static WinHandle g_hMutex;
static WinHandle g_hSemFull;
static WinHandle g_hSemEmpty;
static WinHandle g_hReadyEvent;

static void openSyncObjects() {
    g_hMutex = WinHandle::checked(
        OpenMutexA(SYNCHRONIZE | MUTEX_MODIFY_STATE, FALSE, MUTEX_NAME),
        "OpenMutex");

    g_hSemFull = WinHandle::checked(
        OpenSemaphoreA(SEMAPHORE_MODIFY_STATE | SYNCHRONIZE, FALSE, SEM_FULL_NAME),
        "OpenSemaphore(full)");

    g_hSemEmpty = WinHandle::checked(
        OpenSemaphoreA(SYNCHRONIZE, FALSE, SEM_EMPTY_NAME),
        "OpenSemaphore(empty)");

    g_hReadyEvent = WinHandle::checked(
        OpenEventA(EVENT_MODIFY_STATE, FALSE, READY_EVENT),
        "OpenEvent(ready)");
}

static void sendMessage(const string& text) {
    if (text.size() >= MAX_MSG_LEN)
        throw std::runtime_error("Message too long (max 19 chars)");

    if (WaitForSingleObject(g_hSemEmpty.get(), INFINITE) != WAIT_OBJECT_0)
        throw std::runtime_error("WaitForSingleObject(semEmpty) failed");

    if (WaitForSingleObject(g_hMutex.get(), INFINITE) != WAIT_OBJECT_0)
        throw std::runtime_error("WaitForSingleObject(mutex) failed");

    Message msg{};
    msg.valid = 1;
    std::strncpy(msg.text, text.c_str(), MAX_MSG_LEN - 1);

    SetFilePointer(g_hFile.get(), 0, nullptr, FILE_BEGIN);
    DWORD written = 0;
    WriteFile(g_hFile.get(), &msg, static_cast<DWORD>(sizeof(Message)), &written, nullptr);

    ReleaseMutex(g_hMutex.get());
    ReleaseSemaphore(g_hSemFull.get(), 1, nullptr);
}

int main(int argc, char* argv[]) {
    try {
        if (argc < 2)
            throw std::runtime_error("Usage: Sender <filename>");

        string fileName(argv[1]);
        cout << "=== Sender ===\n";

        g_hFile = WinHandle::checked(
            CreateFileA(fileName.c_str(),
                        GENERIC_READ | GENERIC_WRITE,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        nullptr, OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL, nullptr),
            "CreateFile(Sender)");

        openSyncObjects();

        SetEvent(g_hReadyEvent.get());
        cout << "[Sender] Ready signal sent.\n";

        while (true) {
            cout << "\nCommands: [s]end  [q]uit\n> ";
            char cmd{};
            cin >> cmd;
            if (cmd == 'q' || cmd == 'Q') break;
            if (cmd == 's' || cmd == 'S') {
                cout << "Enter message (max 19 chars): ";
                string text;
                cin.ignore();
                std::getline(cin, text);
                sendMessage(text);
                cout << "[Sender] Message sent.\n";
            }
        }
        cout << "[Sender] Exiting.\n";
    } catch (const std::exception& ex) {
        cerr << "[Sender] ERROR: " << ex.what() << "\n";
        return 1;
    }
    return 0;
}
