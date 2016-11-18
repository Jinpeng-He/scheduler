#include <BasicUsageEnvironment0.h>
#include <HandlerSet.h>

//用于描述演示任务的类
class AlarmHandler: public DelayQueueEntry
{
public:
    AlarmHandler(TaskFunc* proc, void* clientData, DelayInterval timeToDelay)
        : DelayQueueEntry(timeToDelay), fProc(proc), fClientData(clientData)
    {
    }

private: // redefined virtual functions
    virtual void handleTimeout()
    {
        (*fProc)(fClientData);
        DelayQueueEntry::handleTimeout();
    }

private:
	//延时任务处理函数
    TaskFunc* fProc;
	//延时任务处理函数的参数
    void* fClientData;
};


BasicTaskScheduler0::BasicTaskScheduler0()
    : fLastHandledSocketNum(-1), fTriggersAwaitingHandling(0), fLastUsedTriggerMask(1), fLastUsedTriggerNum(MAX_NUM_EVENT_TRIGGERS-1)
{
    fHandlers = new HandlerSet;
    for (unsigned i = 0; i < MAX_NUM_EVENT_TRIGGERS; ++i)
    {
        fTriggeredEventHandlers[i] = NULL;
        fTriggeredEventClientDatas[i] = NULL;
    }
}

BasicTaskScheduler0::~BasicTaskScheduler0()
{
    delete fHandlers;
}

//注册delaytask任务到调度器
//microseconds:延时的时间
//proc:注册的回调函数
//返回:TaskToken 唯一对应一个delaytask，之后可以通过这个参数注销掉这里注册的任务
TaskToken BasicTaskScheduler0::scheduleDelayedTask(int64_t microseconds,
        TaskFunc* proc,
        void* clientData)
{
    if (microseconds < 0) microseconds = 0;
    DelayInterval timeToDelay((long)(microseconds/1000000), (long)(microseconds%1000000));
	//生成延时任务对象
    AlarmHandler* alarmHandler = new AlarmHandler(proc, clientData, timeToDelay);
	//将延时对象添加进delayqueue
    fDelayQueue.addEntry(alarmHandler);

    return (void*)(alarmHandler->token());
}
//这里注销通过scheduleDelayedTask注册的delaytask
void BasicTaskScheduler0::unscheduleDelayedTask(TaskToken& prevTask)
{
    DelayQueueEntry* alarmHandler = fDelayQueue.removeEntry((intptr_t)prevTask);
    prevTask = NULL;
    delete alarmHandler;
}
//调度器总循环
void BasicTaskScheduler0::doEventLoop(char volatile* watchVariable)
{
    // Repeatedly loop, handling readble sockets and timed events:
    while (1)
    {
        if (watchVariable != NULL && *watchVariable != 0) break;
        SingleStep();
    }
}
//注册用户触发事件到调度器，用户可以使用返回值主动去触发这个任务
EventTriggerId BasicTaskScheduler0::createEventTrigger(TaskFunc* eventHandlerProc)
{
    unsigned i = fLastUsedTriggerNum;
    EventTriggerId mask = fLastUsedTriggerMask;

    do
    {
        i = (i+1)%MAX_NUM_EVENT_TRIGGERS;
        mask >>= 1;
        if (mask == 0) mask = 0x80000000;

        if (fTriggeredEventHandlers[i] == NULL)
        {
            // 回调函数注册进fTriggeredEventHandlers数组
            fTriggeredEventHandlers[i] = eventHandlerProc;
            fTriggeredEventClientDatas[i] = NULL; // sanity

            fLastUsedTriggerMask = mask;
            fLastUsedTriggerNum = i;

            return mask;
        }
    }
    while (i != fLastUsedTriggerNum);

    return 0;
}
//删除用户注册的触发事件
void BasicTaskScheduler0::deleteEventTrigger(EventTriggerId eventTriggerId)
{
    fTriggersAwaitingHandling &=~ eventTriggerId;

    if (eventTriggerId == fLastUsedTriggerMask)
    {
        fTriggeredEventHandlers[fLastUsedTriggerNum] = NULL;
        fTriggeredEventClientDatas[fLastUsedTriggerNum] = NULL;
    }
    else
    {
        EventTriggerId mask = 0x80000000;
        for (unsigned i = 0; i < MAX_NUM_EVENT_TRIGGERS; ++i)
        {
            if ((eventTriggerId&mask) != 0)
            {
                fTriggeredEventHandlers[i] = NULL;
                fTriggeredEventClientDatas[i] = NULL;
            }
            mask >>= 1;
        }
    }
}
//用户使用注册时生成的EventTriggerId去触发注册的触发事件，这样我们的调度器就会处理我们的触发事件
void BasicTaskScheduler0::triggerEvent(EventTriggerId eventTriggerId, void* clientData)
{
    EventTriggerId mask = 0x80000000;
    for (unsigned i = 0; i < MAX_NUM_EVENT_TRIGGERS; ++i)
    {
        if ((eventTriggerId&mask) != 0)
        {
            fTriggeredEventClientDatas[i] = clientData;
        }
        mask >>= 1;
    }
    fTriggersAwaitingHandling |= eventTriggerId;
}



//网络socket触发事件的事件遍历描述符
HandlerDescriptor::HandlerDescriptor(HandlerDescriptor* nextHandler)
    : conditionSet(0), handlerProc(NULL)
{
    if (nextHandler == this)
    {
        fNextHandler = fPrevHandler = this;
    }
    else
    {
        fNextHandler = nextHandler;
        fPrevHandler = nextHandler->fPrevHandler;
        nextHandler->fPrevHandler = this;
        fPrevHandler->fNextHandler = this;
    }
}

HandlerDescriptor::~HandlerDescriptor()
{
    fNextHandler->fPrevHandler = fPrevHandler;
    fPrevHandler->fNextHandler = fNextHandler;
}

//网络触发事件函数的双向链表
HandlerSet::HandlerSet()
    : fHandlers(&fHandlers)
{
    fHandlers.socketNum = -1;
}

HandlerSet::~HandlerSet()
{
	//删除每一个handler的描述符防止内存泄露
    while (fHandlers.fNextHandler != &fHandlers)
    {
        delete fHandlers.fNextHandler;
    }
}

//将我们的socket条件满足时的处理函数注册进HandlerSet，其实是一个HandlerDescriptor
void HandlerSet
::assignHandler(int socketNum, int conditionSet, TaskScheduler::BackgroundHandlerProc* handlerProc, void* clientData)
{
    HandlerDescriptor* handler = lookupHandler(socketNum);
    if (handler == NULL)
    {
        handler = new HandlerDescriptor(fHandlers.fNextHandler);
        handler->socketNum = socketNum;
    }

    handler->conditionSet = conditionSet;
    handler->handlerProc = handlerProc;
    handler->clientData = clientData;
}

//删除socketNum对应的触发回调函数
void HandlerSet::clearHandler(int socketNum)
{
    HandlerDescriptor* handler = lookupHandler(socketNum);
    delete handler;
}

//由于socket有可能有变动，因此这里修改socket文件描述符
void HandlerSet::moveHandler(int oldSocketNum, int newSocketNum)
{
    HandlerDescriptor* handler = lookupHandler(oldSocketNum);
    if (handler != NULL)
    {
        handler->socketNum = newSocketNum;
    }
}

//通过文件描述符，查看我们的链表里面是否有该socketNum对应的handler
HandlerDescriptor* HandlerSet::lookupHandler(int socketNum)
{
    HandlerDescriptor* handler;
    HandlerIterator iter(*this);
    while ((handler = iter.next()) != NULL)
    {
        if (handler->socketNum == socketNum) break;
    }
    return handler;
}

HandlerIterator::HandlerIterator(HandlerSet& handlerSet)
    : fOurSet(handlerSet)
{
    reset();
}

HandlerIterator::~HandlerIterator()
{
}

//复位链表
void HandlerIterator::reset()
{
    fNextPtr = fOurSet.fHandlers.fNextHandler;
}

//遍历链表
HandlerDescriptor* HandlerIterator::next()
{
    HandlerDescriptor* result = fNextPtr;
    if (result == &fOurSet.fHandlers)
    {
        result = NULL;
    }
    else
    {
        fNextPtr = fNextPtr->fNextHandler;
    }

    return result;
}
