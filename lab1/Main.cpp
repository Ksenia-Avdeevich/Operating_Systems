#include <windows.h>
#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>
#include <cstdio>
#include <cstring>

using namespace std;

struct employee {
    int    num;
    char   name[10];
    double hours;
};

static int runAndWait(const char* cmdLine) {
    STARTUPINFOA        si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);

    char cmd[512];
    strncpy_s(cmd, cmdLine, sizeof(cmd) - 1);
    cmd[sizeof(cmd) - 1] = '\0';

    if (!CreateProcessA(
        NULL,
        cmd,
        NULL, NULL,
        FALSE,
        0,
        NULL, NULL,
        &si, &pi)) {
        cerr << "!CreateProcess failed: " << GetLastError() << "\n";
        return -1;
    }

    WaitForSingleObject(pi.hProcess, INFINITE);

    DWORD exitCode = 0;
    GetExitCodeProcess(pi.hProcess, &exitCode);

    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);

    return static_cast<int>(exitCode);
}

static void printBinaryFile(const char* filename) {
    HANDLE hFile = CreateFileA(
        filename, GENERIC_READ, FILE_SHARE_READ,
        NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if (hFile == INVALID_HANDLE_VALUE) {
        cerr << "!Could not open \"" << filename << "\"\n";
        return;
    }

    cout << "\nContents of file \"" << filename << "\"\n";
    printf("%-8s %-11s %-10s\n", "ID", "Name", "Hours");
    cout << "\n";

    employee emp;
    DWORD    bytesRead = 0;

    while (ReadFile(hFile, &emp, sizeof(employee), &bytesRead, NULL) && bytesRead == sizeof(employee)) {
        printf("%-8d %-11s %-10.2f\n", emp.num, emp.name, emp.hours);
    }

    CloseHandle(hFile);
    cout << "\n";
}

static void printTextFile(const char* filename) {
    ifstream in(filename);
    if (!in.is_open()) {
        cerr << "!Could not open \"" << filename << "\"\n";
        return;
    }

    cout << "\nReport \"" << filename << "\"\n";

    string line;
    while (getline(in, line))
        cout << line << "\n";

    cout << "\n";
}

int main() {
    cout << "Lab1 Process Creation (Win32)\n";

    char binFile[256];
    int  numRec = 0;

    cout << "Binary file name: ";
    cin >> binFile;

    while (numRec <= 0) {
        cout << "Number of records: ";
        cin >> numRec;
        if (numRec <= 0)
            cout << "!Enter a positive number.\n";
    }

    char cmdCreator[512];
    snprintf(cmdCreator, sizeof(cmdCreator),
        "Creator.exe %s %d", binFile, numRec);

    cout << "\nStarting Creator: " << cmdCreator << "\n\n";

    int exitCreator = runAndWait(cmdCreator);

    cout << "\nCreator finished with code " << exitCreator << "\n";

    if (exitCreator != 0) {
        cerr << "Creator returned non-zero code. Aborting!\n";
        return exitCreator;
    }

    printBinaryFile(binFile);

    char   reportFile[256];
    double rate = 0.0;

    cout << "Report file name: ";
    cin >> reportFile;

    while (rate <= 0.0) {
        cout << "Hourly rate: ";
        cin >> rate;
        if (rate <= 0.0)
            cout << "Rate must be > 0!\n";
    }

    char cmdReporter[512];
    snprintf(cmdReporter, sizeof(cmdReporter),
        "Reporter.exe %s %s %.4f", binFile, reportFile, rate);

    cout << "\nStarting Reporter: " << cmdReporter << "\n\n";

    int exitReporter = runAndWait(cmdReporter);

    cout << "\nReporter finished with code " << exitReporter << "\n";

    if (exitReporter != 0) {
        cerr << "Reporter returned non-zero code!\n";
        return exitReporter;
    }

    printTextFile(reportFile);

    cout << "Program finished successfully!!!\n";

    return 0;
}
