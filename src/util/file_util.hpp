#pragma once

#include <filesystem>
#include <optional>
#include <vector>

namespace FileUtil
{
std::optional<std::vector<char>> readFile(const std::filesystem::path& path);
}