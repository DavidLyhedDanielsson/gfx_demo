#pragma once

#include "job.hpp"

#include <cassert>
#include <filesystem>
#include <future>

#include <util/file_util.hpp>

class IOJob: public Job
{
  public:
    using Result = std::vector<char>;
    constexpr static int ID = __COUNTER__;

    struct Parameters
    {
        constexpr static int ID = IOJob::ID;

        const std::filesystem::path path;
    };

  private:
    friend AssetLoader;

    std::filesystem::path path;
    std::promise<Result> promise;

  public:
    IOJob(JobHandle handle, Parameters parameters): Job(handle), path(parameters.path) {}
    IOJob(IOJob&&) = default;
    IOJob& operator=(IOJob&&) = default;

    void run()
    {
        std::optional<std::vector<char>> data = FileUtil::readFile(path);
        assert(data.has_value()); // TODO: Handle this
        promise.set_value(data.value());
    }

    const std::filesystem::path& getPath() const
    {
        return path;
    }
};
