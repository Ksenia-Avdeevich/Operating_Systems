#define NOMINMAX
#include <windows.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <stdexcept>
#include "../common/employee.h"
#include "../common/pipe_names.h"

using std::cin;
using std::cout;
using std::cerr;
using std::string;
using std::vector;
using std::fstream;
using std::ifstream;
using std::ofstream;

static string g_filename;

static void throwLastError(const string& ctx) {
    DWORD err = GetLastError();
    char buf[256];
    FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr, err, 0, buf, sizeof(buf), nullptr);
    throw std::runtime_error(ctx + ": " + buf);
}

static void writeFile(const string& path, const vector<Employee>& employees) {
    ofstream f(path, std::ios::binary | std::ios::trunc);
    if (!f) throw std::runtime_error("Cannot open file for writing: " + path);
    for (const auto& e : employees)
        f.write(reinterpret_cast<const char*>(&e), sizeof(Employee));
}

static vector<Employee> readFile(const string& path) {
    ifstream f(path, std::ios::binary);
    if (!f) throw std::runtime_error("Cannot open file for reading: " + path);
    vector<Employee> result;
    Employee e;
    while (f.read(reinterpret_cast<char*>(&e), sizeof(Employee)))
        result.push_back(e);
    return result;
}

static void printEmployees(const vector<Employee>& emps) {
    cout << "\n--- Employees ---\n";
    for (const auto& e : emps)
        cout << "  num=" << e.num << "  name=" << e.name << "  hours=" << e.hours << '\n';
    cout << "-----------------\n\n";
}

static int findByKey(const vector<Employee>& emps, int key) {
    for (int i = 0; i < static_cast<int>(emps.size()); ++i)
        if (emps[i].num == key) return i;
    return -1;
}

static HANDLE createPipe(const char* name) {
    HANDLE h = CreateNamedPipeA(name,
        PIPE_ACCESS_DUPLEX,
        PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
        1, PIPE_BUF_SIZE, PIPE_BUF_SIZE, 0, nullptr);
    if (h == INVALID_HANDLE_VALUE) throwLastError(string("CreateNamedPipe ") + name);
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

int main() {
    try {
        cout << "Enter filename: ";
        cin >> g_filename;

        {
            ifstream check(g_filename, std::ios::binary);
            if (check) {
                cout << "File already exists. Overwrite? (y/n): ";
                char c; cin >> c;
                if (c != 'y' && c != 'Y') {
                    cout << "Aborted.\n";
                    return 0;
                }
            }
        }

        int n = 0;
        do {
            cout << "Number of employees (>0): ";
            if (!(cin >> n) || n <= 0) {
                cin.clear();
                cin.ignore(1024, '\n');
                cout << "Invalid input.\n";
                n = 0;
            }
        } while (n <= 0);

        vector<Employee> employees(static_cast<size_t>(n));
        for (int i = 0; i < n; ++i) {
            cout << "Employee " << i + 1 << " — num, name, hours: ";
            cin >> employees[i].num;
            char tmp[64];
            cin >> tmp;
            std::strncpy(employees[i].name, tmp, 9);
            employees[i].name[9] = '\0';
            double h = 0;
            if (!(cin >> h) || h < 0) throw std::runtime_error("Invalid hours value");
            employees[i].hours = h;
        }

        writeFile(g_filename, employees);
        cout << "File created.\n";
        printEmployees(employees);

        HANDLE hReq  = createPipe(PIPE_REQUEST);
        HANDLE hResp = createPipe(PIPE_RESPONSE);

        cout << "Server waiting for client...\n";

        if (!ConnectNamedPipe(hReq, nullptr) && GetLastError() != ERROR_PIPE_CONNECTED)
            throwLastError("ConnectNamedPipe request");
        if (!ConnectNamedPipe(hResp, nullptr) && GetLastError() != ERROR_PIPE_CONNECTED)
            throwLastError("ConnectNamedPipe response");

        cout << "Client connected.\n";

        bool running = true;
        while (running) {
            Request req{};
            pipeRead(hReq, &req, sizeof(req));

            Response resp{};
            resp.success = false;

            if (req.type == RequestType::Exit) {
                resp.success = true;
                std::strncpy(resp.message, "Bye", sizeof(resp.message) - 1);
                pipeWrite(hResp, &resp, sizeof(resp));
                running = false;
                break;
            }

            int idx = findByKey(employees, req.key);
            if (idx < 0) {
                std::strncpy(resp.message, "Record not found", sizeof(resp.message) - 1);
                pipeWrite(hResp, &resp, sizeof(resp));
                continue;
            }

            if (req.type == RequestType::Read) {
                resp.success = true;
                resp.data = employees[static_cast<size_t>(idx)];
                std::strncpy(resp.message, "OK", sizeof(resp.message) - 1);
                pipeWrite(hResp, &resp, sizeof(resp));

                Request ack{};
                pipeRead(hReq, &ack, sizeof(ack));

            } else if (req.type == RequestType::Modify) {
                resp.success = true;
                resp.data = employees[static_cast<size_t>(idx)];
                std::strncpy(resp.message, "OK", sizeof(resp.message) - 1);
                pipeWrite(hResp, &resp, sizeof(resp));

                Request update{};
                pipeRead(hReq, &update, sizeof(update));
                employees[static_cast<size_t>(idx)] = update.data;
                writeFile(g_filename, employees);

                Response ack{};
                ack.success = true;
                std::strncpy(ack.message, "Updated", sizeof(ack.message) - 1);
                pipeWrite(hResp, &ack, sizeof(ack));
            }
        }

        DisconnectNamedPipe(hResp);
        DisconnectNamedPipe(hReq);
        CloseHandle(hResp);
        CloseHandle(hReq);

        employees = readFile(g_filename);
        cout << "\nFinal file contents:\n";
        printEmployees(employees);

        cout << "Press Enter to exit...\n";
        cin.ignore(1024, '\n');
        cin.get();

    } catch (const std::exception& ex) {
        cerr << "Server error: " << ex.what() << '\n';
        return 1;
    }
    return 0;
}
