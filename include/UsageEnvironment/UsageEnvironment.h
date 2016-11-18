#ifndef _USAGE_ENVIRONMENT_H
#define _USAGE_ENVIRONMENT_H



#ifndef _NETCOMMON_H
#include <NetCommon.h>
#endif

#ifndef _BOOLEAN_H
#include <Boolean.h>
#endif

#ifndef _STRDUP_H
#include <strDup.h>
#endif

#ifndef NULL
#define NULL 0
#endif


class TaskScheduler; 

//该库的基础抽象类
class UsageEnvironment
{
public:
    Boolean reclaim();

    // 任务调度器
    TaskScheduler& taskScheduler() const
    {
        return fScheduler;
    }

    // 日志消息处理
    typedef char const* MsgString;
    virtual MsgString getResultMsg() const = 0;

    virtual void setResultMsg(MsgString msg) = 0;
    virtual void setResultMsg(MsgString msg1, MsgString msg2) = 0;
    virtual void setResultMsg(MsgString msg1, MsgString msg2, MsgString msg3) = 0;
    virtual void setResultErrMsg(MsgString msg, int err = 0) = 0;

    virtual void appendToResultMsg(MsgString msg) = 0;

    virtual void reportBackgroundError() = 0;

    virtual void internalError(); 
    virtual int getErrno() const = 0;
    virtual UsageEnvironment& operator<<(char const* str) = 0;
    virtual UsageEnvironment& operator<<(int i) = 0;
    virtual UsageEnvironment& operator<<(unsigned u) = 0;
    virtual UsageEnvironment& operator<<(double d) = 0;
    virtual UsageEnvironment& operator<<(void* p) = 0;

    void* liveMediaPriv;
    void* groupsockPriv;

protected:
    UsageEnvironment(TaskScheduler& scheduler); // abstract base class
    virtual ~UsageEnvironment(); // we are deleted only by reclaim()

private:
    TaskScheduler& fScheduler;
};


typedef void TaskFunc(void* clientData);
typedef void* TaskToken;
typedef u_int32_t EventTriggerId;


//调度器基类
class TaskScheduler
{
public:
    virtual ~TaskScheduler();

    virtual TaskToken scheduleDelayedTask(int64_t microseconds, TaskFunc* proc,
                                          void* clientData) = 0;

    virtual void unscheduleDelayedTask(TaskToken& prevTask) = 0;
    virtual void rescheduleDelayedTask(TaskToken& task,
                                       int64_t microseconds, TaskFunc* proc,
                                       void* clientData);

    //调度器里面处理socket事件的回调函数
    typedef void BackgroundHandlerProc(void* clientData, int mask);
    // Possible bits to set in "mask".  (These are deliberately defined
    // the same as those in Tcl, to make a Tcl-based subclass easy.)
#define SOCKET_READABLE    (1<<1)
#define SOCKET_WRITABLE    (1<<2)
#define SOCKET_EXCEPTION   (1<<3)
    virtual void setBackgroundHandling(int socketNum, int conditionSet, BackgroundHandlerProc* handlerProc, void* clientData) = 0;
    void disableBackgroundHandling(int socketNum)
    {
        setBackgroundHandling(socketNum, 0, NULL, NULL);
    }
	//新的socket取代老的socket，因为socket的文件描述符有可能改变(close之后重新创建)
    virtual void moveSocketHandling(int oldSocketNum, int newSocketNum) = 0;
	//调度器总循环
    virtual void doEventLoop(char volatile* watchVariable = NULL) = 0;

    virtual EventTriggerId createEventTrigger(TaskFunc* eventHandlerProc) = 0;
    virtual void deleteEventTrigger(EventTriggerId eventTriggerId) = 0;

    virtual void triggerEvent(EventTriggerId eventTriggerId, void* clientData = NULL) = 0;

    void turnOnBackgroundReadHandling(int socketNum, BackgroundHandlerProc* handlerProc, void* clientData)
    {
        setBackgroundHandling(socketNum, SOCKET_READABLE, handlerProc, clientData);
    }
    void turnOffBackgroundReadHandling(int socketNum)
    {
        disableBackgroundHandling(socketNum);
    }

    virtual void internalError();

protected:
    TaskScheduler(); 
};

#endif
