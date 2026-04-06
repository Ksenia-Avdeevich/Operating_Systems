#pragma once
#include <windows.h>
#include <cstdint>

static constexpr DWORD  MAX_MSG_LEN      = 20;
static constexpr char   MUTEX_NAME[]     = "Lab4_FileMutex";
static constexpr char   SEM_FULL_NAME[]  = "Lab4_SemFull";
static constexpr char   SEM_EMPTY_NAME[] = "Lab4_SemEmpty";
static constexpr char   READY_EVENT[]    = "Lab4_SenderReady";

#pragma pack(push, 1)
struct Message {
    char     text[MAX_MSG_LEN];
    uint8_t  valid;
};
#pragma pack(pop)
