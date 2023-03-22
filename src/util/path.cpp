#include "path.hpp"

namespace Path
{
std::filesystem::path getAssetPath()
{
    return std::filesystem::path(ROOT_DIR_ASSET);
}

std::filesystem::path getAssetPath(const std::filesystem::path& name)
{
    return std::filesystem::path(ROOT_DIR_ASSET) / name;
}

std::filesystem::path getShaderPath()
{
    return std::filesystem::path(ROOT_DIR_SHADER);
}

std::filesystem::path getShaderPath(const std::filesystem::path& name)
{
    return getShaderPath() / name;
}

std::filesystem::path getRandomCat()
{
    char path[] = "catX.jpg";
    path[3] = '0' + (char)(rand() % 4);

    return getAssetPath(path);
}
};
