# AIO RHI runtime refresh 1

运行库取自 [RankFTW/rhi-repo Releases](https://github.com/RankFTW/rhi-repo/releases)。本包复用 Aurora AIO Preview 1 的已编译程序，更新随包运行库，不修改程序功能。版本组合尚未完成游戏实测。

## 默认安装内容

| 文件 | 版本 | Release |
| --- | --- | --- |
| nvngx_dlss.dll | SR 310.9.1.0 | dlss-310.9.1 |
| nvngx_dlssd.dll | RR 310.9.1.0 | dlssd-310.9.1 |
| nvngx_dlssg.dll | FG 310.9.1.0 | dlssg-310.9.1 |
| nvngx_dlssnr.dll | NR 310.8.0.0 原版 | dlssnr-310.8.0 |

这些默认 DLL 的 NVIDIA Authenticode 签名在打包时验证有效。下载 ZIP 的 SHA-256 与 GitHub Release 提供的摘要一致；来源链接、压缩包及 DLL 哈希见 `RHI_RUNTIMES.json`。

原版 NR 与上一份 Aurora 包中的 NR 文件哈希不同；本次明确采用本仓库标为 `dlssnr-310.8.0`、签名有效的原版文件。根目录的 `nvngx.dll_dlssnr.dll` 是 Aurora 自己的转发组件，继续使用当前编译版本，不能拿 NR 运行库覆盖或改名替换它。

## 可选 NR 版本

`Optional/Runtimes` 内提供以下 ZIP，默认不解压、不加载：

- `nvngx_dlssnr_310.8.0-RTX40.zip`：第三方修改版。Windows 验签结果为 `HashMismatch`，即现有签名与修改后的文件内容不匹配。
- `nvngx_dlssnr_310.8.SF-v2.zip`：第三方修改版，Windows 验签结果为 `NotSigned`。文件内版本字符串为 `310.8.SF.0`，SF-v2 来自 Release 标签。
- `nvngx_dlssnr_310.8.0.zip`：用于恢复默认原版。

要试用可选 NR，先完全退出游戏，备份当前 `OptiScaler/nvngx_dlssnr.dll`，从选定 ZIP 解出同名 DLL 并替换这一文件，然后重启游戏。恢复时从原版 ZIP 解出文件到同一位置。备份请放到游戏目录以外，避免递归搜索选中旧 DLL。

不要把多个解压后的同名 NR DLL 留在游戏目录树中，也不要替换根目录 `nvngx.dll_dlssnr.dll`。可选版本的兼容性和效果未在本整合包中实测，Release 的名称不代表所有对应显卡/游戏均受支持。

## Streamline 与 SM86

`streamline_2.14.1.0.zip` 也作为可选原始附件保留。它没有包含本包现有的 `sl.dlss_nr.dll`，因此本次不自动替换现有 Streamline 组合，也不把旧 NR 插件与新核心混搭后宣称已验证。需要升级时应另行确认 NR 插件与目标游戏兼容性。

RTX 20/30 的 SM86 0.3.5 保持完整配套组件。SM86 启用后会使用其内置运行库，不能通过替换以上外部 `nvngx_dlssg.dll` 来升级其内部后端。

原有 Runtime Sync 与 Streamline 1.x 保护逻辑保持不变。本包不会直接修改已安装游戏。

## 重现打包

先运行 `scripts/fetch_rhi_runtimes.ps1` 下载并校验锁定附件，再向 `scripts/package_aio.ps1` 传入 `-RhiCache <下载目录>` 和一个新的 `-Version`。仍需提供原 Aurora 完整包及 SM86 组件目录。没有 `-RhiCache` 时保留原有打包行为。
