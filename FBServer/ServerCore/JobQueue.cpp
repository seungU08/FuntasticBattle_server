#include "pch.h"
#include "JobQueue.h"

void JobQueue::Push(JobRef job, bool pushOnly)
{
    int32 prevCount = _jobCount.fetch_add(1);
    {
        WRITE_LOCK;
        _jobs.push(job);
    }

    // pushOnly가 아니고 첫 번째 진입자라면 직접 Execute
    if (pushOnly == false && prevCount == 0)
    {
        Execute();
    }
}

void JobQueue::Execute()
{
    while (true)
    {
        vector<JobRef> jobs;
        {
            WRITE_LOCK;
            int32 count = static_cast<int32>(_jobs.size());
            if (count == 0)
                break;
            jobs.reserve(count);
            while (_jobs.empty() == false)
            {
                jobs.push_back(_jobs.front());
                _jobs.pop();
            }
        }

        for (JobRef& job : jobs)
        {
            job->Execute();
        }

        // 처리한 만큼 카운터 감소; 0이 되면 종료
        int32 remaining = _jobCount.fetch_sub(static_cast<int32>(jobs.size()));
        if (remaining == static_cast<int32>(jobs.size()))
            break;
    }
}
