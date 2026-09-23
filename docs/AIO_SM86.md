# Aurora AIO Preview 1

此预览版整合 Aurora 的现有功能与 [DLSSG for SM86 0.3.5](https://github.com/sdli1995/dlssg_for_sm86)，提供一个安装包和统一设置菜单。适配目标是 NVIDIA RTX 20/30 的 Windows x64、D3D12 游戏；RTX 40/50 保留 Aurora 原有路径。

**整合层已构建并做配置、DLL 加载和重定向测试，尚未完成 RTX 20/30 游戏实测。组件已加载不代表帧生成已正常运行。**

## 安装与设置

1. 退出游戏，将完整 AIO 包解压到真实渲染 EXE 所在目录。
2. 按原方式运行 `setup_windows.bat`，选择适合游戏的 Aurora 代理名称，例如 `dxgi.dll` 或 `winmm.dll`。
3. 保留整个 `OptiScaler/SM86` 子目录。**不用把 SM86 的 `version.dll` 复制到游戏根目录，也不要把它改名为 Aurora 的代理 DLL。**
4. 打开 Aurora 菜单，在帧生成区域展开 **RTX 20/30 MFG**，启用组件，点击 **保存组件设置**，然后完全退出并重启游戏。
5. 在游戏设置中开启 DLSS 帧生成。若使用 Aurora 的 DLSSG 输出，NVNGX 替换选择 **无（原生 DLSSG）**，不要同时启用其他 FG 替代方案。

SM86 默认关闭，避免改变 RTX 40/50、AMD、Intel 和已有安装的行为。首次开启需要重启。组件配置保存在 `OptiScaler/SM86/dlssg_sm86.ini`，Aurora 底部的“保存设置”不负责保存这份独立配置。

RTX 20/30 MFG 与 RTX 40 MFG 使用相同的折叠面板样式，状态显示在开关下方，详细说明收在 `(?)` 悬停提示中。RTX 40 仍使用底部“保存设置”；两组设置的保存与生效方式保持独立。即使 FG Input 或 FG Output 选择 DLSSG，RTX 40 区块也会显示补丁状态；补丁已应用不等于 RTX 40 MFG 正在输出，实际帧生成方式取决于 FG Output。

两个面板首次显示时默认展开。“帧生成”区域的 FG Input、FG Output 和倍率设置下方直接显示共用的“强制动态 MFG”“DMFG 目标帧数”和应用/重置按钮，随后才是 RTX 20/30 与 RTX 40 面板。未检测到 Streamline DLSSG 运行环境报告支持动态 MFG 时，控件仍显示，但不可操作。RTX 40 固定倍率解锁补丁不会开启动态 MFG。RTX 20/30 的实际倍率仍受 SM86 `MaxGeneratedFrames` 与游戏插件上限约束；FPS Target 只在动态模式下生效。

已独立安装 SM86 的用户，请先退出游戏并移除/还原原来的 SM86 代理，再使用 AIO。不要删除不属于 SM86 的同名游戏 DLL。AIO 检测到已加载的其他 SM86 代理时会提示重复安装。

## 菜单选项

| 选项 | 含义 |
| --- | --- |
| 启用 RTX 20/30 DLSSG | 下次启动时加载组件；关闭后重启才停止加载 |
| 优化等级 | 0 原厂数值；1 一致性优先（组件默认）；2/3 有损加速 |
| 倍率上限 | 2X–6X；默认 4X，是上限而不是强制倍率，实际取决于游戏和 Streamline |
| UI 重合成预设 | Auto 跟随游戏；A 关闭；B 请求开启，需要游戏提供 HUDless 图像和 UI 平面 |
| 日志级别 | 关闭、仅错误、配置与能力、详细诊断 |

所有选项均在保存并重启后生效。不要强制仅有 4X 呈现队列的游戏使用 6X。具体限制以组件原作者文档为准。

默认日志位于 `OptiScaler/SM86/dlssg_sm86/logs`。排查时可设为“配置与能力”，检查 `loader_*.jsonl` 与 `backend_*.jsonl`。AIO 不把“DLL 已加载”当成“后端已生效”，也不伪造实时帧生成状态。

## 与现有 Aurora 的配合

- 配置关闭或缺少组件时，不加载第三方 DLL。
- 启用后，Aurora 在安装自身加载钩子前，从确定的子目录加载组件。
- Aurora 自带 Streamline 请求 DLSSG 时经过第三方的 `LoadLibrary` 路由，避免直接 `LdrLoadDll` 绕过 SM86 的配套运行库。
- SM86 管理运行库期间，跳过 Aurora 的 Ada/Blackwell 内核补丁和 Ada 上限推测，避免同一运行库重复打补丁。
- 只有组件已加载且当前是 NVIDIA D3D12 路径，才额外放开菜单里的原生 DLSSG 选项；未安装组件时的原有硬件限制保留。
- 保留上一版本的两个“应用更改”按钮 ID 修复；新增控件均有稳定的内部 ID。
- 不更改 Runtime Sync、Streamline 1.x 保护、游戏文件恢复逻辑或用户现有 Aurora 配置。

## 验证范围

配置测试覆盖未知键/注释保留、保存与重读、无效范围拒绝、外部修改检测、临时文件冲突与只读替换失败。保存使用同目录临时文件完成写入后替换，失败保留原 INI。

子进程测试覆盖 SM86 在子目录中禁用/启用时的加载与导出，及启用时把 DLSSG 请求重定向到配套缓存运行库。它不能代替真实 RTX 20/30 上的 DX12 游戏测试、切换分辨率/画质、多次 FG 重建及长时间稳定性测试。

实际 Aurora DLL 的启动测试覆盖缺少组件、组件关闭、组件开启三种模式，并验证 Aurora 与 SM86 的加载钩子串联后仍按预期选择运行库。可分别运行 `tests/run_sm86_tests.ps1` 和 `tests/run_aio_startup_smoke.ps1 -Sm86Source <组件目录>` 重现配置与启动测试。

## 打包与来源

`scripts/package_aio.ps1` 从上游 Aurora v1.0 完整包、当前本地编译的 Aurora DLL 和指定的 SM86 目录生成预览包，并记录 SHA-256。原 SM86 DLL 不做修改，不把它的第三方内核冒充为 Aurora 的源码成果。

本次整合锁定的组件：

- 作者：sdli1995；[仓库](https://github.com/sdli1995/dlssg_for_sm86)
- 版本：0.3.5 / 310.9
- 提交：`9621db573e07ed54f50c15bbb585ed9a7bdfac28`
- `version.dll` SHA-256：`c3934a09399f022504227c72df0bf8c0de55f9a08880dddde898c5262cefa838`
- RTX 20 / SM75 适配致谢：Coldwood1026，详见组件的 `THIRD_PARTY_NOTICES.txt`。

包内保留第三方文档与声明。SM86 项目描述其源码采用 GPLv3，但该分发仓库主要提供预编译 DLL；本次采用独立组件集成，不声称已取得或合并其完整构建源码。NVIDIA 运行库与内核的权利声明以各自文档为准。
