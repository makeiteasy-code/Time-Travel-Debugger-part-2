#pragma once
#include <iostream>
#include <cstdint>
#include <string>
#include "common.h"
#include "constants.h"
#include "stack.h"
#include "timeline.h"
#include "resolve.h"

using namespace std;
enum tokenType
{
    KEYWORD,
    IDENTIFIER,
    PARAM
};
struct Token
{
    tokenType type;
    string text;
};
int32_t tokenizeLine(const string &line, Token tokens[], int32_t maxTokens);
Snapshot *buildSnapshot(Stack<Frame> &callStack);
void executeProgram(const char *resolveBinPath, int64_t mainOffset, Timeline &timeline);