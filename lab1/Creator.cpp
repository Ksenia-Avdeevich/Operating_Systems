#include <windows.h>
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <cstdio>

using namespace std;

struct employee {
    int    num;
    char   name[10];
    double hours;
};

int main(int argc, char* argv[]) {
    if (argc != 3) {
        cerr << "Usage: Creator.exe <filename> <num_records>\n";
        return 1;
    }

    const char* filename = argv[1];
    int         numRec = atoi(argv[2]);

    if (numRec <= 0) {
        cerr << "Error: number of records must be > 0.\n";
        return 2;
    }

    HANDLE hFile = CreateFileA(
        filename,
        GENERIC_WRITE,
        0,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (hFile == INVALID_HANDLE_VALUE) {
        cerr << "CreateFile error: " << GetLastError() << "\n";
        return 3;
    }

    int basePid = static_cast<int>(GetCurrentProcessId()) * 1000;

    cout << "Enter data for " << numRec << " employee(s):\n\n";

    for (int i = 0; i < numRec; ++i) {
        employee emp;
        emp.num = basePid + i + 1;

        char tmpName[64] = {};
        bool validName = false;
        while (!validName) {
            cout << "Record [" << (i + 1) << "/" << numRec << "]\n";
            cout << "  Employee ID: " << emp.num << "\n";
            cout << "  Name (max 9 chars): ";
            cin >> tmpName;

            if (strlen(tmpName) == 0) {
                cout << "Name cannot be empty!\n";
            }
            else if (strlen(tmpName) > 9) {
                cout << "Name too long, will be trimmed to 9 chars!\n";
                tmpName[9] = '\0';
                validName = true;
            }
            else {
                validName = true;
            }
        }
        strncpy_s(emp.name, tmpName, 9);
        emp.name[9] = '\0';

        bool validHours = false;
        while (!validHours) {
            cout << "  Hours worked: ";
            if (cin >> emp.hours) {
                if (emp.hours < 0.0)
                    cout << "Hours cannot be negative!\n";
                else
                    validHours = true;
            }
            else {
                cin.clear();
                cin.ignore(256, '\n');
                cout << "Invalid input, enter a number!\n";
            }
        }

        DWORD written = 0;
        if (!WriteFile(hFile, &emp, sizeof(employee), &written, NULL) || written != sizeof(employee)) {
            cerr << "Write error: " << GetLastError() << "\n";
            CloseHandle(hFile);
            return 4;
        }

        cout << "Record saved)\n\n";
    }

    CloseHandle(hFile);
    cout << "File \"" << filename << "\" created (" << numRec << " record(s))\n";
    return 0;
}