#pragma once
#include <iostream>
#include <fstream>
#include <cstdint>
#include <cstdio>
#include <string>
using namespace std;
bool readSourceLine(ifstream &in, string &out);
string firstWord(const string &line);
string secondWord(const string &line);
bool validateProgram(const char *sourcePath);
