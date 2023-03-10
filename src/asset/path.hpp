#pragma once

#include <filesystem>

namespace Path
{
    std::filesystem::path getAssetPath();
    std::filesystem::path getAssetPath(const std::filesystem::path& name);

    std::filesystem::path getShaderPath();
    std::filesystem::path getShaderPath(const std::filesystem::path& name);
}
