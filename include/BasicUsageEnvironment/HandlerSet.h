#ifndef _HANDLER_SET_H
#define _HANDLER_SET_H

#ifndef _BOOLEAN_H
#include <Boolean.h>
#endif



class HandlerDescriptor
{
    HandlerDescriptor(HandlerDescriptor* nextHandler);
    virtual ~HandlerDescriptor();

public:
    int socketNum;
    int conditionSet;
    TaskScheduler::BackgroundHandlerProc* handlerProc;
    void* clientData;

private:
    friend class HandlerSet;
    friend class HandlerIterator;
    HandlerDescriptor* fNextHandler;
    HandlerDescriptor* fPrevHandler;
};

class HandlerSet
{
public:
    HandlerSet();
    virtual ~HandlerSet();

    void assignHandler(int socketNum, int conditionSet, TaskScheduler::BackgroundHandlerProc* handlerProc, void* clientData);
    void clearHandler(int socketNum);
    void moveHandler(int oldSocketNum, int newSocketNum);

private:
    HandlerDescriptor* lookupHandler(int socketNum);

private:
    friend class HandlerIterator;
    HandlerDescriptor fHandlers;
};

class HandlerIterator
{
public:
    HandlerIterator(HandlerSet& handlerSet);
    virtual ~HandlerIterator();

    HandlerDescriptor* next();
    void reset();

private:
    HandlerSet& fOurSet;
    HandlerDescriptor* fNextPtr;
};

#endif
