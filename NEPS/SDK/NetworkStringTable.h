#pragma once

#include "VirtualMethod.h"

class NetworkStringTable {
public:
    VIRTUAL_METHOD(int, getNumStrings, 3, (), (this))
    VIRTUAL_METHOD(int, addString, 8, (bool isServer, const char* value, int length = -1, const void* userdata = nullptr), (this, isServer, value, length, userdata))
    VIRTUAL_METHOD(const char *, getString, 9, (int i), (this, i))
};

class NetworkStringTableContainer {
public:
    VIRTUAL_METHOD(NetworkStringTable *, findTable, 3, (const char* name), (this, name))
};
