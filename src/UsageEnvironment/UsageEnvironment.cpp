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
//默认情况下，我们的select出错时调用abort(),我们可以在子类里面重新实现
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

//默认情况下，我们的select出错时调用abort(),我们可以在子类里面重新实现
void TaskScheduler::internalError()
{
    abort();
}
