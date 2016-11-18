#include "BasicUsageEnvironment.h"
#include <stdio.h>


//调度器的管理者
BasicUsageEnvironment::BasicUsageEnvironment(TaskScheduler& taskScheduler)
    : BasicUsageEnvironment0(taskScheduler)
{
}

BasicUsageEnvironment::~BasicUsageEnvironment()
{
}
//创建调度器的管理者环境注意第一个参数是一个调度器，所以可以通过BasicUsageEnvironment去调度我们的调度器
BasicUsageEnvironment*
BasicUsageEnvironment::createNew(TaskScheduler& taskScheduler)
{
    return new BasicUsageEnvironment(taskScheduler);
}

int BasicUsageEnvironment::getErrno() const
{
    return errno;
}

UsageEnvironment& BasicUsageEnvironment::operator<<(char const* str)
{
    if (str == NULL) str = "(NULL)";
    fprintf(stderr, "%s", str);
    return *this;
}

UsageEnvironment& BasicUsageEnvironment::operator<<(int i)
{
    fprintf(stderr, "%d", i);
    return *this;
}

UsageEnvironment& BasicUsageEnvironment::operator<<(unsigned u)
{
    fprintf(stderr, "%u", u);
    return *this;
}

UsageEnvironment& BasicUsageEnvironment::operator<<(double d)
{
    fprintf(stderr, "%f", d);
    return *this;
}

UsageEnvironment& BasicUsageEnvironment::operator<<(void* p)
{
    fprintf(stderr, "%p", p);
    return *this;
}
