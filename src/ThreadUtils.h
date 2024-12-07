/**
 * Provides thread management utilities in a multi-core environment.
 * It includes functionality to start threads with specific attributes such as core affinity,
 * scheduling policy, priority, and whether the thread is joinable or detached.
 * 
*/

#pragma once

#include <limits.h>
#include <pthread.h>
#include <string>
#include <vector>

#define INVALID_PTHREAD UINT_MAX

namespace ThreadUtils
{
    typedef void* (*threadFunc_t) (void*);

    pthread_t startThread
    (
        const char*            name,
        threadFunc_t           threadFunc,
        void*                  arg,
        std::vector<unsigned>& cores,
        bool                   joinable,
        bool                   inherit_sched,
        int                    priority,
        int                    policy
    );
}
