#include "pch.h"
#include <filesystem>
#include <iostream>
#include <string>

int wmain(int argc, wchar_t** argv)
{
    // Aurora path, component path, expected enabled flag, optional runtime request.
    if (argc != 4 && argc != 5)
        return 2;
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
    auto aurora = LoadLibraryExW(argv[1], nullptr, 0);
    if (!aurora)
    {
        std::cerr << "Aurora load failed: " << GetLastError() << std::endl;
        return 1;
    }
    bool expected = std::wstring(argv[3]) == L"1";
    auto component = GetModuleHandleW(argv[2]);
    if ((component != nullptr) != expected)
    {
        std::cerr << "Unexpected managed SM86 load state" << std::endl;
        return 1;
    }
    std::cout << "PASS: Aurora startup, managed SM86 loaded=" << expected << std::endl;
    if (argc == 5)
    {
        auto runtime = LoadLibraryExW(argv[4], nullptr, 0);
        wchar_t path[32768] {};
        if (!runtime || !GetModuleFileNameW(runtime, path, 32768))
            return 1;
        bool redirected = std::wstring(path).find(L"sm86-test-cache") != std::wstring::npos;
        std::wcout << L"Runtime path: " << path << std::endl;
        if (redirected != expected)
            return 1;
        std::cout << "PASS: Aurora loader chain preserves the expected runtime route" << std::endl;
    }
    return 0;
}
