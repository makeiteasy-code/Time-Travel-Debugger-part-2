#pragma once
#include <iostream>
#include <string>
#include <cstdint>
using namespace std;
struct Snapshot; // fwd declaration;
struct TimelineNode
{
    Snapshot *data;
    TimelineNode *next;
    TimelineNode *prev;

    TimelineNode(Snapshot*s):data(s),next(nullptr),prev(nullptr){}
};

class Timeline
{
    TimelineNode *head, *tail;
    int32_t stepCount;

public:
    // Implement these functions
    Timeline();
    void record(Snapshot *s);
    TimelineNode *begin();
    int32_t getStepCount();
};