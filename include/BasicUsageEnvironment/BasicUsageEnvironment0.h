#ifndef _BASIC_USAGE_ENVIRONMENT0_H
#define _BASIC_USAGE_ENVIRONMENT0_H


#ifndef _USAGE_ENVIRONMENT_H
#include <UsageEnvironment.h>
#endif

#ifndef _DELAY_QUEUE_H
#include <DelayQueue.h>
#endif

#define RESULT_MSG_BUFFER_MAX 1000

//һ��������,�ṩ����־��¼�Ļ���ʵ��
class BasicUsageEnvironment0: public UsageEnvironment
{
public:
    virtual MsgString getResultMsg() const;

    virtual void setResultMsg(MsgString msg);
    virtual void setResultMsg(MsgString msg1,
                              MsgString msg2);
    virtual void setResultMsg(MsgString msg1,
                              MsgString msg2,
                              MsgString msg3);
    virtual void setResultErrMsg(MsgString msg, int err = 0);

    virtual void appendToResultMsg(MsgString msg);

    virtual void reportBackgroundError();

protected:
    BasicUsageEnvironment0(TaskScheduler& taskScheduler);
    virtual ~BasicUsageEnvironment0();

private:
    void reset();
	//��־buffer
    char fResultMsgBuffer[RESULT_MSG_BUFFER_MAX];
    unsigned fCurBufferSize;
    unsigned fBufferMaxSize;
};

class HandlerSet;

#define MAX_NUM_EVENT_TRIGGERS 32

//������������
class BasicTaskScheduler0: public TaskScheduler
{
public:
    virtual ~BasicTaskScheduler0();
	//������һ��ѭ��
    virtual void SingleStep(unsigned maxDelayTime = 0) = 0;

public:
    // Redefined virtual functions:
    //ע��delaytask�¼�
    virtual TaskToken scheduleDelayedTask(int64_t microseconds, TaskFunc* proc,
                                          void* clientData);
	//ע��delaytask�¼�
    virtual void unscheduleDelayedTask(TaskToken& prevTask);
	//���������Ⱥ���
    virtual void doEventLoop(char volatile* watchVariable);
	//���������¼�
    virtual EventTriggerId createEventTrigger(TaskFunc* eventHandlerProc);
	//ɾ�������¼�
    virtual void deleteEventTrigger(EventTriggerId eventTriggerId);
	//�û��������������¼�
    virtual void triggerEvent(EventTriggerId eventTriggerId, void* clientData = NULL);

protected:
    BasicTaskScheduler0();

protected:
    //delaytask���¼�����
    DelayQueue fDelayQueue;

    //��������ص�socket����������
    HandlerSet* fHandlers;
    int fLastHandledSocketNum;

    // ʹ�����¼���Ч
    EventTriggerId volatile fTriggersAwaitingHandling;
    EventTriggerId fLastUsedTriggerMask;
	//�û������¼��Ļص���������
    TaskFunc* fTriggeredEventHandlers[MAX_NUM_EVENT_TRIGGERS];
	//�û������¼��ص������Ĳ�������
    void* fTriggeredEventClientDatas[MAX_NUM_EVENT_TRIGGERS];
    unsigned fLastUsedTriggerNum;//��Χ[0,MAX_NUM_EVENT_TRIGGERS]
};

#endif
