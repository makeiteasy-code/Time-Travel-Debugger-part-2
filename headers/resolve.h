#pragma once
#include <iostream>
#include <cstdint>
#include <cstdio>
#include <string>
#include "constants.h"
using namespace std;
struct FuncEntry
{
    string funcName;
    int64_t byteOffsetInResolveBin; // where this function's FUNC header record sits
};
struct PendingPatch
{
    int64_t byteOffsetOfOffsetField; // where in resolve.bin to seek back and overwrite
    string targetFuncName;
};


int64_t writeResolveRecord(FILE *f, int64_t offsetField, const string &text);
int64_t readResolveRecord(FILE *f, string &outText);
int64_t resolveProgram(const char *sourcePath, const char *resolveBinPath);
