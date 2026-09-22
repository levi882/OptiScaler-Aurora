#pragma once

#include <filesystem>
#include <string>

namespace Sm86
{
struct Settings
{
    bool enabled = false;
    int optimized = 1;
    int maxGeneratedFrames = 3;
    std::string preset = "Auto";
    int logLevel = 1;
    bool operator==(const Settings&) const = default;
};

struct Document
{
    Settings settings;
    std::string original;
};

// Preserve third-party keys/comments and reject overwriting an externally edited file.
bool ReadSettings(const std::filesystem::path& path, Document& document, std::string& error);
bool SaveSettings(const std::filesystem::path& path, Document& document, std::string& error);
} // namespace Sm86
