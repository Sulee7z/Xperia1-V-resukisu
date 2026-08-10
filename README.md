# Xperia 1 V (XQ-DQ72) ReSukiSU 定制内核

基于索尼官方开源代码 `yodo-1.2.0-rel`(固件 67.2.A.3.141,GKI 5.15.170)构建的定制内核,集成 **ReSukiSU** 内核级 Root 与多项省电/性能优化。

## 特性

### Root 方案
- **ReSukiSU v4.1.0** — 基于 KernelSU 的 root 解决方案
  - Tracepoint Syscall Redirect hook(GKI 2.0 标准方式)
  - KSU Toolkit 支持(含 uname 伪装)
  - Multi-Manager 支持(兼容 KernelSU / RKSU / MKSU / SukiSU 管理器)

### 省电与性能优化
移植自 [ryncsn/android-common-kernel](https://github.com/ryncsn/android-common-kernel) `android13-5.15-silsrc-yodo-0.2`:

| 特性 | 说明 |
|---|---|
| **MGLRU** (`CONFIG_LRU_GEN_ENABLED`) | Multi-Gen LRU 内存回收,内存压力下更流畅 |
| **BBR** (`CONFIG_TCP_CONG_BBR`) | 默认 BBR 拥塞控制,网络更快更稳 |
| `CONFIG_SUSPEND_SKIP_SYNC` | 挂起时跳过 sync,更快进入深度休眠 |
| `CONFIG_WQ_POWER_EFFICIENT_DEFAULT` | 工作队列省电默认开启 |
| `CONFIG_THERMAL_DEFAULT_GOV_STEP_WISE` | 温控STEP_WISE |

### 构建伪装
- 编译信息伪装:`build-user@build-host`
- 时间戳伪装:`Tue Sep 16 15:52:19 UTC 2025`

## 构建方法

### 环境
- WSL2 (Ubuntu 24.04)
- clang-17(或 Android NDK r29)
- 工具链:`/root/toolchain/bin`(软链 clang/lld/llvm-* 全套)

### 构建命令
```bash
cd kernel_platform
export PATH=$PWD/prebuilts/clang/host/linux-x86/clang-r450784e/bin:$PATH
export BUILD_CONFIG=msm-kernel/build.config.msm.kalama.le
export VARIANT=gki
build/build.sh -j24
```

产物: `out/gki_kernel/dist/Image`(boot 内核,48MB)

## 刷机

> ⚠️ 刷机前请确保 bootloader 已解锁

```
fastboot flash boot boot_XQ-DQ72_resukisu-clean.img
fastboot reboot
```

## 目录结构

```
kernel_platform/
├── common/          # GKI 5.15 主内核树
├── msm-kernel/      # 高通 MSM 平台树(kalama)
│   └── arch/arm64/configs/
│       └── kalama_le_gki.fragment   # 定制配置(省电 + ReSukiSU)
└── vendor/qcom/     # 高通 vendor 驱动源码
```

## 许可

- 内核代码: GPL-2.0(遵循 Linux 内核许可)
- ReSukiSU: GPL-3.0
- 索尼开源代码遵循索尼开源许可条款

## 致谢

- [Sony Developer World](https://developer.sony.com/develop/open-devices/) — 开源内核
- [ReSukiSU](https://github.com/ReSukiSU/ReSukiSU) — KSU 分支
- [ryncsn/android-common-kernel](https://github.com/ryncsn/android-common-kernel) — 省电配置参考
