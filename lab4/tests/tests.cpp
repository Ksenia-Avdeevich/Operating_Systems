#include <windows.h>
#include <cassert>
#include <cstring>
#include <iostream>
#include <string>
#include <stdexcept>

#include "shared.h"
#include "win_handle.h"

using std::cout;
using std::string;

// ---- helpers ----

static void test_message_struct_size() {
    static_assert(sizeof(Message) == MAX_MSG_LEN + 1,
                  "Message struct size must be MAX_MSG_LEN + 1");
    cout << "[PASS] test_message_struct_size\n";
}

static void test_message_text_fits() {
    Message m{};
    const char* text = "Hello";
    std::strncpy(m.text, text, MAX_MSG_LEN - 1);
    m.valid = 1;
    assert(std::strcmp(m.text, "Hello") == 0);
    assert(m.valid == 1);
    cout << "[PASS] test_message_text_fits\n";
}

static void test_message_text_truncated() {
    Message m{};
    // 25 chars – must be silently truncated to 19 + null
    const string longText(25, 'A');
    std::strncpy(m.text, longText.c_str(), MAX_MSG_LEN - 1);
    m.text[MAX_MSG_LEN - 1] = '\0';
    assert(std::strlen(m.text) == MAX_MSG_LEN - 1);
    cout << "[PASS] test_message_text_truncated\n";
}

static void test_win_handle_valid() {
    // A real event handle should be valid
    HANDLE h = CreateEventA(nullptr, FALSE, FALSE, nullptr);
    assert(h != nullptr && h != INVALID_HANDLE_VALUE);
    WinHandle wh(h);
    assert(wh.valid());
    cout << "[PASS] test_win_handle_valid\n";
}

static void test_win_handle_move() {
    HANDLE h = CreateEventA(nullptr, FALSE, FALSE, nullptr);
    WinHandle a(h);
    WinHandle b(std::move(a));
    assert(!a.valid());
    assert(b.valid());
    cout << "[PASS] test_win_handle_move\n";
}

static void test_win_handle_checked_throws() {
    bool threw = false;
    try {
        WinHandle::checked(nullptr, "test");
    } catch (const std::runtime_error&) {
        threw = true;
    }
    assert(threw);
    cout << "[PASS] test_win_handle_checked_throws\n";
}

static void test_semaphore_create_release_wait() {
    // Create semaphore with count 0, max 1
    WinHandle sem(CreateSemaphoreA(nullptr, 0, 1, nullptr));
    assert(sem.valid());

    // Should time out immediately (count = 0)
    DWORD res = WaitForSingleObject(sem.get(), 0);
    assert(res == WAIT_TIMEOUT);

    // Release once (count becomes 1)
    BOOL ok = ReleaseSemaphore(sem.get(), 1, nullptr);
    assert(ok);

    // Now wait should succeed (count drops back to 0)
    res = WaitForSingleObject(sem.get(), 0);
    assert(res == WAIT_OBJECT_0);

    cout << "[PASS] test_semaphore_create_release_wait\n";
}

static void test_mutex_create_lock_unlock() {
    WinHandle mtx(CreateMutexA(nullptr, FALSE, nullptr));
    assert(mtx.valid());

    DWORD res = WaitForSingleObject(mtx.get(), 0);
    assert(res == WAIT_OBJECT_0);  // lock acquired

    BOOL ok = ReleaseMutex(mtx.get());
    assert(ok);

    cout << "[PASS] test_mutex_create_lock_unlock\n";
}

static void test_event_set_wait_reset() {
    WinHandle ev(CreateEventA(nullptr, TRUE, FALSE, nullptr));  // manual reset
    assert(ev.valid());

    // Initially nonsignaled
    assert(WaitForSingleObject(ev.get(), 0) == WAIT_TIMEOUT);

    SetEvent(ev.get());
    assert(WaitForSingleObject(ev.get(), 0) == WAIT_OBJECT_0);

    ResetEvent(ev.get());
    assert(WaitForSingleObject(ev.get(), 0) == WAIT_TIMEOUT);

    cout << "[PASS] test_event_set_wait_reset\n";
}

static void test_file_write_read() {
    const char* fname = "test_lab4_tmp.bin";
    // Write a Message
    Message out{};
    std::strncpy(out.text, "TestMsg", MAX_MSG_LEN - 1);
    out.valid = 1;

    {
        WinHandle f(CreateFileA(fname,
            GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr));
        assert(f.valid());
        DWORD w = 0;
        assert(WriteFile(f.get(), &out, static_cast<DWORD>(sizeof(Message)), &w, nullptr));
        assert(w == sizeof(Message));
    }

    // Read it back
    Message in{};
    {
        WinHandle f(CreateFileA(fname,
            GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr));
        assert(f.valid());
        DWORD r = 0;
        assert(ReadFile(f.get(), &in, static_cast<DWORD>(sizeof(Message)), &r, nullptr));
        assert(r == sizeof(Message));
    }

    assert(std::strcmp(in.text, "TestMsg") == 0);
    assert(in.valid == 1);
    DeleteFileA(fname);
    cout << "[PASS] test_file_write_read\n";
}

// ---- runner ----

int main() {
    cout << "=== Lab4 Unit Tests ===\n\n";

    test_message_struct_size();
    test_message_text_fits();
    test_message_text_truncated();
    test_win_handle_valid();
    test_win_handle_move();
    test_win_handle_checked_throws();
    test_semaphore_create_release_wait();
    test_mutex_create_lock_unlock();
    test_event_set_wait_reset();
    test_file_write_read();

    cout << "\nAll tests passed.\n";
    return 0;
}
