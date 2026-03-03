#include <windows.h>
#include <iostream>
#include <iomanip>
#include <string>


using namespace std;

struct ThreadData {
    int*   arr;
    int    size;
    int    minVal;
    int    maxVal;
    int    minIdx;
    int    maxIdx;
    double avgVal;
};

DWORD WINAPI MinMaxThread(LPVOID param) {
    ThreadData* data = reinterpret_cast<ThreadData*>(param);

    int mn = data->arr[0], mx = data->arr[0];
    int mnIdx = 0, mxIdx = 0;

    for (int i = 1; i < data->size; ++i) {
        if (data->arr[i] < mn) { mn = data->arr[i]; mnIdx = i; }
        if (data->arr[i] > mx) { mx = data->arr[i]; mxIdx = i; }
        Sleep(7);
    }

    data->minVal = mn;
    data->maxVal = mx;
    data->minIdx = mnIdx;
    data->maxIdx = mxIdx;

    cout << "[min_max] min = " << mn << " (index " << mnIdx << ")"
         << "  max = " << mx << " (index " << mxIdx << ")\n";

    return 0;
}

DWORD WINAPI AverageThread(LPVOID param) {
    ThreadData* data = reinterpret_cast<ThreadData*>(param);

    long long sum = 0;
    for (int i = 0; i < data->size; ++i) {
        sum += data->arr[i];
        Sleep(12);
    }

    data->avgVal = static_cast<double>(sum) / data->size;

    cout << "[average] avg = " << fixed << setprecision(2) << data->avgVal << "\n";

    return 0;
}

void printArray(const int* arr, int n, const string& label) {
    cout << label << ": [ ";
    for (int i = 0; i < n; ++i) cout << arr[i] << (i + 1 < n ? ", " : "");
    cout << " ]\n";
}

int main() {
    int n = 0;
    cout << "Enter number of elements: ";
    while (!(cin >> n) || n <= 0) {
        cin.clear(); cin.ignore(1000, '\n');
        cout << "Invalid input. Enter a positive integer: ";
    }

    int* arr = new int[n];
    cout << "Enter " << n << " integers: ";
    for (int i = 0; i < n; ++i) cin >> arr[i];

    printArray(arr, n, "Original array");

    ThreadData data = {};
    data.arr  = arr;
    data.size = n;

    DWORD idMinMax, idAvg;
    HANDLE hMinMax  = CreateThread(NULL, 0, MinMaxThread,  &data, 0, &idMinMax);
    HANDLE hAverage = CreateThread(NULL, 0, AverageThread, &data, 0, &idAvg);

    if (hMinMax == NULL || hAverage == NULL) {
        cerr << "CreateThread failed. Error: " << GetLastError() << "\n";
        delete[] arr;
        return 1;
    }

    WaitForSingleObject(hMinMax,  INFINITE);
    WaitForSingleObject(hAverage, INFINITE);

    CloseHandle(hMinMax);
    CloseHandle(hAverage);

    int avgRounded = static_cast<int>(data.avgVal + 0.5);
    arr[data.minIdx] = avgRounded;
    arr[data.maxIdx] = avgRounded;

    printArray(arr, n, "Modified array");

    cout << "\nmin = " << data.minVal << " (index " << data.minIdx << ")\n";
    cout << "max = " << data.maxVal << " (index " << data.maxIdx << ")\n";
    cout << "avg = " << fixed << setprecision(4) << data.avgVal << "\n";

    delete[] arr;
    return 0;
}
