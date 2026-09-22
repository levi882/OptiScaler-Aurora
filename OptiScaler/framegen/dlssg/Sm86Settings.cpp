#include "pch.h"
#include "Sm86Settings.h"

#include <SimpleIni.h>
#include <charconv>
#include <fstream>
#include <iterator>

namespace
{
bool ReadBytes(const std::filesystem::path& path, std::string& bytes)
{
    std::ifstream stream(path, std::ios::binary);
    if (!stream)
        return false;
    bytes.assign(std::istreambuf_iterator<char>(stream), {});
    return !stream.bad();
}

bool ReadInt(const CSimpleIniA& ini, const char* section, const char* key, int fallback, int minimum,
             int maximum, int& value)
{
    const char* text = ini.GetValue(section, key, nullptr);
    value = fallback;
    if (!text)
        return true;
    auto end = text + strlen(text);
    auto result = std::from_chars(text, end, value);
    return result.ec == std::errc() && result.ptr == end && value >= minimum && value <= maximum;
}
} // namespace

bool Sm86::ReadSettings(const std::filesystem::path& path, Document& document, std::string& error)
{
    Document next;
    CSimpleIniA ini(true);
    int enabled = 0;
    if (!ReadBytes(path, next.original) || ini.LoadData(next.original) < 0)
    {
        error = "Cannot read dlssg_sm86.ini";
        return false;
    }
    if (!ReadInt(ini, "General", "Enabled", 0, 0, 1, enabled) ||
        !ReadInt(ini, "FrameGeneration", "Optimized", 0, 0, 3, next.settings.optimized) ||
        !ReadInt(ini, "FrameGeneration", "MaxGeneratedFrames", 3, 1, 5, next.settings.maxGeneratedFrames) ||
        !ReadInt(ini, "Logging", "Level", 1, 0, 3, next.settings.logLevel))
    {
        error = "Invalid SM86 setting; check Enabled, Optimized, MaxGeneratedFrames and Logging/Level";
        return false;
    }
    next.settings.enabled = enabled != 0;
    auto preset = ini.GetValue("Compatibility", "Preset", "Auto");
    if (_stricmp(preset, "Auto") == 0)
        next.settings.preset = "Auto";
    else if (_stricmp(preset, "A") == 0 || _stricmp(preset, "B") == 0)
        next.settings.preset = _stricmp(preset, "A") == 0 ? "A" : "B";
    else
    {
        error = "Invalid SM86 Compatibility/Preset";
        return false;
    }
    document = std::move(next);
    error.clear();
    return true;
}

bool Sm86::SaveSettings(const std::filesystem::path& path, Document& document, std::string& error)
{
    auto& settings = document.settings;
    if (settings.optimized < 0 || settings.optimized > 3 || settings.maxGeneratedFrames < 1 ||
        settings.maxGeneratedFrames > 5 || settings.logLevel < 0 || settings.logLevel > 3 ||
        (settings.preset != "Auto" && settings.preset != "A" && settings.preset != "B"))
    {
        error = "Invalid SM86 settings";
        return false;
    }

    std::string current;
    if (!ReadBytes(path, current) || current != document.original)
    {
        error = "The INI changed outside the menu; reload it before saving";
        return false;
    }
    CSimpleIniA ini(true);
    if (ini.LoadData(current) < 0 ||
        ini.SetLongValue("General", "Enabled", settings.enabled ? 1 : 0) < 0 ||
        ini.SetLongValue("FrameGeneration", "Optimized", settings.optimized) < 0 ||
        ini.SetLongValue("FrameGeneration", "MaxGeneratedFrames", settings.maxGeneratedFrames) < 0 ||
        ini.SetValue("Compatibility", "Preset", settings.preset.c_str()) < 0 ||
        ini.SetLongValue("Logging", "Level", settings.logLevel) < 0)
    {
        error = "Cannot serialize SM86 settings";
        return false;
    }
    std::string serialized;
    if (ini.Save(serialized, true) < 0)
    {
        error = "Cannot serialize SM86 settings";
        return false;
    }

    // Replace only after a complete write; a failed save must leave the old configuration intact.
    auto temporary = path;
    temporary += L".aurora-" + std::to_wstring(GetCurrentProcessId()) + L".tmp";
    HANDLE file = CreateFileW(temporary.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW,
                              FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE)
    {
        error = "Cannot create the temporary INI (permissions or an interrupted previous save)";
        return false;
    }
    DWORD written = 0;
    bool ok = WriteFile(file, serialized.data(), static_cast<DWORD>(serialized.size()), &written, nullptr) &&
              written == serialized.size() && FlushFileBuffers(file);
    CloseHandle(file);
    if (ok)
        ok = ReadBytes(path, current) && current == document.original;
    if (ok)
        ok = MoveFileExW(temporary.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH);
    if (!ok)
    {
        DeleteFileW(temporary.c_str());
        error = "Cannot replace the INI; check permissions or reload external changes";
        return false;
    }
    document.original = std::move(serialized);
    error.clear();
    return true;
}
