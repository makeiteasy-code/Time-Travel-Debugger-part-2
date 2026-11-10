#pragma once
#include <iostream>
#include <cstdint>
#include <string>
using namespace std;
template <typename T>
class Stack
{
    struct Node
    {
        T data;
        Node *next;

        Node(const T& d, Node* ptr) :data(d), next(ptr) {}
    };
    Node *top;
    int32_t count; 

public:
    // Implement these functions:

	Stack():top(nullptr),count(0){}

    void push(const T &val)
    {
		if(count<=0){
			top=new Node(val,nullptr);
            count++;
            return;
        }
        if(count<=MAX_STACK_DEPTH){
            top=new Node(val,top);
            count++;
        }
        else {
            throw "Stack overflow";
        }
    }
    T pop()
    {
        if (count > 0) {
            Node* a = top->next;
            T value = top->data;
            delete top;
            top = a;
            count--;
            return value;
        }
        else {
            throw "Stack underflow";
        }
    }
    
    T &peek()
    {
        if(count>0) return top->data;
        throw "Stack underflow";
    }
    bool isEmpty()
    {
        if(count<=0)return 1;
        return 0;
    }
    int32_t depth()
    {
        return count;
    }
    int32_t snapshot_into(T out[], int32_t maxLen)
    {
        // copies every frame, top to bottom in the array given as a parameter
        // this is what buildSnapshot() call, returns count written
        Node* temp=top;
        for(int i=0;i<maxLen&&i<count;i++){
            out[i]=temp->data;
            temp=temp->next;
        }
        if(count<=maxLen)return count;
        return maxLen;
    }
};
