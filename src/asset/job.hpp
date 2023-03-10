#pragma once

#include "asset/job_handle.hpp"
#include "job_handle.hpp"

class Job
{
    friend class AssetLoader;

  private:
    JobHandle handle = -1;

  public:
    Job(JobHandle handle): handle(handle) {}
    Job(Job&&) = default;
    Job& operator=(Job&&) = default;
    virtual ~Job(){};

    JobHandle getHandle() const
    {
        return handle;
    }

    virtual void run() = 0;
};
