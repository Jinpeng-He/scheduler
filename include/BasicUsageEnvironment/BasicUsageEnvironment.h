#ifndef _BASIC_USAGE_ENVIRONMENT_H
#define _BASIC_USAGE_ENVIRONMENT_H

#ifndef _BASIC_USAGE_ENVIRONMENT0_H
#include <BasicUsageEnvironment0.h>
#endif

//��������ʵ����
class BasicUsageEnvironment: public BasicUsageEnvironment0
{
public:
    static BasicUsageEnvironment* createNew(TaskScheduler& taskScheduler);

    virtual int getErrno() const;
	//���������أ�������Ϣ��׼���
    virtual UsageEnvironment& operator<<(char const* str);
    virtual UsageEnvironment& operator<<(int i);
    virtual UsageEnvironment& operator<<(unsigned u);
    virtual UsageEnvironment& operator<<(double d);
    virtual UsageEnvironment& operator<<(void* p);

protected:
	//�ú���ֻ�ܱ�createNew()�����������
    BasicUsageEnvironment(TaskScheduler& taskScheduler);
    virtual ~BasicUsageEnvironment();
};


//����������ʵ����
class BasicTaskScheduler: public BasicTaskScheduler0
{
public:
	//�����������ӿ�,maxSchedulerGranularity��λΪus����֤һ��ѭ���������ʱʱ��
    static BasicTaskScheduler* createNew(unsigned maxSchedulerGranularity = 10000/*microseconds*/);
    virtual ~BasicTaskScheduler();

protected:
	//�ú���ֻ�ܱ�createNew()�����������
    BasicTaskScheduler(unsigned maxSchedulerGranularity);

	//������ʱ��,��֤��û���¼������ʱ�򣬵�����ÿ��һ��ʱ�份��һ��
    static void schedulerTickTask(void* clientData);
    void schedulerTickTask();

protected:
    //����������ѭ����Ԫ
    virtual void SingleStep(unsigned maxDelayTime);
	//������Ҫ��������ص�socket,ͬʱָ����ػص������Ͳ���
    virtual void setBackgroundHandling(int socketNum, int conditionSet, BackgroundHandlerProc* handlerProc, void* clientData);
	//ȡ���ϵ�socket����µ�socket������
	virtual void moveSocketHandling(int oldSocketNum, int newSocketNum);

protected:
    unsigned fMaxSchedulerGranularity;
	//socket�����������ֵ
    int fMaxNumSockets;
	//socket�ɶ�
    fd_set fReadSet;
	//socket��д
    fd_set fWriteSet;
	//socket�쳣
    fd_set fExceptionSet;

private:
};

#endif
