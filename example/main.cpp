#include<BasicUsageEnvironment.h>

int main()
{
	//�������������õ����������ʱʱ��1��
	TaskScheduler* scheduler =BasicTaskScheduler::createNew(1000000);
	UsageEnvironment* env= BasicUsageEnvironment::createNew(*scheduler);
	char eventLoopflag=0;
	//����������������ѭ��
	env->taskScheduler().doEventLoop(&eventLoopflag);
	//��Զ���ᵽ����
	return 0;
}
