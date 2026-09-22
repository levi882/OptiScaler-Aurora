#include "pch.h"
#include <iostream>

int wmain(int argc, wchar_t** argv)
{
    if (argc != 2 && argc != 3)
        return 2;
    HMODULE proxy = LoadLibraryExW(argv[1], nullptr, 0);
    if (!proxy)
    {
        std::cerr << "Load failed: " << GetLastError() << '\n';
        return 1;
    }
    for (auto name : { "DlssgProxy_Name", "DlssgProxy_Role", "GetFileVersionInfoW", "VerQueryValueW" })
    {
        if (!GetProcAddress(proxy, name))
        {
            std::cerr << "Missing export: " << name << '\n';
            return 1;
        }
    }
    std::cout << "PASS: SM86 proxy loads from a subdirectory with expected exports\n";
    if (argc == 3)
    {
        auto runtime = LoadLibraryExW(argv[2], nullptr, 0);
        if (!runtime)
        {
            std::cerr << "Runtime load failed: " << GetLastError() << '\n';
            return 1;
        }
        wchar_t loadedPath[32768] {};
        if (!GetModuleFileNameW(runtime, loadedPath, 32768))
            return 1;
        std::wcout << L"Runtime path: " << loadedPath << L'\n';
        if (std::wstring(loadedPath).find(L"sm86-test-cache") == std::wstring::npos)
        {
            std::cerr << "Runtime was not redirected to the configured SM86 bundle cache\n";
            return 1;
        }
        std::cout << "PASS: NVIDIA runtime request redirected through the SM86 proxy\n";
    }
    // Let process shutdown handle installed hooks; do not hot-unload the proxy.
    return 0;
}
