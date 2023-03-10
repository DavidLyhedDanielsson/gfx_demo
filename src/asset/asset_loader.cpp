#include "asset_loader.hpp"
#include "io_job.hpp"

#include <chrono>
#include <cstddef>
#include <mutex>
#include <optional>
#include <stop_token>
#include <string>
#include <variant>

#include <stl_util/functional.hpp>

static thread_local bool suspend = false;
static thread_local std::optional<Loader> currentLoader;

void AssetLoader::runLoadThread(std::stop_token stopToken)
{
    // Give some time for everything to initialize
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    while(!stopToken.stop_requested())
    {
        std::optional<std::variant<std::unique_ptr<Job>, StartLoaderJob, ContinueLoaderJob>> jobVarOpt;
        this->jobs.hold([&](std::unique_lock<std::mutex>& lock, std::condition_variable& condition, Jobs& jobs) {
            condition.wait(lock, [&]() {
                return jobs.jobs.size() > 0 || jobs.startLoadJobs.size() > 0 || jobs.continueLoadJobs.size() > 0
                       || stopToken.stop_requested();
            });

            if(stopToken.stop_requested())
                return NotifyType::NOTIFY_NONE;

            if(jobs.jobs.size() > 0)
            {
                jobVarOpt = std::move(jobs.jobs.front());
                jobs.jobs.erase(jobs.jobs.begin());
            }
            else if(jobs.startLoadJobs.size() > 0)
            {
                jobVarOpt = std::move(jobs.startLoadJobs.front());
                jobs.startLoadJobs.erase(jobs.startLoadJobs.begin());
            }
            else if(jobs.continueLoadJobs.size() > 0)
            {
                // Shouldn't deadlock, teehee
                bool hasDependency = false;
                this->dependencies.modify([&](std::map<JobHandle, std::vector<JobHandle>>& deps) {
                    auto iter = deps.find(jobs.continueLoadJobs.front().loader.id);
                    if(iter != deps.end())
                    {
                        if(!iter->second.empty())
                            hasDependency = true;
                    }
                });

                if(!hasDependency)
                {
                    jobVarOpt = std::move(jobs.continueLoadJobs.front());
                    jobs.continueLoadJobs.erase(jobs.continueLoadJobs.begin());
                }
            }

            return NotifyType::NOTIFY_NONE;
        });

        if(!jobVarOpt.has_value())
            continue;

        auto jobVar = std::move(jobVarOpt.value());
        if(std::holds_alternative<std::unique_ptr<Job>>(jobVar))
        {
            auto job = std::get<std::unique_ptr<Job>>(std::move(jobVar));
            job->run();
            this->completedJobs.modify(
                [&](std::set<JobHandle>& completedJobs) { assert(completedJobs.insert(job->getHandle()).second); });

            NotifyType notifyType = NotifyType::NOTIFY_NONE;
            this->dependencies.modify([&](std::map<JobHandle, std::vector<JobHandle>>& deps) {
                for(auto& [_, values] : deps)
                {
                    auto iter = std::find(values.begin(), values.end(), job->getHandle());
                    if(iter != values.end())
                    {
                        values.erase(iter);

                        if(notifyType == NotifyType::NOTIFY_NONE)
                            notifyType = NotifyType::NOTIFY_ONE;
                        else
                            notifyType = NotifyType::NOTIFY_ALL;
                    }
                }
            });

            this->jobs.notify(notifyType);
        }
        else if(std::holds_alternative<StartLoaderJob>(jobVar) || std::holds_alternative<ContinueLoaderJob>(jobVar))
        {
            if(std::holds_alternative<StartLoaderJob>(jobVar))
            {
                auto [id, loaderGen, path] = std::get<StartLoaderJob>(jobVar);
                currentLoader = loaderGen(*this, path);
                currentLoader->id = this->handleCounter.fetch_add(1);
            }
            else if(std::holds_alternative<ContinueLoaderJob>(jobVar))
            {
                currentLoader = std::move(std::get<ContinueLoaderJob>(jobVar).loader);
            }
            else
                assert(false);

            while(!suspend && currentLoader.value().run())
            {
            }

            if(suspend)
            {
                this->jobs.modify([&](Jobs& jobs) {
                    jobs.continueLoadJobs.push_back(ContinueLoaderJob{.loader = std::move(currentLoader.value())});
                    return NotifyType::NOTIFY_NONE;
                });
            }
        }

        // Placed here as safeguards so a thread won't suspend forever
        currentLoader = std::nullopt;
        suspend = false;
    }
}

AssetLoader::AssetLoader()
{
    for(auto& thread : loadThreads)
    {
        thread = std::jthread([&](std::stop_token stopToken) { runLoadThread(stopToken); });
    }
}

void AssetLoader::suspendThread()
{
    suspend = true;
}

void AssetLoader::load(const std::filesystem::path& path, LoaderGenerator generator)
{
    jobs.modify([&](Jobs& jobs) {
        jobs.startLoadJobs.push_back(StartLoaderJob{
            .id = handleCounter.fetch_add(1),
            .generator = generator,
            .path = path,
        });
        return NotifyType::NOTIFY_ONE;
    });
}

void AssetLoader::addDependencyTo(JobHandle jobId)
{
    dependencies.modify([=](std::map<JobHandle, std::vector<JobHandle>>& deps) {
        auto iter = deps.find(currentLoader->id);
        if(iter == deps.end())
            deps.insert(std::make_pair(currentLoader->id, std::vector<JobHandle>({jobId})));
        else
            iter->second.push_back(jobId);
    });
}

bool AssetLoader::hasCompletedJob(JobHandle jobId)
{
    bool completed = false;
    this->completedJobs.view(
        [&](const std::set<JobHandle>& completedJobs) { completed = completedJobs.contains(jobId); });
    return completed;
}

void AssetLoader::suspendCurrentLoader()
{
    suspend = true;
}

void AssetLoader::stopRunning()
{
    for(auto& thread : loadThreads)
        thread.request_stop();
    jobs.notify(NotifyType::NOTIFY_ALL);
}

void AssetLoader::wait()
{
    for(auto& thread : loadThreads)
        thread.join();
}

void AssetLoader::quickExit()
{
    stopRunning();
    for(auto& thread : loadThreads)
        thread.detach();
}
