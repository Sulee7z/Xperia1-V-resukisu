# perfmgr-kernel — FPS-driven Energy-Aware Frequency Governor (FEAS)

Touch-boosted, FPS-driven frequency governor for **Sony Xperia 1 V (XQ-DQ72, SM8550)**
on GKI 5.15 (Android 13). Ported from Samsung GPIS (`sm8150-samsung perf_mgr`)
and adapted to the GKI common kernel.

## What it does

| Feature | Behavior |
|---|---|
| **Touch boost (iOS-style 3-tier)** | Finger down → CPU full floor + GPU min `touch_gpu_mhz` (550 MHz). Continuous touch 500 ms → CPU drops to 60% floor (iOS f3). Finger up → release after `touch_hold_ms` (2000 ms). |
| **Scroll keep-alive** | Every touch-move event resets the boost/release timers, so a continuous scroll never drops back to idle frequency (fixes 60 Hz micro-jank). |
| **Frame-driven util** | Apps report rendered frame durations via `/dev/perf_manager`; kernel computes required CPU capacity to hit target FPS. |
| **No hard locking** | Frequency is only *floored while interacting*; the system governor (walt / msm-adreno-tz) takes over when idle. |

## Kernel interface

```
/sys/kernel/perf_manager/
├── enable            (rw)  master switch
├── fps               (rw)  target FPS (default 60)
├── margin            (rw)  FPS margin percent (default 30)
├── touch_gpu_mhz     (rw)  GPU min freq during touch (default 550)
├── touch_mid_delay_ms(rw)  time before dropping to mid level (default 500)
├── touch_hold_ms     (rw)  boost hold after finger lift (default 2000)
├── diag              ( r)  diagnostics: touch/connect/boost state
└── frame             ( w)  report frame duration in ns (echo <ns> > frame)

/dev/perf_manager           misc device, mode 0666
  ioctl: PERF_MGR_TASK_ADD / FRAME_END / SET_FPS / PROCESS_KILL
```

## Diagnostics

```sh
cat /sys/kernel/perf_manager/diag
# touch_events=8207        ← input events received
# touch_presses=4928       ← finger-down events
# input_connect_ok=1       ← touchscreen connected
# handler_registered=1     ← input handler registered
# gpu_devfreq_found=1      ← GPU devfreq located
# touch_boosted=0/1        ← boost active
# touch_in_mid=0/1         ← dropped to mid level (continuous touch)
```

## iOS-style 3-tier boost

```
finger down  ──► CPU 100% floor + GPU 550 MHz   (iOS f2 - max)
  │
  ├─ 500 ms continuous touch ──► CPU 60% floor   (iOS f3 - mid)
  │
finger up ──► 2000 ms hold ──► release all       (iOS f1 - idle)
```

Every touch-move event keeps the boost alive (scroll never stutters).

## Userspace protocol

```c
int fd = open("/dev/perf_manager", O_RDWR);
struct fps_info f = { .tid = gettid(), .group_id = 0 };
ioctl(fd, PERF_MGR_TASK_ADD, &f);          // register render thread
ioctl(fd, PERF_MGR_FRAME_END, &f);         // per-frame report
```

## Build

```
# GKI 5.15 kernel tree, CONFIG_PERF_MGR=y
make ARCH=arm64 dist  # or the vendor build script
```

Output: `out/gki_kernel/dist/Image`

## Flash

```
fastboot flash boot boot_XQ-DQ72_feas-v15.img
```

Boot images are repacked from the stock Sony boot (header preserved, kernel replaced).

## Compatibility

- **KonaBess Next (GPU undervolt)**: compatible — FEAS sets runtime devfreq
  constraints, KonaBess modifies the static voltage/frequency table. Orthogonal;
  combined = smoother + cooler.
- **Vector Xposed framework**: optional frame-reporting module (`com.sony.feas`)
  hooks `Choreographer.onVsync` to report frame intervals.

## Release notes

- **v10**: fixed touch boost chain — `input_open_device`, EV_ABS+ABS_MT matching,
  per-policy `FREQ_QOS_MIN` floor, `ERR_PTR` check, atomic-context safety.
- **v11**: fixed GPU devfreq DT path (`/soc/qcom,kgsl-3d0@3d00000`).
- **v12**: iOS-style 3-tier boost (full → mid → release) with generation-based
  race protection; frame reporting no longer overrides touch boost.
- **v13**: scroll keep-alive — move events reset boost/release timers.
- **v15**: kernel-side frame stats + write-path fix: scroll keep-alive — move events reset boost/release timers.

## Tunables (examples)

```sh
# Raise GPU floor during touch to 680 MHz (max)
echo 680 > /sys/kernel/perf_manager/touch_gpu_mhz

# Drop to mid level after 1 s of continuous touch
echo 1000 > /sys/kernel/perf_manager/touch_mid_delay_ms

# Hold boost 3 s after lifting finger
echo 3000 > /sys/kernel/perf_manager/touch_hold_ms

# Disable the whole driver
echo 0 > /sys/kernel/perf_manager/enable
```

## Files

- `perf_mgr.c` — driver (GKI 5.15)
- `perf_mgr.h` — ioctl protocol header
- `perf_mgr_reference.c` — original Samsung reference for comparison
- `Kconfig` — `CONFIG_PERF_MGR` entry

## License

GPL-2.0
