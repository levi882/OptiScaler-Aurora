#include "pch.h"
#include "Sm86Integration.h"
#include "Sm86Settings.h"

#include <Util.h>
#include <menu/menu_common.h>
#include <proxies/Ntdll_Proxy.h>
#include <atomic>

namespace
{
std::atomic<bool> ownsRuntime { false };
HMODULE module = nullptr;
Sm86::Settings startup;
std::string startupError;

std::filesystem::path ComponentPath()
{
    return Util::DllPath().parent_path() / L"OptiScaler" / L"SM86";
}

bool IsProxy(HMODULE handle)
{
    // Check the identification exports without calling an undocumented third-party ABI.
    return handle && GetProcAddress(handle, "DlssgProxy_Name") && GetProcAddress(handle, "DlssgProxy_Role");
}

std::string Utf8(const wchar_t* text) { return wstring_to_string(std::wstring(text)); }
#define SM86_CN(text) Utf8(L##text).c_str()

void Help(const wchar_t* text)
{
    ImGui::SameLine();
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
    {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 36.0f);
        ImGui::TextUnformatted(Utf8(text).c_str());
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}
} // namespace

void Sm86::Initialize()
{
    Document document;
    std::error_code fileError;
    const auto folder = ComponentPath();
    // A regular Aurora installation has no SM86 component. Do not change its startup behavior.
    if (!std::filesystem::exists(folder / L"dlssg_sm86.ini", fileError))
        return;
    if (!ReadSettings(folder / L"dlssg_sm86.ini", document, startupError))
        return;
    startup = document.settings;
    if (!startup.enabled)
        return;

    for (auto name : { L"version.dll", L"winmm.dll", L"dbghelp.dll", L"dinput8.dll", L"dxgi.dll", L"d3d12.dll" })
    {
        if (IsProxy(GetModuleHandleW(name)))
        {
            ownsRuntime.store(true);
            startupError = "Another SM86 proxy is already loaded; remove the duplicate installation before using AIO";
            LOG_WARN("SM86 AIO: {}", startupError);
            return;
        }
    }

    if (!std::filesystem::exists(folder / L"version.dll", fileError))
    {
        startupError = "OptiScaler/SM86/version.dll is missing";
        return;
    }
    ownsRuntime.store(true); // Do not apply Aurora's Ada patches during nested third-party DLL loads.
    auto loaded = NtdllProxy::LoadLibraryExW_Ldr((folder / L"version.dll").c_str(), nullptr, 0);
    if (!IsProxy(loaded))
    {
        ownsRuntime.store(false);
        startupError = "Cannot load the bundled SM86 proxy or its identification exports";
        LOG_ERROR("SM86 AIO: {}", startupError);
        return;
    }
    module = loaded; // Process lifetime: never hot-unload a proxy with installed hooks.
    LOG_INFO("SM86 AIO proxy loaded. Optimized={}, MaxGeneratedFrames={}; restart required for changes",
             startup.optimized, startup.maxGeneratedFrames);
}

bool Sm86::IsLoaded() { return module != nullptr; }
bool Sm86::OwnsRuntime() { return ownsRuntime.load(); }

void Sm86::RenderMenu()
{
    if (auto section = ScopedCollapsingHeader(SM86_CN("RTX 20/30 DLSSG###sm86_settings"),
                                              ImGuiTreeNodeFlags_DefaultOpen);
        section.IsHeaderOpen())
    {
        ScopedIndent indent;
        static Document document;
        static Settings saved;
        static bool attempted = false;
        static bool ready = false;
        static std::string error;
        const auto path = ComponentPath() / L"dlssg_sm86.ini";
        auto reload = [&]() {
            ready = ReadSettings(path, document, error);
            if (ready)
                saved = document.settings;
            attempted = true;
        };
        if (!attempted)
            reload();
        if (!ready)
            ImGui::TextWrapped("%s", SM86_CN("无法读取组件配置。请安装完整 AIO 包，或检查 dlssg_sm86.ini。"));

        ImGui::BeginDisabled(!ready);
        auto& settings = document.settings;
        ImGui::Checkbox(SM86_CN("启用 RTX 20/30 DLSSG###sm86_enabled"), &settings.enabled);
        Help(L"DLSSG for SM86 0.3.5，适用于 NVIDIA RTX 20/30 的 D3D12 路径。实验性组合，尚未完成游戏内兼容性验证。\nRTX 40/50 请保留原有方案。组件独立加载，不占用游戏根目录的 version.dll。所有改动保存后重启生效。\n先在游戏中开启 DLSS 帧生成。使用 Aurora DLSSG 路径时选择“无（原生 DLSSG）”；不要同时启用其他帧生成替代方案。");
        ImGui::EndDisabled();
        ImGui::TextWrapped("%s", IsLoaded() ? SM86_CN("状态：组件已加载") : SM86_CN("状态：本次启动未加载组件"));
        Help(L"组件已加载不代表正在生成帧，请以游戏表现和组件日志为准。启用并保存后，需要完全退出并重启游戏。");
        if (!startupError.empty())
            ImGui::TextWrapped("%s", startupError.c_str());
        ImGui::Spacing();
        ImGui::BeginDisabled(!ready);
        ImGui::PushItemWidth(ImGui::GetFontSize() * 15.0f);
        const std::string tiers[] = { Utf8(L"0 · 原厂数值"), Utf8(L"1 · 一致性优先（推荐）"),
                                      Utf8(L"2 · 有损图像加速"), Utf8(L"3 · 更多有损加速") };
        const char* tierNames[] = { tiers[0].c_str(), tiers[1].c_str(), tiers[2].c_str(), tiers[3].c_str() };
        ImGui::Combo(SM86_CN("优化等级###sm86_optimized"), &settings.optimized, tierNames, 4);
        Help(L"等级 1 为组件默认档；等级 2/3 会牺牲画面一致性换取性能。实际效果取决于游戏和显卡。");
        int multiplier = settings.maxGeneratedFrames - 1;
        ImGui::Combo(SM86_CN("倍率上限###sm86_frame_limit"), &multiplier, "2X\0" "3X\0" "4X\0" "5X\0" "6X\0");
        settings.maxGeneratedFrames = multiplier + 1;
        Help(L"这是上限，不是强制实际倍率。默认 4X；5X/6X 还需要游戏和 Streamline 支持。不要强制旧版 4X 游戏超过其呈现队列上限。");
        int preset = settings.preset == "A" ? 1 : settings.preset == "B" ? 2 : 0;
        ImGui::Combo(SM86_CN("UI 重合成预设###sm86_preset"), &preset, "Auto\0A\0B\0");
        settings.preset = preset == 1 ? "A" : preset == 2 ? "B" : "Auto";
        Help(L"Auto 跟随游戏；A 关闭 UI 重合成；B 请求开启，但需要游戏提供 HUDless 图像与 UI 平面。");
        const std::string logs[] = { Utf8(L"关闭"), Utf8(L"仅错误"), Utf8(L"配置与能力"), Utf8(L"详细诊断") };
        const char* logNames[] = { logs[0].c_str(), logs[1].c_str(), logs[2].c_str(), logs[3].c_str() };
        ImGui::Combo(SM86_CN("日志级别###sm86_log_level"), &settings.logLevel, logNames, 4);
        ImGui::PopItemWidth();
        if (ImGui::Button(SM86_CN("保存组件设置###sm86_save")) && SaveSettings(path, document, error))
            saved = document.settings;
        ImGui::EndDisabled();
        ImGui::SameLine();
        if (ImGui::Button(SM86_CN("重新读取###sm86_reload")))
            reload();
        if (ready && !(document.settings == saved))
            ImGui::TextUnformatted(SM86_CN("有未保存的组件设置。"));
        if (ready && !(saved == startup))
            ImGui::TextWrapped("%s", SM86_CN("已保存的设置将在完全退出并重启游戏后生效。"));
        if (!error.empty())
            ImGui::TextWrapped("%s", error.c_str());
        ImGui::TextDisabled("%s", SM86_CN("使用“保存组件设置”，重启游戏后生效。"));
        Help(L"组件配置单独保存到 OptiScaler/SM86/dlssg_sm86.ini；底部“保存设置”仅保存 Aurora 配置。");
    }
}

#undef SM86_CN
