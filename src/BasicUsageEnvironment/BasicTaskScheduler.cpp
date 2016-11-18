#include <BasicUsageEnvironment.h>
#include <HandlerSet.h>
#include <stdio.h>


//静态函数创建BasicTaskScheduler调度类的对象
BasicTaskScheduler* BasicTaskScheduler::createNew(unsigned maxSchedulerGranularity)
{
    return new BasicTaskScheduler(maxSchedulerGranularity);
}
//构造函数，创建调度器，清空socket描述符的set
BasicTaskScheduler::BasicTaskScheduler(unsigned maxSchedulerGranularity)
    : fMaxSchedulerGranularity(maxSchedulerGranularity), fMaxNumSockets(0)

{
    FD_ZERO(&fReadSet);
    FD_ZERO(&fWriteSet);
    FD_ZERO(&fExceptionSet);
	//如果maxSchedulerGranularity大于0,就在调度器里面添加一个时钟任务，保证调度器不会永远阻塞
    if (maxSchedulerGranularity > 0) schedulerTickTask();
}

BasicTaskScheduler::~BasicTaskScheduler()
{
}
//时钟函数，保证周期向调度器添加时钟函数，类似与递归函数，
void BasicTaskScheduler::schedulerTickTask(void* clientData)
{
    ((BasicTaskScheduler*)clientData)->schedulerTickTask();
}
//时钟函数
void BasicTaskScheduler::schedulerTickTask()
{
	printf("the scheduler test tick...\n");
    scheduleDelayedTask(fMaxSchedulerGranularity, schedulerTickTask, this);
}

#ifndef MILLION
#define MILLION 1000000
#endif
//任务调度函数的一个循环调度，maxDelayTime是传递的最大延时时间，保证select不会长期阻塞，
void BasicTaskScheduler::SingleStep(unsigned maxDelayTime)
{
    fd_set readSet = fReadSet;
    fd_set writeSet = fWriteSet; 
    fd_set exceptionSet = fExceptionSet; 
	//从delayqueue里面取出一个延时时间
    DelayInterval const& timeToDelay = fDelayQueue.timeToNextAlarm();
    struct timeval tv_timeToDelay;
    tv_timeToDelay.tv_sec = timeToDelay.seconds();
    tv_timeToDelay.tv_usec = timeToDelay.useconds();
    const long MAX_TV_SEC = MILLION;
    if (tv_timeToDelay.tv_sec > MAX_TV_SEC)
    {
        tv_timeToDelay.tv_sec = MAX_TV_SEC;
    }
    // 同时不能大于我们创建调度器的时候传递的最大延时时间保证调度器的实时性:
    if (maxDelayTime > 0 &&
        (tv_timeToDelay.tv_sec > (long)maxDelayTime/MILLION ||
         (tv_timeToDelay.tv_sec == (long)maxDelayTime/MILLION &&
          tv_timeToDelay.tv_usec > (long)maxDelayTime%MILLION)))
    {
        tv_timeToDelay.tv_sec = maxDelayTime/MILLION;
        tv_timeToDelay.tv_usec = maxDelayTime%MILLION;
    }
	//select阻塞等待网络事件
    int selectResult = select(fMaxNumSockets, &readSet, &writeSet, &exceptionSet, &tv_timeToDelay);
    if (selectResult < 0)
    {

        if (errno != EINTR && errno != EAGAIN)
        {
            fprintf(stderr, "socket numbers used in the select() call:");
            for (int i = 0; i < 10000; ++i)
            {
                if (FD_ISSET(i, &fReadSet) || FD_ISSET(i, &fWriteSet) || FD_ISSET(i, &fExceptionSet))
                {
                    fprintf(stderr, " %d(", i);
                    if (FD_ISSET(i, &fReadSet)) fprintf(stderr, "r");
                    if (FD_ISSET(i, &fWriteSet)) fprintf(stderr, "w");
                    if (FD_ISSET(i, &fExceptionSet)) fprintf(stderr, "e");
                    fprintf(stderr, ")");
                }
            }
            fprintf(stderr, "\n");
            internalError();
        }
    }

    HandlerIterator iter(*fHandlers);
    HandlerDescriptor* handler;
    if (fLastHandledSocketNum >= 0)
    {
        while ((handler = iter.next()) != NULL)
        {
            if (handler->socketNum == fLastHandledSocketNum) break;
        }
        if (handler == NULL)
        {
            fLastHandledSocketNum = -1;
            iter.reset();
        }
    }
    while ((handler = iter.next()) != NULL)
    {
        int sock = handler->socketNum; // alias
        int resultConditionSet = 0;
        if (FD_ISSET(sock, &readSet) && FD_ISSET(sock, &fReadSet)) resultConditionSet |= SOCKET_READABLE;
        if (FD_ISSET(sock, &writeSet) && FD_ISSET(sock, &fWriteSet)) resultConditionSet |= SOCKET_WRITABLE;
        if (FD_ISSET(sock, &exceptionSet) && FD_ISSET(sock, &fExceptionSet)) resultConditionSet |= SOCKET_EXCEPTION;
        if ((resultConditionSet&handler->conditionSet) != 0 && handler->handlerProc != NULL)
        {
            fLastHandledSocketNum = sock;
			//处理注册好的socket触发之后的回调函数
            (*handler->handlerProc)(handler->clientData, resultConditionSet);
            break;
        }
    }
	//检查两次防止漏检
    if (handler == NULL && fLastHandledSocketNum >= 0)
    {
        iter.reset();
        while ((handler = iter.next()) != NULL)
        {
            int sock = handler->socketNum; // alias
            int resultConditionSet = 0;
            if (FD_ISSET(sock, &readSet) && FD_ISSET(sock, &fReadSet)) resultConditionSet |= SOCKET_READABLE;
            if (FD_ISSET(sock, &writeSet) && FD_ISSET(sock, &fWriteSet)) resultConditionSet |= SOCKET_WRITABLE;
            if (FD_ISSET(sock, &exceptionSet) && FD_ISSET(sock, &fExceptionSet)) resultConditionSet |= SOCKET_EXCEPTION;
            if ((resultConditionSet&handler->conditionSet) != 0 && handler->handlerProc != NULL)
            {
                fLastHandledSocketNum = sock;

                (*handler->handlerProc)(handler->clientData, resultConditionSet);
                break;
            }
        }
        if (handler == NULL) fLastHandledSocketNum = -1;
    }

    //检查是否有用户触发事件
    if (fTriggersAwaitingHandling != 0)
    {
        if (fTriggersAwaitingHandling == fLastUsedTriggerMask)
        {
            fTriggersAwaitingHandling &=~ fLastUsedTriggerMask;
            if (fTriggeredEventHandlers[fLastUsedTriggerNum] != NULL)
            {
            	//处理用户触发事件
                (*fTriggeredEventHandlers[fLastUsedTriggerNum])(fTriggeredEventClientDatas[fLastUsedTriggerNum]);
            }
        }
        else
        {
            
            unsigned i = fLastUsedTriggerNum;
            EventTriggerId mask = fLastUsedTriggerMask;

            do
            {
                i = (i+1)%MAX_NUM_EVENT_TRIGGERS;
                mask >>= 1;
                if (mask == 0) mask = 0x80000000;

                if ((fTriggersAwaitingHandling&mask) != 0)
                {
                    fTriggersAwaitingHandling &=~ mask;
                    if (fTriggeredEventHandlers[i] != NULL)
                    {
                        (*fTriggeredEventHandlers[i])(fTriggeredEventClientDatas[i]);
                    }

                    fLastUsedTriggerMask = mask;
                    fLastUsedTriggerNum = i;
                    break;
                }
            }
            while (i != fLastUsedTriggerNum);
        }
    }
	//处理delaytask
    fDelayQueue.handleAlarm();
}

//向调度器添加需要监控的socket以及需要监控的触发条件和满足触发条件的回调函数
void BasicTaskScheduler
::setBackgroundHandling(int socketNum, int conditionSet, BackgroundHandlerProc* handlerProc, void* clientData)
{
    if (socketNum < 0) return;

    FD_CLR((unsigned)socketNum, &fReadSet);
    FD_CLR((unsigned)socketNum, &fWriteSet);
    FD_CLR((unsigned)socketNum, &fExceptionSet);
    if (conditionSet == 0)
    {
        fHandlers->clearHandler(socketNum);
        if (socketNum+1 == fMaxNumSockets)
        {
            --fMaxNumSockets;
        }
    }
    else
    {
        fHandlers->assignHandler(socketNum, conditionSet, handlerProc, clientData);
        if (socketNum+1 > fMaxNumSockets)
        {
            fMaxNumSockets = socketNum+1;
        }
        if (conditionSet&SOCKET_READABLE) FD_SET((unsigned)socketNum, &fReadSet);
        if (conditionSet&SOCKET_WRITABLE) FD_SET((unsigned)socketNum, &fWriteSet);
        if (conditionSet&SOCKET_EXCEPTION) FD_SET((unsigned)socketNum, &fExceptionSet);
    }
}
//取消老的socket，变换为新的socket
void BasicTaskScheduler::moveSocketHandling(int oldSocketNum, int newSocketNum)
{
    if (oldSocketNum < 0 || newSocketNum < 0) return; 

    if (FD_ISSET(oldSocketNum, &fReadSet))
    {
        FD_CLR((unsigned)oldSocketNum, &fReadSet);
        FD_SET((unsigned)newSocketNum, &fReadSet);
    }
    if (FD_ISSET(oldSocketNum, &fWriteSet))
    {
        FD_CLR((unsigned)oldSocketNum, &fWriteSet);
        FD_SET((unsigned)newSocketNum, &fWriteSet);
    }
    if (FD_ISSET(oldSocketNum, &fExceptionSet))
    {
        FD_CLR((unsigned)oldSocketNum, &fExceptionSet);
        FD_SET((unsigned)newSocketNum, &fExceptionSet);
    }
    fHandlers->moveHandler(oldSocketNum, newSocketNum);

    if (oldSocketNum+1 == fMaxNumSockets)
    {
        --fMaxNumSockets;
    }
    if (newSocketNum+1 > fMaxNumSockets)
    {
        fMaxNumSockets = newSocketNum+1;
    }
}
