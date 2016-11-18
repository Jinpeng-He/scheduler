#include <BasicUsageEnvironment0.h>
#include <HandlerSet.h>

//����������ʾ�������
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
	//��ʱ��������
    TaskFunc* fProc;
	//��ʱ���������Ĳ���
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

//ע��delaytask���񵽵�����
//microseconds:��ʱ��ʱ��
//proc:ע��Ļص�����
//����:TaskToken Ψһ��Ӧһ��delaytask��֮�����ͨ���������ע��������ע�������
TaskToken BasicTaskScheduler0::scheduleDelayedTask(int64_t microseconds,
        TaskFunc* proc,
        void* clientData)
{
    if (microseconds < 0) microseconds = 0;
    DelayInterval timeToDelay((long)(microseconds/1000000), (long)(microseconds%1000000));
	//������ʱ�������
    AlarmHandler* alarmHandler = new AlarmHandler(proc, clientData, timeToDelay);
	//����ʱ������ӽ�delayqueue
    fDelayQueue.addEntry(alarmHandler);

    return (void*)(alarmHandler->token());
}
//����ע��ͨ��scheduleDelayedTaskע���delaytask
void BasicTaskScheduler0::unscheduleDelayedTask(TaskToken& prevTask)
{
    DelayQueueEntry* alarmHandler = fDelayQueue.removeEntry((intptr_t)prevTask);
    prevTask = NULL;
    delete alarmHandler;
}
//��������ѭ��
void BasicTaskScheduler0::doEventLoop(char volatile* watchVariable)
{
    // Repeatedly loop, handling readble sockets and timed events:
    while (1)
    {
        if (watchVariable != NULL && *watchVariable != 0) break;
        SingleStep();
    }
}
//ע���û������¼������������û�����ʹ�÷���ֵ����ȥ�����������
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
            // �ص�����ע���fTriggeredEventHandlers����
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
//ɾ���û�ע��Ĵ����¼�
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
//�û�ʹ��ע��ʱ���ɵ�EventTriggerIdȥ����ע��Ĵ����¼����������ǵĵ������ͻᴦ�����ǵĴ����¼�
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



//����socket�����¼����¼�����������
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

//���紥���¼�������˫������
HandlerSet::HandlerSet()
    : fHandlers(&fHandlers)
{
    fHandlers.socketNum = -1;
}

HandlerSet::~HandlerSet()
{
	//ɾ��ÿһ��handler����������ֹ�ڴ�й¶
    while (fHandlers.fNextHandler != &fHandlers)
    {
        delete fHandlers.fNextHandler;
    }
}

//�����ǵ�socket��������ʱ�Ĵ�����ע���HandlerSet����ʵ��һ��HandlerDescriptor
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

//ɾ��socketNum��Ӧ�Ĵ����ص�����
void HandlerSet::clearHandler(int socketNum)
{
    HandlerDescriptor* handler = lookupHandler(socketNum);
    delete handler;
}

//����socket�п����б䶯����������޸�socket�ļ�������
void HandlerSet::moveHandler(int oldSocketNum, int newSocketNum)
{
    HandlerDescriptor* handler = lookupHandler(oldSocketNum);
    if (handler != NULL)
    {
        handler->socketNum = newSocketNum;
    }
}

//ͨ���ļ����������鿴���ǵ����������Ƿ��и�socketNum��Ӧ��handler
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

//��λ����
void HandlerIterator::reset()
{
    fNextPtr = fOurSet.fHandlers.fNextHandler;
}

//��������
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
