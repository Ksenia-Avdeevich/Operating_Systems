#include <windows.h>
#include <iostream>
#include <iomanip>
#include <cassert>
#include <cmath>
#include <string>
using namespace std;

struct ThreadData {
    int* arr;
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
    data->minVal = mn; data->maxVal = mx;
    data->minIdx = mnIdx; data->maxIdx = mxIdx;
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
    return 0;
}

void runThreads(ThreadData& data) {
    DWORD id1, id2;
    HANDLE h1 = CreateThread(NULL, 0, MinMaxThread, &data, 0, &id1);
    HANDLE h2 = CreateThread(NULL, 0, AverageThread, &data, 0, &id2);
    WaitForSingleObject(h1, INFINITE);
    WaitForSingleObject(h2, INFINITE);
    CloseHandle(h1);
    CloseHandle(h2);
}

bool approxEqual(double a, double b, double eps = 1e-6) {
    return fabs(a - b) < eps;
}

void printResult(const string& name, bool passed) {
    cout << (passed ? "PASS " : "FAIL") << name << "\n";
}

void test_basic_min_max() {
    int arr[] = { 3, -1, 7, 2, 5 };
    ThreadData d = {}; d.arr = arr; d.size = 5;
    runThreads(d);
    assert(d.minVal == -1);
    assert(d.maxVal == 7);
    assert(d.minIdx == 1);
    assert(d.maxIdx == 2);
    printResult("basic_min_max", true);
}

void test_basic_average() {
    int arr[] = { 3, -1, 7, 2, 5 };
    ThreadData d = {}; d.arr = arr; d.size = 5;
    runThreads(d);
    assert(approxEqual(d.avgVal, 3.2));
    printResult("basic_average", true);
}

void test_single_element() {
    int arr[] = { 42 };
    ThreadData d = {}; d.arr = arr; d.size = 1;
    runThreads(d);
    assert(d.minVal == 42);
    assert(d.maxVal == 42);
    assert(d.minIdx == 0);
    assert(d.maxIdx == 0);
    assert(approxEqual(d.avgVal, 42.0));
    printResult("single_element", true);
}

void test_all_equal() {
    int arr[] = { 5, 5, 5, 5 };
    ThreadData d = {}; d.arr = arr; d.size = 4;
    runThreads(d);
    assert(d.minVal == 5);
    assert(d.maxVal == 5);
    assert(approxEqual(d.avgVal, 5.0));
    printResult("all_equal", true);
}

void test_negative_numbers() {
    int arr[] = { -10, -3, -7, -1, -5 };
    ThreadData d = {}; d.arr = arr; d.size = 5;
    runThreads(d);
    assert(d.minVal == -10);
    assert(d.maxVal == -1);
    assert(approxEqual(d.avgVal, -5.2));
    printResult("negative_numbers", true);
}

void test_two_elements() {
    int arr[] = { 10, 20 };
    ThreadData d = {}; d.arr = arr; d.size = 2;
    runThreads(d);
    assert(d.minVal == 10);
    assert(d.maxVal == 20);
    assert(approxEqual(d.avgVal, 15.0));
    printResult("two_elements", true);
}

void test_avg_replace_min_max() {
    int arr[] = { 1, 9, 3, 5, 2 };
    ThreadData d = {}; d.arr = arr; d.size = 5;
    runThreads(d);
    int avgRounded = static_cast<int>(d.avgVal + 0.5);
    arr[d.minIdx] = avgRounded;
    arr[d.maxIdx] = avgRounded;
    assert(arr[d.minIdx] == avgRounded);
    assert(arr[d.maxIdx] == avgRounded);
    printResult("avg_replace_min_max", true);
}

void test_sorted_ascending() {
    int arr[] = { 1, 2, 3, 4, 5 };
    ThreadData d = {}; d.arr = arr; d.size = 5;
    runThreads(d);
    assert(d.minVal == 1 && d.minIdx == 0);
    assert(d.maxVal == 5 && d.maxIdx == 4);
    assert(approxEqual(d.avgVal, 3.0));
    printResult("sorted_ascending", true);
}

void test_sorted_descending() {
    int arr[] = { 5, 4, 3, 2, 1 };
    ThreadData d = {}; d.arr = arr; d.size = 5;
    runThreads(d);
    assert(d.minVal == 1 && d.minIdx == 4);
    assert(d.maxVal == 5 && d.maxIdx == 0);
    printResult("sorted_descending", true);
}

void test_large_values() {
    int arr[] = { 1000000, -1000000, 0 };
    ThreadData d = {}; d.arr = arr; d.size = 3;
    runThreads(d);
    assert(d.minVal == -1000000);
    assert(d.maxVal == 1000000);
    assert(approxEqual(d.avgVal, 0.0));
    printResult("large_values", true);
}

int main() {
    cout << "Unit Tests - Lab2 Threads\n\n";

    test_basic_min_max();
    test_basic_average();
    test_single_element();
    test_all_equal();
    test_negative_numbers();
    test_two_elements();
    test_avg_replace_min_max();
    test_sorted_ascending();
    test_sorted_descending();
    test_large_values();

    cout << "\nAll tests passed.\n";
    return 0;
}