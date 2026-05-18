#pragma once
#include <cstring>

#pragma pack(push, 1)
struct Employee {
    int    num;
    char   name[10];
    double hours;
};
#pragma pack(pop)

enum class RequestType : int {
    Read   = 0,
    Modify = 1,
    Exit   = 2
};

struct Request {
    RequestType type;
    int         key;
    Employee    data;
};

struct Response {
    bool     success;
    Employee data;
    char     message[128];
};
