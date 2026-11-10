#pragma once
#include <iostream>
#include <cstdint>
#include <cstdio>
#include "common.h"
#include "timeline.h"
#include <cstdio>
#include <string>
using namespace std;
void writeHeader(FILE *f, const TTDBHeader &h);
void writeTdbg(Timeline &timeline, const char *tdbgPath);