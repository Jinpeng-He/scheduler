#ifndef _BASIC_USAGE_ENVIRONMENT_H
#define _BASIC_USAGE_ENVIRONMENT_H

#ifndef _BASIC_USAGE_ENVIRONMENT0_H
#include <BasicUsageEnvironment0.h>
#endif

//基础环境实现类
class BasicUsageEnvironment: public BasicUsageEnvironment0
{
public:
    static BasicUsageEnvironment* createNew(TaskScheduler& taskScheduler);

    virtual int getErrno() const;
	//操作符重载，用于信息标准输出
    virtual UsageEnvironment& operator<<(char const* str);
    virtual UsageEnvironment& operator<<(int i);
    virtual UsageEnvironment& operator<<(unsigned u);
    virtual UsageEnvironment& operator<<(double d);
    virtual UsageEnvironment& operator<<(void* p);

protected:
	//该函数只能被createNew()或者子类调用
    BasicUsageEnvironment(TaskScheduler& taskScheduler);
    virtual ~BasicUsageEnvironment();
};


//基础调度器实现类
class BasicTaskScheduler: public BasicTaskScheduler0
{
public:
	//调度器创建接口,maxSchedulerGranularity单位为us，保证一次循环的最大延时时间
    static BasicTaskScheduler* createNew(unsigned maxSchedulerGranularity = 10000/*microseconds*/);
    virtual ~BasicTaskScheduler();

protected:
	//该函数只能被createNew()或者子类调用
    BasicTaskScheduler(unsigned maxSchedulerGranularity);

	//调度器时钟,保证在没有事件处理的时候，调度器每隔一段时间唤醒一次
    static void schedulerTickTask(void* clientData);
    void schedulerTickTask();

protected:
    //调度器调度循环单元
    virtual void SingleStep(unsigned maxDelayTime);
	//设置需要调度器监控的socket,同时指定监控回调函数和参数
    virtual void setBackgroundHandling(int socketNum, int conditionSet, BackgroundHandlerProc* handlerProc, void* clientData);
	//取消老的socket添加新的socket描述符
	virtual void moveSocketHandling(int oldSocketNum, int newSocketNum);

protected:
    unsigned fMaxSchedulerGranularity;
	//socket描述符的最大值
    int fMaxNumSockets;
	//socket可读
    fd_set fReadSet;
	//socket可写
    fd_set fWriteSet;
	//socket异常
    fd_set fExceptionSet;

private:
};

#endif
