#include <BasicUsageEnvironment.h>
#include <HandlerSet.h>
#include <stdio.h>


//��̬��������BasicTaskScheduler������Ķ���
BasicTaskScheduler* BasicTaskScheduler::createNew(unsigned maxSchedulerGranularity)
{
    return new BasicTaskScheduler(maxSchedulerGranularity);
}
//���캯�������������������socket��������set
BasicTaskScheduler::BasicTaskScheduler(unsigned maxSchedulerGranularity)
    : fMaxSchedulerGranularity(maxSchedulerGranularity), fMaxNumSockets(0)

{
    FD_ZERO(&fReadSet);
    FD_ZERO(&fWriteSet);
    FD_ZERO(&fExceptionSet);
	//���maxSchedulerGranularity����0,���ڵ������������һ��ʱ�����񣬱�֤������������Զ����
    if (maxSchedulerGranularity > 0) schedulerTickTask();
}

BasicTaskScheduler::~BasicTaskScheduler()
{
}
//ʱ�Ӻ�������֤��������������ʱ�Ӻ�����������ݹ麯����
void BasicTaskScheduler::schedulerTickTask(void* clientData)
{
    ((BasicTaskScheduler*)clientData)->schedulerTickTask();
}
//ʱ�Ӻ���
void BasicTaskScheduler::schedulerTickTask()
{
	printf("the scheduler test tick...\n");
    scheduleDelayedTask(fMaxSchedulerGranularity, schedulerTickTask, this);
}

#ifndef MILLION
#define MILLION 1000000
#endif
//������Ⱥ�����һ��ѭ�����ȣ�maxDelayTime�Ǵ��ݵ������ʱʱ�䣬��֤select���᳤��������
void BasicTaskScheduler::SingleStep(unsigned maxDelayTime)
{
    fd_set readSet = fReadSet;
    fd_set writeSet = fWriteSet; 
    fd_set exceptionSet = fExceptionSet; 
	//��delayqueue����ȡ��һ����ʱʱ��
    DelayInterval const& timeToDelay = fDelayQueue.timeToNextAlarm();
    struct timeval tv_timeToDelay;
    tv_timeToDelay.tv_sec = timeToDelay.seconds();
    tv_timeToDelay.tv_usec = timeToDelay.useconds();
    const long MAX_TV_SEC = MILLION;
    if (tv_timeToDelay.tv_sec > MAX_TV_SEC)
    {
        tv_timeToDelay.tv_sec = MAX_TV_SEC;
    }
    // ͬʱ���ܴ������Ǵ�����������ʱ�򴫵ݵ������ʱʱ�䱣֤��������ʵʱ��:
    if (maxDelayTime > 0 &&
        (tv_timeToDelay.tv_sec > (long)maxDelayTime/MILLION ||
         (tv_timeToDelay.tv_sec == (long)maxDelayTime/MILLION &&
          tv_timeToDelay.tv_usec > (long)maxDelayTime%MILLION)))
    {
        tv_timeToDelay.tv_sec = maxDelayTime/MILLION;
        tv_timeToDelay.tv_usec = maxDelayTime%MILLION;
    }
	//select�����ȴ������¼�
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
			//����ע��õ�socket����֮��Ļص�����
            (*handler->handlerProc)(handler->clientData, resultConditionSet);
            break;
        }
    }
	//������η�ֹ©��
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

    //����Ƿ����û������¼�
    if (fTriggersAwaitingHandling != 0)
    {
        if (fTriggersAwaitingHandling == fLastUsedTriggerMask)
        {
            fTriggersAwaitingHandling &=~ fLastUsedTriggerMask;
            if (fTriggeredEventHandlers[fLastUsedTriggerNum] != NULL)
            {
            	//�����û������¼�
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
	//����delaytask
    fDelayQueue.handleAlarm();
}

//������������Ҫ��ص�socket�Լ���Ҫ��صĴ������������㴥�������Ļص�����
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
//ȡ���ϵ�socket���任Ϊ�µ�socket
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
