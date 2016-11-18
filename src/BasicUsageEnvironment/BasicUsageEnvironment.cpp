#include "BasicUsageEnvironment.h"
#include <stdio.h>


//�������Ĺ�����
BasicUsageEnvironment::BasicUsageEnvironment(TaskScheduler& taskScheduler)
    : BasicUsageEnvironment0(taskScheduler)
{
}

BasicUsageEnvironment::~BasicUsageEnvironment()
{
}
//�����������Ĺ����߻���ע���һ��������һ�������������Կ���ͨ��BasicUsageEnvironmentȥ�������ǵĵ�����
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
