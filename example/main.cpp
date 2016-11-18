#include<BasicUsageEnvironment.h>

int main()
{
	//创建调度器，该调度器最大延时时间1秒
	TaskScheduler* scheduler =BasicTaskScheduler::createNew(1000000);
	UsageEnvironment* env= BasicUsageEnvironment::createNew(*scheduler);
	char eventLoopflag=0;
	//启动调度器，进入循环
	env->taskScheduler().doEventLoop(&eventLoopflag);
	//永远不会到这里
	return 0;
}
