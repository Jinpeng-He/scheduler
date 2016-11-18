#ifndef _BASIC_USAGE_ENVIRONMENT0_H
#define _BASIC_USAGE_ENVIRONMENT0_H


#ifndef _USAGE_ENVIRONMENT_H
#include <UsageEnvironment.h>
#endif

#ifndef _DELAY_QUEUE_H
#include <DelayQueue.h>
#endif

#define RESULT_MSG_BUFFER_MAX 1000

//一个抽象类,提供了日志记录的基本实现
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
	//日志buffer
    char fResultMsgBuffer[RESULT_MSG_BUFFER_MAX];
    unsigned fCurBufferSize;
    unsigned fBufferMaxSize;
};

class HandlerSet;

#define MAX_NUM_EVENT_TRIGGERS 32

//调度器抽象类
class BasicTaskScheduler0: public TaskScheduler
{
public:
    virtual ~BasicTaskScheduler0();
	//调度器一次循环
    virtual void SingleStep(unsigned maxDelayTime = 0) = 0;

public:
    // Redefined virtual functions:
    //注册delaytask事件
    virtual TaskToken scheduleDelayedTask(int64_t microseconds, TaskFunc* proc,
                                          void* clientData);
	//注销delaytask事件
    virtual void unscheduleDelayedTask(TaskToken& prevTask);
	//调度器调度函数
    virtual void doEventLoop(char volatile* watchVariable);
	//创建触发事件
    virtual EventTriggerId createEventTrigger(TaskFunc* eventHandlerProc);
	//删除触发事件
    virtual void deleteEventTrigger(EventTriggerId eventTriggerId);
	//用户主动触发触发事件
    virtual void triggerEvent(EventTriggerId eventTriggerId, void* clientData = NULL);

protected:
    BasicTaskScheduler0();

protected:
    //delaytask的事件队列
    DelayQueue fDelayQueue;

    //调度器监控的socket处理函数链表
    HandlerSet* fHandlers;
    int fLastHandledSocketNum;

    // 使触发事件生效
    EventTriggerId volatile fTriggersAwaitingHandling;
    EventTriggerId fLastUsedTriggerMask;
	//用户触发事件的回调函数数组
    TaskFunc* fTriggeredEventHandlers[MAX_NUM_EVENT_TRIGGERS];
	//用户触发事件回调函数的参数数组
    void* fTriggeredEventClientDatas[MAX_NUM_EVENT_TRIGGERS];
    unsigned fLastUsedTriggerNum;//范围[0,MAX_NUM_EVENT_TRIGGERS]
};

#endif
