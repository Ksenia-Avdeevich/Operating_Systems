#pragma once
#include <windows.h>
#include <stdexcept>
#include <string>

class WinHandle {
public:
    explicit WinHandle(HANDLE h = INVALID_HANDLE_VALUE) : h_(h) {}

    WinHandle(const WinHandle&)            = delete;
    WinHandle& operator=(const WinHandle&) = delete;

    WinHandle(WinHandle&& o) noexcept : h_(o.h_) { o.h_ = INVALID_HANDLE_VALUE; }
    WinHandle& operator=(WinHandle&& o) noexcept {
        if (this != &o) { close(); h_ = o.h_; o.h_ = INVALID_HANDLE_VALUE; }
        return *this;
    }

    ~WinHandle() { close(); }

    HANDLE get()  const noexcept { return h_; }
    bool   valid()const noexcept { return h_ != INVALID_HANDLE_VALUE && h_ != nullptr; }
    operator HANDLE() const noexcept { return h_; }

    static WinHandle checked(HANDLE h, const std::string& ctx) {
        if (h == INVALID_HANDLE_VALUE || h == nullptr) {
            throw std::runtime_error(ctx + " failed, error=" + std::to_string(GetLastError()));
        }
        return WinHandle(h);
    }

private:
    void close() noexcept {
        if (valid()) { CloseHandle(h_); h_ = INVALID_HANDLE_VALUE; }
    }
    HANDLE h_;
};
