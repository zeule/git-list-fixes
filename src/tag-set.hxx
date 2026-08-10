#pragma once

#include <filesystem>
#include <map>
#include <string>
#include <vector>

std::map<std::string, std::vector<std::string>> load_tag_set(const std::filesystem::path& filePath);
