#include "pch.h"
#include <framegen/dlssg/Sm86Settings.h>
#include <fstream>
#include <iostream>
#include <stdexcept>

static void Check(bool value, const char* message)
{
    if (!value)
        throw std::runtime_error(message);
}

static void Write(const std::filesystem::path& path, const std::string& text)
{
    std::ofstream stream(path, std::ios::binary);
    stream << text;
    Check(stream.good(), "test fixture write failed");
}

static std::string Read(const std::filesystem::path& path)
{
    std::ifstream stream(path, std::ios::binary);
    return { std::istreambuf_iterator<char>(stream), {} };
}

int wmain(int argc, wchar_t** argv)
{
    try
    {
        Check(argc == 2, "expected temporary test directory");
        auto root = std::filesystem::path(argv[1]) / (L"settings-" + std::to_wstring(GetCurrentProcessId()));
        Check(std::filesystem::create_directory(root), "test directory must be new");
        auto path = root / L"dlssg_sm86.ini";
        const std::string fixture =
            "; keep this comment\n[General]\nEnabled=0\n[FrameGeneration]\nOptimized=1\nMaxGeneratedFrames=3\n"
            "[Compatibility]\nPreset=Auto\nRouter=Auto\n[Logging]\nLevel=1\nDirectory=logs\\custom\n"
            "[Runtime]\nMode=Bundled\nCacheDirectory=custom-cache\n[Custom]\nFutureKey=preserve-me\n";
        Write(path, fixture);
        Sm86::Document document;
        std::string error;
        Check(Sm86::ReadSettings(path, document, error), "read factory settings");
        Check(!document.settings.enabled && document.settings.maxGeneratedFrames == 3, "safe factory defaults");
        document.settings.enabled = true;
        document.settings.optimized = 2;
        document.settings.maxGeneratedFrames = 5;
        document.settings.preset = "B";
        document.settings.logLevel = 3;
        Check(Sm86::SaveSettings(path, document, error), "save edited settings");
        Sm86::Document reread;
        Check(Sm86::ReadSettings(path, reread, error) && reread.settings == document.settings, "round trip");
        for (auto token : { "keep this comment", "Router", "custom-cache", "preserve-me", "logs\\custom" })
            Check(Read(path).find(token) != std::string::npos, "preserve unknown keys/comments/paths");

        auto previous = Read(path);
        document.settings.maxGeneratedFrames = 6;
        Check(!Sm86::SaveSettings(path, document, error) && Read(path) == previous, "invalid values must not write");
        document.settings.maxGeneratedFrames = 5;
        Write(path, previous + "\n; external edit\n");
        Check(!Sm86::SaveSettings(path, document, error), "detect concurrent external edits");
        Check(Read(path).find("external edit") != std::string::npos, "external edit retained");
        Check(Sm86::ReadSettings(path, document, error), "reload external edits");
        Check(Sm86::SaveSettings(path, document, error), "save after explicit reload");

        previous = Read(path);
        auto temporary = path;
        temporary += L".aurora-" + std::to_wstring(GetCurrentProcessId()) + L".tmp";
        Write(temporary, "interrupted-save");
        Check(!Sm86::SaveSettings(path, document, error) && Read(path) == previous, "failed temporary creation preserves INI");
        Check(Read(temporary) == "interrupted-save", "do not overwrite an existing temporary file");
        std::filesystem::remove(temporary);

        Check(SetFileAttributesW(path.c_str(), FILE_ATTRIBUTE_READONLY), "set read-only fixture");
        Check(!Sm86::SaveSettings(path, document, error), "report read-only save failure");
        Check(Read(path) == previous && !std::filesystem::exists(temporary), "failed replace preserves original and cleans own temp");
        Check(SetFileAttributesW(path.c_str(), FILE_ATTRIBUTE_NORMAL), "restore fixture attributes");

        Write(path, "[FrameGeneration]\nOptimized=99\n");
        auto oldSettings = document.settings;
        Check(!Sm86::ReadSettings(path, document, error) && document.settings == oldSettings, "bad INI cannot partially replace document");
        Check(!Sm86::ReadSettings(root / L"missing.ini", document, error), "missing INI reported");
        Check(!std::filesystem::exists(root / L"missing.ini"), "reading must not create INI");
        std::cout << "PASS: defaults, round-trip, unknown keys, external edits, validation, atomic-save failures\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
