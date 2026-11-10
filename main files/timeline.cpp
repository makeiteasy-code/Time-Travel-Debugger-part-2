#include"timeline.h"

Timeline::Timeline():head(nullptr),tail(nullptr),stepCount(0){}
void Timeline::record(Snapshot *s)
{
    if (head == nullptr) {
        head = tail = new TimelineNode(s);
        stepCount++;
        return;
    }
    if(head==tail){
        tail=new TimelineNode(s);
        head->next=tail;
        tail->prev=head;
        stepCount++;
        return;
    }
    TimelineNode* temp = tail;
    tail = new TimelineNode(s);
    temp->next = tail;
    tail->prev=temp;
    stepCount++;
}
TimelineNode *Timeline::begin()
{
    return head;
}
int32_t Timeline::getStepCount()
{
    return stepCount;
}
