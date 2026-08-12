# Xperia 1 V (XQ-DQ72) — FEAS & Resolution Unlock

iOS-style touch-boosted, FPS-driven energy-aware scheduling (**FEAS**) + screen
resolution unlock + dynamic refresh rate (dfps) + 240Hz MBR (BFI) for the
Sony Xperia 1 V (SM8550, GKI 5.15).

> Everything (module zip, APK, daemon jar, overlay, **dtbo image**) is built by
> GitHub Actions — see [workflows](.github/workflows).

## What this project provides

| Component | Path | What it does |
|---|---|---|
| FEAS Xposed module (APK) | `userspace/app` | systemui vsync frame reporting → kernel sysfs; system_server `supportedModes` sorting; Settings resolution options; **boot resolution restore** (4K persists across reboots) |
| Root daemon (`feasd.jar`) | `userspace/feasd-java` | touch → 120Hz / idle 4s → 60Hz (dfps), GPU compensation, animation boost, 240Hz MBR (BFI) via `FramerateController` HAL |
| RRO overlay | `userspace/overlay` | `config_maxUiWidth=0` (removes Sony's 1096 UI width limit) |
| Kernel driver | `kernel/` | FPS-driven frequency governor (`/sys/kernel/perf_manager/*`, `/dev/perf_manager`), built into GKI 5.15 |
| DTBO patch | `dtbo/` | adds Qualcomm DFPS properties to the panel overlay (dynamic 144/120/90/60 fps) |

## Building

All artifacts are produced by GitHub Actions:

| Workflow | Artifacts |
|---|---|
| [build-module.yml](.github/workflows/build-module.yml) | `FEAS-allinone.zip` (app-debug.apk + feasd.jar + overlay + service.sh + module.prop + sepolicy) |
| [build-dtbo.yml](.github/workflows/build-dtbo.yml) | `dtbo_patched.img` (25 MB partition image, DTBO v0) |
| [build-kernel.yml](.github/workflows/build-kernel.yml) | `Image` / `vmlinux` with `CONFIG_PERF_MGR=y` (GKI android13-5.15 + driver) |

Trigger: push to affected paths, `workflow_dispatch`, or tag → Release upload.

### Local build

```bash
# userspace (module zip)
cd userspace
ANDROID_HOME=/path/to/sdk ./gradlew :app:assembleDebug :overlay:assembleRelease
# then build feasd.jar (javac + d8) and assemble module/ as the workflow does

# dtbo
apt install device-tree-compiler
cd dtbo && rm -f overlay_07.dtb && \
  dtc -I dts -O dtb -o overlay_07.dtb overlay_07_patched.dts && \
  python3 build_dtbo.py . dtbo_patched.img
```

## Installation (device)

1. Flash `dtbo_patched.img`: `fastboot flash dtbo dtbo_patched.img`
2. Install `FEAS-allinone.zip` via KernelSU (or Magisk) manager
3. Reboot — resolution restores to your last chosen mode (e.g. 1644×3840),
   dfps/BFI are controlled from the FEAS app

Requirements: unlocked bootloader, KernelSU (Zygisk + Vector framework), root.

## How boot resolution restore works (the hard part)

Sony's display stack on Android 15 (`67.2.A.3.163`):

- `PersistentDataStore` refuses internal displays (`hasStableUniqueId()==false`)
  so the official boot-restore channel is a dead end.
- `cmd display set-user-preferred-display-mode` is applied too late (display
  config already finished) and the `Display.Mode` refresh-rate exact-match
  (`120.0f` vs `120.00001f`) silently drops it.
- The FEAS module therefore runs **inside system_server** at boot: waits for
  `sys.boot_completed`, picks the **real `Display.Mode` object** from
  `supportedModes` (refresh-rate tolerant), then calls
  `DisplayManagerGlobal.requestDisplayModes()` + `setUserPreferredDisplayMode()`
  and writes `Settings.System.user_selected_resolution` — the exact same path
  the Settings page uses. Verified working: 4K persists across reboots.

## License

MIT (kernel driver GPL-2.0 where applicable).
