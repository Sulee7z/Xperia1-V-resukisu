# Xperia 1 V (XQ-DQ72) — FEAS & Resolution Unlock

iOS-style touch-boosted, FPS-driven energy-aware scheduling (**FEAS**) + screen
resolution unlock + dynamic refresh rate (dfps) + 240Hz MBR (BFI) for the
Sony Xperia 1 V (SM8550, GKI 5.15).

Everything (module zip, APK, daemon jar, overlay, dtbo image, **kernel boot
image**) is built from this repository by GitHub Actions.

## Repository layout

| Path | What it is |
|---|---|
| `kernel_platform/` | **Full Sony/GKI 5.15.170 kernel source** (GKI `common` + QC `msm-kernel` vendor tree + `KernelSU` + custom `perf_mgr` driver). Excludes `prebuilts/` (2.6 GB toolchain, installed by CI) and `out/` build trees. |
| `kernel_platform/config_launchboost_550_base.config` | **The exact kernel .config of the verified-on-device kernel** (extracted via IKCONFIG from the flashed boot image). CI rebuilds from this so the result matches what was tested: GPU hard-capped at 550 MHz, 680 MHz allowed only on confirmed frame-time jank. **Do not lose this file.** |
| `kernel_platform/boot_base.img` | Boot image template (verified flashable) — CI swaps in the freshly built kernel to produce the new boot image. |
| `kernel/` | Standalone `perf_mgr` driver sources (reference/porting aid). |
| `dtbo/` | DTBO sources: 23 stock overlays (`.dtb`) + `overlay_07_patched.dts` (DFPS properties) + `build_dtbo.py` packer. |
| `userspace/` | FEAS Xposed module (APK), root daemon (`feasd.jar`), RRO overlay, Magisk/KernelSU module scripts. |
| `.github/workflows/` | CI: `build-kernel.yml`, `build-dtbo.yml`, `build-module.yml`. |

## What this project provides

| Component | Path | What it does |
|---|---|---|
| FEAS Xposed module (APK) | `userspace/app` | systemui vsync frame reporting → kernel sysfs; system_server `supportedModes` sorting; Settings resolution options; boot resolution restore (4K persists across reboots) |
| Root daemon (`feasd.jar`) | `userspace/feasd-java` | cold-launch detection → kernel launch_boost; touch → 120Hz / idle 4s → 60Hz (dfps), GPU compensation, animation boost, 240Hz MBR (BFI) via `FramerateController` HAL |
| RRO overlay | `userspace/overlay` | `config_maxUiWidth=0` (removes Sony's 1096 UI width limit) |
| Kernel driver | `kernel_platform/common/drivers/perf_mgr/` | FPS-driven governor + cold-launch boost (`/sys/kernel/perf_manager/*`), **GPU cap policy: 550 MHz normal / 680 MHz only on jank** |
| DTBO patch | `dtbo/` | adds Qualcomm DFPS properties to the panel overlay (dynamic 144/120/90/60 fps) |

## CI builds (GitHub Actions)

| Workflow | Artifact | How to trigger |
|---|---|---|
| `build-kernel.yml` | `boot_launchboost_550.img` (+ `Image`, `Image.lz4`, effective `.config`) | push touching `kernel_platform/**` or `workflow_dispatch` |
| `build-module.yml` | `FEAS-allinone.zip` (app-debug.apk + feasd.jar + overlay + service.sh + module.prop + sepolicy) | push touching `userspace/**` or `workflow_dispatch` |
| `build-dtbo.yml` | `dtbo_patched.img` (DTBO v0, 23 overlays) | push touching `dtbo/**` or `workflow_dispatch` |

To build everything: open **Actions → workflow → Run workflow** (branch `master`).
Artifacts are downloadable from the run page; tagging a release (`git tag v1.x &&
git push --tags`) attaches them to a GitHub Release.

CI equivalence notes:

- The kernel build uses the **exact saved config** (`config_launchboost_550_base.config`)
  and the same boot-image packaging as the local flow, so the produced boot
  image is functionally identical to the one validated on device (build
  environment differences mean the bytes are not bit-identical).
- `FEAS-allinone.zip` assembles the same 5 module entries as the locally built
  module zip (`service.sh`, `customize.sh`, `module.prop`, `sepolicy.rule`,
  `feasd.jar`, `app-debug.apk`, overlay apk).
- `dtbo_patched.img` is built from the committed overlays with `overlay_07`
  recompiled from `overlay_07_patched.dts`.

## Local build

### Kernel (boot image)

Requires ~16 GB free disk and the Android clang toolchain (a local
`kernel_platform/prebuilts/` from a full checkout, or any clang-14+ with lld):

```bash
cd kernel_platform
export PATH=/path/to/clang-r450784e/bin:$PATH   # or llvm-14+
export LLVM=1
mkdir -p out/gki_kernel/common
cp config_launchboost_550_base.config out/gki_kernel/common/.config

make -C common O=$PWD/out/gki_kernel/common ARCH=arm64 LLVM=1 olddefconfig
make -C common O=$PWD/out/gki_kernel/common ARCH=arm64 LLVM=1 -j$(nproc) Image

# package boot image (swap kernel into the template)
python3 - <<'EOF'
import struct
old = open('boot_base.img','rb').read()
k   = open('out/gki_kernel/common/arch/arm64/boot/Image','rb').read()
h   = bytearray(old[:4096])
struct.pack_into('<I', h, 8, len(k))
open('boot_launchboost_550.img','wb').write(bytes(h) + k)
EOF
```

Result: `boot_launchboost_550.img` — flash to `boot_a` (`fastboot flash boot_a
boot_launchboost_550.img`, or `dd` the file to `/dev/block/bootdevice/by-name/boot_a`
as root). Backup the current partition first.

### Userspace (module zip)

```bash
cd userspace
ANDROID_HOME=/path/to/sdk ./gradlew :app:assembleDebug :overlay:assembleRelease
# then build feasd.jar (javac + d8, min-api 35) and assemble module/ as
# .github/workflows/build-module.yml does; the module zip is
# userspace/dist/FEAS-allinone.zip
```

### DTBO

```bash
apt install device-tree-compiler
cd dtbo && rm -f overlay_07.dtb && \
  dtc -I dts -O dtb -o overlay_07.dtb overlay_07_patched.dts && \
  python3 build_dtbo.py . dtbo_patched.img
```

## Installation (device)

1. Flash `dtbo_patched.img`: `fastboot flash dtbo dtbo_patched.img`
2. Flash `boot_launchboost_550.img`: `fastboot flash boot_a boot_launchboost_550.img`
   (unlocked bootloader required; `verifiedbootstate=orange` is fine)
3. Install `FEAS-allinone.zip` via KernelSU (or Magisk) manager
4. Reboot — resolution restores to your last chosen mode (e.g. 1644×3840),
   dfps/BFI are controlled from the FEAS app

Requirements: unlocked bootloader, KernelSU (Zygisk + Vector framework), root.

## Tuning knobs (kernel, runtime)

| Sysfs node | Meaning |
|---|---|
| `/sys/kernel/perf_manager/launch_boost` | write 1 → 2.5 s cold-launch boost window |
| `/sys/kernel/perf_manager/touch_gpu_mhz` | GPU cap during touch/anim (default 550) |
| `/sys/kernel/perf_manager/jank` | 1 = 3 consecutive frames over budget (only then is 680 MHz GPU allowed) |
| `/sys/kernel/perf_manager/fps` | reported frame rate (drives CPU/GPU floor) |

GPU policy (built-in): **550 MHz hard cap** for idle/animation/touch/launch
boost; **680 MHz** only while `jank` is asserted (frame time over budget).

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
