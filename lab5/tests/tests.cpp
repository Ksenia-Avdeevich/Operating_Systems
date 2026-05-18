#define NOMINMAX
#include <gtest/gtest.h>
#include <windows.h>
#include <fstream>
#include <vector>
#include <string>
#include <thread>
#include <stdexcept>
#include "../common/employee.h"
#include "../common/pipe_names.h"

using std::string;
using std::vector;
using std::ofstream;
using std::ifstream;

static const string TEST_FILE = "test_employees.bin";

static void writeEmployees(const string& path, const vector<Employee>& emps) {
    ofstream f(path, std::ios::binary | std::ios::trunc);
    ASSERT_TRUE(f.is_open());
    for (const auto& e : emps)
        f.write(reinterpret_cast<const char*>(&e), sizeof(Employee));
}

static vector<Employee> readEmployees(const string& path) {
    ifstream f(path, std::ios::binary);
    vector<Employee> result;
    Employee e;
    while (f.read(reinterpret_cast<char*>(&e), sizeof(Employee)))
        result.push_back(e);
    return result;
}

static Employee makeEmployee(int num, const char* name, double hours) {
    Employee e{};
    e.num   = num;
    std::strncpy(e.name, name, 9);
    e.name[9] = '\0';
    e.hours = hours;
    return e;
}

TEST(EmployeeFile, WriteAndReadBack) {
    vector<Employee> orig = {
        makeEmployee(1, "Alice", 40.0),
        makeEmployee(2, "Bob",   35.5),
        makeEmployee(3, "Carol", 20.0)
    };
    writeEmployees(TEST_FILE, orig);
    auto loaded = readEmployees(TEST_FILE);
    ASSERT_EQ(loaded.size(), orig.size());
    for (size_t i = 0; i < orig.size(); ++i) {
        EXPECT_EQ(loaded[i].num,   orig[i].num);
        EXPECT_STREQ(loaded[i].name, orig[i].name);
        EXPECT_DOUBLE_EQ(loaded[i].hours, orig[i].hours);
    }
    std::remove(TEST_FILE.c_str());
}

TEST(EmployeeFile, FindByKey) {
    vector<Employee> emps = {
        makeEmployee(10, "Dave",  10.0),
        makeEmployee(20, "Eve",   20.0),
        makeEmployee(30, "Frank", 30.0)
    };
    auto findByKey = [&](int key) -> int {
        for (int i = 0; i < static_cast<int>(emps.size()); ++i)
            if (emps[i].num == key) return i;
        return -1;
    };
    EXPECT_EQ(findByKey(10), 0);
    EXPECT_EQ(findByKey(20), 1);
    EXPECT_EQ(findByKey(30), 2);
    EXPECT_EQ(findByKey(99), -1);
}

TEST(EmployeeFile, ModifyAndPersist) {
    vector<Employee> emps = { makeEmployee(5, "Grace", 15.0) };
    writeEmployees(TEST_FILE, emps);

    auto loaded = readEmployees(TEST_FILE);
    ASSERT_EQ(loaded.size(), 1u);
    std::strncpy(loaded[0].name, "Modified", 9);
    loaded[0].hours = 99.0;
    writeEmployees(TEST_FILE, loaded);

    auto reloaded = readEmployees(TEST_FILE);
    ASSERT_EQ(reloaded.size(), 1u);
    EXPECT_STREQ(reloaded[0].name, "Modified");
    EXPECT_DOUBLE_EQ(reloaded[0].hours, 99.0);
    std::remove(TEST_FILE.c_str());
}

TEST(EmployeeFile, NameTruncatesAt9) {
    Employee e = makeEmployee(1, "ABCDEFGHIJK", 0);
    EXPECT_EQ(std::strlen(e.name), 9u);
    EXPECT_EQ(e.name[9], '\0');
}

TEST(EmployeeStructSize, PackedLayout) {
    EXPECT_EQ(sizeof(Employee), sizeof(int) + 10 + sizeof(double));
}

TEST(RequestResponse, TypesDistinct) {
    EXPECT_NE(static_cast<int>(RequestType::Read),   static_cast<int>(RequestType::Modify));
    EXPECT_NE(static_cast<int>(RequestType::Modify), static_cast<int>(RequestType::Exit));
    EXPECT_NE(static_cast<int>(RequestType::Read),   static_cast<int>(RequestType::Exit));
}

constexpr const char* TEST_PIPE = "\\\\.\\pipe\\test_employee_pipe";

static HANDLE serverPipe = INVALID_HANDLE_VALUE;
static HANDLE clientPipe = INVALID_HANDLE_VALUE;

static void serverThread() {
    serverPipe = CreateNamedPipeA(TEST_PIPE,
        PIPE_ACCESS_DUPLEX,
        PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
        1, PIPE_BUF_SIZE, PIPE_BUF_SIZE, 0, nullptr);
    ConnectNamedPipe(serverPipe, nullptr);
}

TEST(PipeCommunication, RequestResponseRoundTrip) {
    std::thread srv(serverThread);
    Sleep(100);

    clientPipe = CreateFileA(TEST_PIPE, GENERIC_READ | GENERIC_WRITE,
        0, nullptr, OPEN_EXISTING, 0, nullptr);
    DWORD mode = PIPE_READMODE_MESSAGE;
    SetNamedPipeHandleState(clientPipe, &mode, nullptr, nullptr);

    Request req{};
    req.type = RequestType::Read;
    req.key  = 42;
    DWORD written = 0;
    WriteFile(clientPipe, &req, sizeof(req), &written, nullptr);
    EXPECT_EQ(written, static_cast<DWORD>(sizeof(req)));

    Request received{};
    DWORD read = 0;
    ReadFile(serverPipe, &received, sizeof(received), &read, nullptr);
    EXPECT_EQ(read, static_cast<DWORD>(sizeof(received)));
    EXPECT_EQ(received.type, RequestType::Read);
    EXPECT_EQ(received.key, 42);

    srv.join();
    DisconnectNamedPipe(serverPipe);
    CloseHandle(serverPipe);
    CloseHandle(clientPipe);
}
