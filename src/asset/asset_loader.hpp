#pragma once

#include <atomic>
#include <cassert>
#include <condition_variable>
#include <coroutine>
#include <cstring>
#include <exception>
#include <filesystem>
#include <functional>
#include <future>
#include <map>
#include <memory>
#include <optional>
#include <queue>
#include <semaphore>
#include <set>
#include <thread>
#include <variant>

#include <util/concurrent_data.hpp>

#include <entt/entity/fwd.hpp> // ???
#include <entt/entt.hpp>

#include "asset.hpp"
#include "io_job.hpp"
#include "job_handle.hpp"
#include "loader.hpp"

#ifndef __COUNTER__
    #error __COUNTER__ macro not available
#endif

class AssetLoader;

template<typename T>
class JobAwaitable
{
    AssetLoader& assetLoader;
    JobHandle handle;
    std::future<typename T::Result> res;

  public:
    JobAwaitable(AssetLoader& assetLoader, JobHandle handle, std::future<typename T::Result>&& res)
        : assetLoader(assetLoader)
        , handle(handle)
        , res(std::move(res))
    {
    }
    JobAwaitable(JobAwaitable&&) = default;

    bool await_ready()
    {
        return assetLoader.HasCompletedJob(this->handle);
    }
    void await_suspend(std::coroutine_handle<> handle)
    {
        assetLoader.SuspendCurrentLoader();
        assetLoader.AddDependencyTo(this->handle);
    }

    typename T::Result await_resume()
    {
        return res.get();
    }
};

class AssetLoader
{
  public:
    using LoaderGenerator = Loader (*)(AssetLoader&, const std::filesystem::path&);

  private:
    std::atomic<uint32_t> handleCounter = 0;

    static constexpr int LOAD_THREAD_COUNT = 4;
    std::array<std::jthread, LOAD_THREAD_COUNT> loadThreads;

    struct StartLoaderJob
    {
        JobHandle id;
        LoaderGenerator generator;
        std::filesystem::path path;
    };

    struct ContinueLoaderJob
    {
        Loader loader;
    };

    struct Jobs
    {
        std::vector<std::unique_ptr<Job>> jobs;
        std::vector<StartLoaderJob> startLoadJobs;
        std::vector<ContinueLoaderJob> continueLoadJobs;
    };
    WaitableConcurrentData<Jobs> jobs;
    ConcurrentData<std::map<JobHandle, std::vector<JobHandle>>> dependencies;
    ConcurrentData<std::set<JobHandle>> completedJobs;

    std::map<int, LoaderGenerator> loaders;

    void runLoadThread(std::stop_token stopToken);
    void suspendThread();

    void load(const std::filesystem::path& path, LoaderGenerator generator);

  public:
    AssetLoader();

    template<typename Type>
    void registerLoader(LoaderGenerator generator)
    {
        assert(!loaders.contains(Type::ID));
        loaders.insert(std::make_pair(Type::ID, generator));
    }

    template<typename Type>
    void load(const std::filesystem::path& path)
    {
        auto iter = loaders.find(Type::ID);
        assert(iter != loaders.end());

        load(path, iter->second);
    }

    template<typename JobType>
    JobAwaitable<JobType> start(const typename JobType::Parameters& parameters)
    {
        suspendThread();

        JobHandle jobHandle{this->handleCounter.fetch_add(1)};
        auto newJob = std::make_unique<JobType>(jobHandle, parameters);
        std::future<typename JobType::Result> future = newJob->promise.get_future();

        jobs.modify([&](Jobs& jobs) {
            jobs.jobs.push_back(std::move(newJob));
            return NotifyType::NOTIFY_ONE;
        });

        return JobAwaitable<JobType>(*this, jobHandle, std::move(future));
    }

    void suspendCurrentLoader();
    void addDependencyTo(JobHandle jobId);

    bool hasCompletedJob(JobHandle jobId);

    void stopRunning();
    void wait();
    /**
     * @brief Quickly exits by leaking all memory and detaching threads.
     *
     * Useful when exiting the application and a quick exit is preferred over
     * deallocation.
     */
    void quickExit();
};