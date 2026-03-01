#include <windows.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <cstdlib>
#include <cstdio>
#include <cstring>

using namespace std;

struct employee {
    int    num;
    char   name[10];
    double hours;
};

bool cmpByName(const employee& a, const employee& b) {
    return _stricmp(a.name, b.name) < 0;
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        cerr << "Usage: Reporter.exe <bin_file> <report_file> <hourly_rate>\n";
        return 1;
    }

    const char* srcFile = argv[1];
    const char* reportFile = argv[2];
    double      hourlyRate = atof(argv[3]);

    if (hourlyRate <= 0.0) {
        cerr << "Error: hourly rate must be > 0!\n";
        return 2;
    }

    HANDLE hSrc = CreateFileA(
        srcFile,
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (hSrc == INVALID_HANDLE_VALUE) {
        cerr << "Error opening file \"" << srcFile << "\": " << GetLastError() << "\n";
        return 3;
    }

    vector<employee> employees;
    employee emp;
    DWORD    bytesRead = 0;

    while (ReadFile(hSrc, &emp, sizeof(employee), &bytesRead, NULL) && bytesRead == sizeof(employee)) {
        employees.push_back(emp);
    }
    CloseHandle(hSrc);

    if (employees.empty()) {
        cerr << "File \"" << srcFile << "\" is empty or corrupted.\n";
        return 4;
    }

    sort(employees.begin(), employees.end(), cmpByName);

    ofstream out(reportFile);
    if (!out.is_open()) {
        cerr << "Could not create report file \"" << reportFile << "\".\n";
        return 5;
    }

    out << "\n";
    out << "Report for file \"" << srcFile << "\"\n";
    out << "Hourly rate: " << hourlyRate << "\n";
    out << "\n";

    out << left;
    out.width(8);  out << "ID";
    out.width(12); out << "Name";
    out.width(10); out << "Hours";
    out << "Salary\n";
    out << "\n";

    double totalSalary = 0.0;
    char   line[256];

    for (const auto& e : employees) {
        double salary = e.hours * hourlyRate;
        totalSalary += salary;

        snprintf(line, sizeof(line),
            "%-8d %-11s %-10.2f %.2f\n",
            e.num, e.name, e.hours, salary);
        out << line;
    }

    out << "\n";
    snprintf(line, sizeof(line),
        "Total employees: %d   Total payroll: %.2f\n",
        static_cast<int>(employees.size()), totalSalary);
    out << line;
    out << "\n";

    out.close();

    cout << "Reporter: report written to \"" << reportFile << "\")\n";
    return 0;
}