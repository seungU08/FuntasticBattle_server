#pragma once
#include "Job.h"

class JobQueue : public RefCountable
{
public:
    void Push(JobRef job, bool pushOnly = false);
    void Execute();

private:
    USE_LOCK;
    queue<JobRef>       _jobs;
    Atomic<int32>       _jobCount = 0;
};

using JobQueueRef = TSharedPtr<JobQueue>;
