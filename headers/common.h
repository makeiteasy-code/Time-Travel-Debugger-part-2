#pragma once
#include <cstdint>
#include <string>
#include "constants.h"
using namespace std;

struct Variable
{
    string name;
    int32_t value;
};
struct Frame
{
    string func_name;
    int32_t argc;
    Variable argv[MAX_VARS_PER_FRAME];
    int64_t returnLine;
    Variable locals[MAX_VARS_PER_FRAME];
    int32_t localCount;
    Frame():func_name(""),argc(0),argv{},returnLine(-1),locals{},localCount(0){}

    Frame(string name,int32_t argC=0,int64_t offset=-1,int32_t localC=0):func_name(name),argc(argC),returnLine(offset),localCount(localC){}
};
struct Snapshot
{
    Frame callStack[MAX_STACK_DEPTH];
    int32_t stackDepth;
};
struct TTDBHeader
{
    char magic[4]; // "TTDB"
    int32_t version;
    int32_t stepCount;
    int64_t indexOffset;

    TTDBHeader ():version(1),stepCount(0),indexOffset(0) {
        magic[0]='T';
        magic[1]='T';
        magic[2]='D';
        magic[3]='B';
        
    }
    

};