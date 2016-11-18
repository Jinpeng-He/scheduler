#include <UsageEnvironment.h>

Boolean UsageEnvironment::reclaim()
{
    if (liveMediaPriv == NULL && groupsockPriv == NULL)
    {
        delete this;
        return True;
    }
    return False;
}

UsageEnvironment::UsageEnvironment(TaskScheduler& scheduler)
    : liveMediaPriv(NULL), groupsockPriv(NULL), fScheduler(scheduler)
{
}

UsageEnvironment::~UsageEnvironment()
{
}
//Ĭ������£����ǵ�select����ʱ����abort(),���ǿ�����������������ʵ��
void UsageEnvironment::internalError()
{
    abort();
}


TaskScheduler::TaskScheduler()
{
}

TaskScheduler::~TaskScheduler()
{
}

void TaskScheduler::rescheduleDelayedTask(TaskToken& task,
        int64_t microseconds, TaskFunc* proc,
        void* clientData)
{
    unscheduleDelayedTask(task);
    task = scheduleDelayedTask(microseconds, proc, clientData);
}

//Ĭ������£����ǵ�select����ʱ����abort(),���ǿ�����������������ʵ��
void TaskScheduler::internalError()
{
    abort();
}
