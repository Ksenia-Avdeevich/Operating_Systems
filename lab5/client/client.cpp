#define NOMINMAX
#include <windows.h>
#include <iostream>
#include <string>
#include <stdexcept>
#include "../common/employee.h"
#include "../common/pipe_names.h"

using std::cin;
using std::cout;
using std::cerr;
using std::string;

static void throwLastError(const string& ctx) {
    DWORD err = GetLastError();
    char buf[256];
    FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr, err, 0, buf, sizeof(buf), nullptr);
    throw std::runtime_error(ctx + ": " + buf);
}

static HANDLE openPipe(const char* name) {
    HANDLE h = INVALID_HANDLE_VALUE;
    while (true) {
        h = CreateFileA(name, GENERIC_READ | GENERIC_WRITE, 0,
            nullptr, OPEN_EXISTING, 0, nullptr);
        if (h != INVALID_HANDLE_VALUE) break;
        if (GetLastError() != ERROR_PIPE_BUSY)
            throwLastError(string("CreateFile ") + name);
        if (!WaitNamedPipeA(name, 5000))
            throwLastError(string("WaitNamedPipe ") + name);
    }
    DWORD mode = PIPE_READMODE_MESSAGE;
    if (!SetNamedPipeHandleState(h, &mode, nullptr, nullptr))
        throwLastError("SetNamedPipeHandleState");
    return h;
}

static void pipeWrite(HANDLE h, const void* buf, DWORD sz) {
    DWORD written = 0;
    if (!WriteFile(h, buf, sz, &written, nullptr) || written != sz)
        throwLastError("WriteFile");
}

static void pipeRead(HANDLE h, void* buf, DWORD sz) {
    DWORD read = 0;
    if (!ReadFile(h, buf, sz, &read, nullptr) || read != sz)
        throwLastError("ReadFile");
}

static void printEmployee(const Employee& e) {
    cout << "  num=" << e.num << "  name=" << e.name << "  hours=" << e.hours << '\n';
}

int main() {
    try {
        HANDLE hReq  = openPipe(PIPE_REQUEST);
        HANDLE hResp = openPipe(PIPE_RESPONSE);

        bool running = true;
        while (running) {
            cout << "\n1) Read  2) Modify  3) Exit\nChoice: ";
            int choice = 0;
            if (!(cin >> choice)) {
                cin.clear(); cin.ignore(1024, '\n');
                cout << "Invalid input.\n";
                continue;
            }

            if (choice == 3) {
                Request req{};
                req.type = RequestType::Exit;
                pipeWrite(hReq, &req, sizeof(req));
                Response resp{};
                pipeRead(hResp, &resp, sizeof(resp));
                cout << "Server: " << resp.message << '\n';
                running = false;
                break;
            }

            if (choice != 1 && choice != 2) { cout << "Unknown option.\n"; continue; }

            int key = 0;
            cout << "Enter employee ID: ";
            if (!(cin >> key)) {
                cin.clear(); cin.ignore(1024, '\n');
                cout << "Invalid ID.\n";
                continue;
            }

            if (choice == 1) {
                Request req{};
                req.type = RequestType::Read;
                req.key  = key;
                pipeWrite(hReq, &req, sizeof(req));

                Response resp{};
                pipeRead(hResp, &resp, sizeof(resp));
                if (!resp.success) { cout << "Error: " << resp.message << '\n'; continue; }
                cout << "Record:\n";
                printEmployee(resp.data);

                cout << "Press Enter to release record...";
                cin.ignore(1024, '\n'); cin.get();

                Request ack{};
                ack.type = RequestType::Read;
                ack.key  = key;
                pipeWrite(hReq, &ack, sizeof(ack));

            } else {
                Request req{};
                req.type = RequestType::Modify;
                req.key  = key;
                pipeWrite(hReq, &req, sizeof(req));

                Response resp{};
                pipeRead(hResp, &resp, sizeof(resp));
                if (!resp.success) { cout << "Error: " << resp.message << '\n'; continue; }
                cout << "Current record:\n";
                printEmployee(resp.data);

                Employee updated = resp.data;
                cout << "New name (max 9 chars): ";
                char tmp[64];
                cin >> tmp;
                std::strncpy(updated.name, tmp, 9);
                updated.name[9] = '\0';
                cout << "New hours: ";
                double h = 0;
                if (!(cin >> h) || h < 0) {
                    cin.clear(); cin.ignore(1024, '\n');
                    cout << "Invalid hours, keeping old value.\n";
                    h = updated.hours;
                }
                updated.hours = h;

                Request update{};
                update.type = RequestType::Modify;
                update.key  = key;
                update.data = updated;
                pipeWrite(hReq, &update, sizeof(update));

                Response ack{};
                pipeRead(hResp, &ack, sizeof(ack));
                cout << "Server: " << ack.message << '\n';
            }
        }

        CloseHandle(hResp);
        CloseHandle(hReq);

    } catch (const std::exception& ex) {
        cerr << "Client error: " << ex.what() << '\n';
        return 1;
    }
    return 0;
}
