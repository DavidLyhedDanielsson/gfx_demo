#include "path.hpp"

namespace Path
{
    std::filesystem::path getAssetPath()
    {
        return std::filesystem::path(ROOT_DIR_ASSETS);
    }

    std::filesystem::path getAssetPath(const std::filesystem::path& name)
    {
        return std::filesystem::path(ROOT_DIR_ASSETS) / name;
    }

    std::filesystem::path getShaderPath()
    {
        return std::filesystem::path(ROOT_DIR_SHADERS);
    }

    std::filesystem::path getShaderPath(const std::filesystem::path& name)
    {
        return getShaderPath() / name;
    }
};
