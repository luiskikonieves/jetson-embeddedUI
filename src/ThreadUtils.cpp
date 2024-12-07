#include <sys/syscall.h>
#include <unistd.h>
#include "ThreadUtils.h"

pthread_t ThreadUtils::startThread
(
    const char*            name,
    threadFunc_t           threadFunc,
    void*                  arg,
    std::vector<unsigned>& cores,
    bool                   joinable,
    bool                   inherit_sched,
    int                    priority,
    int                    policy
)
{
    pthread_attr_t     attr;
    cpu_set_t          cpuset;
    int                ret;
    struct sched_param sched_params = {};

    sched_params.sched_priority = priority;

    // Set desired core set.
    //
    CPU_ZERO(&cpuset);
    for (unsigned ii = 0; ii < cores.size(); ii++)
    {
        CPU_SET(cores[ii], &cpuset);
    }

    // Initialize the pthread attributes.
    //
    ret = pthread_attr_init(&attr);
    if (ret != 0)
    {
        printf( "ThreadUtils::startThread pthread_attr_init FAIL %d for %s",
            ret, name);
    }

    // Set the inherited scheduling policy.
    //
    ret = pthread_attr_setinheritsched(&attr, inherit_sched ? PTHREAD_INHERIT_SCHED : PTHREAD_EXPLICIT_SCHED);
    if (ret != 0)
    {
        printf( "ThreadUtils::startThread pthread_attr_setinheritsched FAIL %d for %s",
            ret, name);
    }

    // Set the detached state.
    //
    ret = pthread_attr_setdetachstate(&attr, joinable ? PTHREAD_CREATE_JOINABLE : PTHREAD_CREATE_DETACHED);
    if (ret != 0)
    {
        printf( "ThreadUtils::startThread pthread_attr_setdetachstate FAIL %d for %s",
            ret, name);
    }

    // Set the scheduling policy.
    //
    ret = pthread_attr_setschedpolicy(&attr, policy);
    if (ret != 0)
    {
        printf( "ThreadUtils::startThread pthread_attr_setschedpolicy FAIL %d for %s",
            ret, name);
    }

    // Set the priority.
    //
    ret = pthread_attr_setschedparam(&attr, &sched_params);
    if (ret != 0)
    {
        printf( "ThreadUtils::startThread pthread_attr_setschedparam FAIL %d for %s",
            ret, name);
    }

    // Set the affinity.
    //
    ret = pthread_attr_setaffinity_np(&attr, sizeof cpuset, &cpuset);
    if (ret != 0)
    {
        printf( "ThreadUtils::startThread pthread_attr_setaffinity_np FAIL %d for %s",
            ret, name);
    }

    // Create the thread.
    pthread_t pthread;
    ret = pthread_create(&pthread, &attr, threadFunc, arg);

    if (ret == 0)
    {
        // Set the name.
        pthread_setname_np(pthread, name);
    }
    else
    {
        pthread = INVALID_PTHREAD;
        printf( "ThreadUtils::startThread pthread_create FAIL return %d for %s\n", ret, name);
    }

    // Destroy attribute.
    pthread_attr_destroy(&attr);

    return pthread;
}
