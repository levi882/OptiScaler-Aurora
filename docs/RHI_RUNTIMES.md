# AIO RHI runtime refresh 2 — ShortFuse NR

运行库取自 [RankFTW/rhi-repo Releases](https://github.com/RankFTW/rhi-repo/releases)。本包复用 Aurora AIO Preview 1 的已编译程序，更新随包运行库，不修改程序功能。版本组合尚未完成游戏实测。

## 默认安装内容

| 文件 | 版本 | Release |
| --- | --- | --- |
| nvngx_dlss.dll | SR 310.9.1.0 | dlss-310.9.1 |
| nvngx_dlssd.dll | RR 310.9.1.0 | dlssd-310.9.1 |
| nvngx_dlssg.dll | FG 310.9.1.0 | dlssg-310.9.1 |
| nvngx_dlssnr.dll | ShortFuse NR 310.8.SF-v2 | dlssnr-310.8.SF-v2 |

SR/RR/FG DLL 的 NVIDIA Authenticode 签名在打包时验证有效。ShortFuse NR 是第三方修改版，Windows 验签结果为 `NotSigned`；文件内版本字符串为 `310.8.SF.0`，SF-v2 来自 Release 标签。下载 ZIP 的 SHA-256 与 GitHub Release 提供的摘要一致；来源链接、压缩包及 DLL 哈希见 `RHI_RUNTIMES.json`。

本包统一使用 ShortFuse SF-v2，不再附带原版 NR 或 RTX40 专用修改版。根目录的 `nvngx.dll_dlssnr.dll` 是 Aurora 自己的转发组件，继续使用当前编译版本，不能拿 NR 运行库覆盖或改名替换它。

## 开启 NR

完整安装后，在 Aurora 菜单右侧展开 **DLSS 神经渲染**，勾选 **启用神经渲染**。SF-v2 已安装到 `OptiScaler/nvngx_dlssnr.dll`，不需要另外解压或选择 NR 版本；NR 功能本身仍默认关闭。

RHI 作者说明 ShortFuse NR 支持 RTX 20/30/40/50，参见 [RHI 2.4.9 发布说明](https://github.com/RankFTW/RHI/releases/tag/RHI-2.4.9)。这是组件作者声明；本 AIO 在各代显卡上的 NR 画面、性能和与 MFG 同时开启的兼容性仍未完成游戏实测。

从旧 AIO 升级时先完全退出游戏，将原安装备份到游戏目录外，再按安装说明覆盖对应文件；新包会替换 `OptiScaler/nvngx_dlssnr.dll`。旧包留下的 `Optional/Runtimes` NR 压缩包可自行移走，新包不会自动删除用户文件。

不要把多个解压后的同名 NR DLL 留在游戏目录树中，也不要替换根目录 `nvngx.dll_dlssnr.dll`。左侧 RTX 20/30 DLSSG 开关控制帧生成，与 NR 开关独立。

## Streamline 与 SM86

`streamline_2.14.1.0.zip` 也作为可选原始附件保留。它没有包含本包现有的 `sl.dlss_nr.dll`，因此本次不自动替换现有 Streamline 组合，也不把旧 NR 插件与新核心混搭后宣称已验证。需要升级时应另行确认 NR 插件与目标游戏兼容性。

RTX 20/30 的 SM86 0.3.5 保持完整配套组件。SM86 启用后会使用其内置运行库，不能通过替换以上外部 `nvngx_dlssg.dll` 来升级其内部后端。

原有 Runtime Sync 与 Streamline 1.x 保护逻辑保持不变。本包不会直接修改已安装游戏。

## 重现打包

先运行 `scripts/fetch_rhi_runtimes.ps1` 下载并校验锁定附件，再运行 `scripts/package_aio.ps1`。RHI 缓存默认取 `build_out/rhi`，可用 `-RhiCache <下载目录>` 指定；仍需提供原 Aurora 完整包及 SM86 组件目录。打包会检查 NR 清单只有默认 SF-v2，缺少缓存时会报错，避免退回原版。生成另一个包时请指定新的 `-Version`。
