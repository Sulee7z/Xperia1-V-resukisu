cmd_/root/kernel-e/kernel_platform/msm-kernel/vendor/qcom/opensource/display-drivers/msm/sde_hdcp_1x.o := clang -Wp,-MMD,/root/kernel-e/kernel_platform/msm-kernel/vendor/qcom/opensource/display-drivers/msm/.sde_hdcp_1x.o.d -nostdinc -isystem /usr/lib/llvm-17/lib/clang/17/include -I/root/kernel-e/kernel_platform/msm-kernel/arch/arm64/include -I./arch/arm64/include/generated -I/root/kernel-e/kernel_platform/msm-kernel/include -I./include -I/root/kernel-e/kernel_platform/msm-kernel/arch/arm64/include/uapi -I./arch/arm64/include/generated/uapi -I/root/kernel-e/kernel_platform/msm-kernel/include/uapi -I./include/generated/uapi -include /root/kernel-e/kernel_platform/msm-kernel/include/linux/compiler-version.h -include /root/kernel-e/kernel_platform/msm-kernel/include/linux/kconfig.h -I/root/kernel-e/kernel_platform/msm-kernel/vendor/qcom/opensource/display-drivers/include/uapi/display -I/root/kernel-e/kernel_platform/msm-kernel/vendor/qcom/opensource/display-drivers/include -include /root/kernel-e/kernel_platform/msm-kernel/include/linux/compiler_types.h -D__KERNEL__ -mlittle-endian -DKASAN_SHADOW_SCALE_SHIFT= -Qunused-arguments -fmacro-prefix-map=/root/kernel-e/kernel_platform/msm-kernel/= -DANI_LITTLE_BYTE_ENDIAN -DANI_LITTLE_BIT_ENDIAN -DDOT11F_LITTLE_ENDIAN_HOST -DANI_COMPILER_TYPE_GCC -DANI_OS_TYPE_ANDROID=6 -DPTT_SOCK_SVC_ENABLE -Wall -Werror -D__linux__ -DMULTI_IF_NAME=\"msm_drm\" -DBUILD_TIMESTAMP=\"2026-08-12T03:10:07Z\" -Wall -Wundef -Werror=strict-prototypes -Wno-trigraphs -fno-strict-aliasing -fno-common -fshort-wchar -fno-PIE -Werror=implicit-function-declaration -Werror=implicit-int -Werror=return-type -Wno-format-security -std=gnu89 --target=aarch64-linux-gnu -fintegrated-as -Werror=unknown-warning-option -Werror=ignored-optimization-argument -mgeneral-regs-only -DCONFIG_CC_HAS_K_CONSTRAINT=1 -Wno-psabi -fno-asynchronous-unwind-tables -fno-unwind-tables -mbranch-protection=pac-ret+leaf+bti -Wa,-march=armv8.5-a -DARM64_ASM_ARCH='"armv8.5-a"' -ffixed-x18 -DKASAN_SHADOW_SCALE_SHIFT= -fno-delete-null-pointer-checks -Wno-frame-address -Wno-address-of-packed-member -O2 -Wframe-larger-than=2048 -fstack-protector-strong -Werror -Wno-gnu -mno-global-merge -Wno-unused-but-set-variable -Wno-unused-const-variable -fno-omit-frame-pointer -fno-optimize-sibling-calls -fno-stack-clash-protection -g -gdwarf-4 -fsanitize=shadow-call-stack -fno-lto -flto=thin -fsplit-lto-unit -fvisibility=default -fsanitize=cfi -fsanitize-cfi-cross-dso -fno-sanitize-cfi-canonical-jump-tables -fno-sanitize-trap=cfi -fno-sanitize-blacklist -Wdeclaration-after-statement -Wvla -Wno-pointer-sign -Wno-array-bounds -fno-strict-overflow -fno-stack-check -Werror=date-time -Werror=incompatible-pointer-types -Wno-initializer-overrides -Wno-format -Wno-sign-compare -Wno-format-zero-length -Wno-pointer-to-enum-cast -Wno-tautological-constant-out-of-range-compare -Wno-unaligned-access -mstack-protector-guard=sysreg -mstack-protector-guard-reg=sp_el0 -mstack-protector-guard-offset=1504 -include /root/kernel-e/kernel_platform/msm-kernel/vendor/qcom/opensource/display-drivers/config/gki_kalamadispconf.h -Iinclude/linux -Iinclude/linux/drm -I/root/kernel-e/kernel_platform/msm-kernel/vendor/qcom/opensource/display-drivers -I/root/kernel-e/kernel_platform/msm-kernel/vendor/qcom/opensource/display-drivers/include -I/root/kernel-e/kernel_platform/drivers/clk/qcom -I/root/kernel-e/kernel_platform/msm-kernel/drivers/clk/qcom -I/root/kernel-e/kernel_platform/msm-kernel/vendor/qcom/opensource/display-drivers/include/linux -I/root/kernel-e/kernel_platform/msm-kernel/vendor/qcom/opensource/display-drivers/rotator -I/root/kernel-e/kernel_platform/msm-kernel/vendor/qcom/opensource/display-drivers/msm -I/root/kernel-e/kernel_platform/msm-kernel/vendor/qcom/opensource/display-drivers/msm/dp -I/root/kernel-e/kernel_platform/msm-kernel/vendor/qcom/opensource/display-drivers/msm/dsi -I/root/kernel-e/kernel_platform/msm-kernel/vendor/qcom/opensource/display-drivers/msm/sde -I/root/kernel-e/kernel_platform/msm-kernel/vendor/qcom/opensource/display-drivers/include/uapi/display -Wheader-guard  -fsanitize=array-bounds -fsanitize=local-bounds -fsanitize-undefined-trap-on-error  -DMODULE  -DKBUILD_BASENAME='"sde_hdcp_1x"' -DKBUILD_MODNAME='"msm_drm"' -D__KBUILD_MODNAME=kmod_msm_drm -c -o /root/kernel-e/kernel_platform/msm-kernel/vendor/qcom/opensource/display-drivers/msm/sde_hdcp_1x.o /root/kernel-e/kernel_platform/msm-kernel/vendor/qcom/opensource/display-drivers/msm/sde_hdcp_1x.c

source_/root/kernel-e/kernel_platform/msm-kernel/vendor/qcom/opensource/display-drivers/msm/sde_hdcp_1x.o := /root/kernel-e/kernel_platform/msm-kernel/vendor/qcom/opensource/display-drivers/msm/sde_hdcp_1x.c

deps_/root/kernel-e/kernel_platform/msm-kernel/vendor/qcom/opensource/display-drivers/msm/sde_hdcp_1x.o := \
    $(wildcard include/config/HDCP_QSEECOM) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/compiler-version.h \
    $(wildcard include/config/CC_VERSION_TEXT) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/kconfig.h \
    $(wildcard include/config/CPU_BIG_ENDIAN) \
    $(wildcard include/config/BOOGER) \
    $(wildcard include/config/FOO) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/compiler_types.h \
    $(wildcard include/config/HAVE_ARCH_COMPILER_H) \
    $(wildcard include/config/CC_HAS_ASM_INLINE) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/compiler_attributes.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/compiler-clang.h \
    $(wildcard include/config/ARCH_USE_BUILTIN_BSWAP) \
  /root/kernel-e/kernel_platform/msm-kernel/arch/arm64/include/asm/compiler.h \
    $(wildcard include/config/CFI_CLANG) \
  /root/kernel-e/kernel_platform/msm-kernel/vendor/qcom/opensource/display-drivers/config/gki_kalamadispconf.h \
    $(wildcard include/config/DRM_MSM) \
    $(wildcard include/config/DRM_MSM_SDE) \
    $(wildcard include/config/SYNC_FILE) \
    $(wildcard include/config/DRM_MSM_DSI) \
    $(wildcard include/config/DRM_MSM_DP) \
    $(wildcard include/config/DRM_MSM_DP_MST) \
    $(wildcard include/config/DSI_PARSER) \
    $(wildcard include/config/DRM_SDE_WB) \
    $(wildcard include/config/DRM_SDE_RSC) \
    $(wildcard include/config/DRM_MSM_REGISTER_LOGGING) \
    $(wildcard include/config/MSM_MMRM) \
    $(wildcard include/config/DRM_SDE_EVTLOG_DEBUG) \
    $(wildcard include/config/QCOM_MDSS_PLL) \
    $(wildcard include/config/GKI_DISPLAY) \
    $(wildcard include/config/MSM_EXT_DISPLAY) \
    $(wildcard include/config/DRM_SDE_VM) \
    $(wildcard include/config/QTI_HW_FENCE) \
    $(wildcard include/config/QCOM_SPEC_SYNC) \
    $(wildcard include/config/DRM_SDE_SPECIFIC_PANEL) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/io.h \
    $(wildcard include/config/MMU) \
    $(wildcard include/config/HAS_IOPORT_MAP) \
    $(wildcard include/config/PCI) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/types.h \
    $(wildcard include/config/HAVE_UID16) \
    $(wildcard include/config/UID16) \
    $(wildcard include/config/ARCH_DMA_ADDR_T_64BIT) \
    $(wildcard include/config/PHYS_ADDR_T_64BIT) \
    $(wildcard include/config/64BIT) \
    $(wildcard include/config/ARCH_32BIT_USTAT_F_TINODE) \
  /root/kernel-e/kernel_platform/msm-kernel/include/uapi/linux/types.h \
  arch/arm64/include/generated/uapi/asm/types.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/uapi/asm-generic/types.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/asm-generic/int-ll64.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/uapi/asm-generic/int-ll64.h \
  /root/kernel-e/kernel_platform/msm-kernel/arch/arm64/include/uapi/asm/bitsperlong.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/asm-generic/bitsperlong.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/uapi/asm-generic/bitsperlong.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/uapi/linux/posix_types.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/stddef.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/uapi/linux/stddef.h \
  /root/kernel-e/kernel_platform/msm-kernel/arch/arm64/include/uapi/asm/posix_types.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/uapi/asm-generic/posix_types.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/init.h \
    $(wildcard include/config/HAVE_ARCH_PREL32_RELOCATIONS) \
    $(wildcard include/config/STRICT_KERNEL_RWX) \
    $(wildcard include/config/STRICT_MODULE_RWX) \
    $(wildcard include/config/LTO_CLANG) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/compiler.h \
    $(wildcard include/config/TRACE_BRANCH_PROFILING) \
    $(wildcard include/config/PROFILE_ALL_BRANCHES) \
    $(wildcard include/config/STACK_VALIDATION) \
  /root/kernel-e/kernel_platform/msm-kernel/arch/arm64/include/asm/rwonce.h \
    $(wildcard include/config/LTO) \
    $(wildcard include/config/AS_HAS_LDAPR) \
  /root/kernel-e/kernel_platform/msm-kernel/arch/arm64/include/asm/alternative-macros.h \
  arch/arm64/include/generated/asm/cpucaps.h \
  /root/kernel-e/kernel_platform/msm-kernel/arch/arm64/include/asm/insn-def.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/stringify.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/asm-generic/rwonce.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/kasan-checks.h \
    $(wildcard include/config/KASAN_GENERIC) \
    $(wildcard include/config/KASAN_SW_TAGS) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/kcsan-checks.h \
    $(wildcard include/config/KCSAN) \
    $(wildcard include/config/KCSAN_IGNORE_ATOMICS) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/bug.h \
    $(wildcard include/config/GENERIC_BUG) \
    $(wildcard include/config/BUG_ON_DATA_CORRUPTION) \
  /root/kernel-e/kernel_platform/msm-kernel/arch/arm64/include/asm/bug.h \
  /root/kernel-e/kernel_platform/msm-kernel/arch/arm64/include/asm/asm-bug.h \
    $(wildcard include/config/DEBUG_BUGVERBOSE) \
  /root/kernel-e/kernel_platform/msm-kernel/arch/arm64/include/asm/brk-imm.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/asm-generic/bug.h \
    $(wildcard include/config/BUG) \
    $(wildcard include/config/GENERIC_BUG_RELATIVE_POINTERS) \
    $(wildcard include/config/SMP) \
    $(wildcard include/config/MODULES) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/instrumentation.h \
    $(wildcard include/config/DEBUG_ENTRY) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/once_lite.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/panic.h \
    $(wildcard include/config/PANIC_TIMEOUT) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/printk.h \
    $(wildcard include/config/MESSAGE_LOGLEVEL_DEFAULT) \
    $(wildcard include/config/CONSOLE_LOGLEVEL_DEFAULT) \
    $(wildcard include/config/CONSOLE_LOGLEVEL_QUIET) \
    $(wildcard include/config/EARLY_PRINTK) \
    $(wildcard include/config/PRINTK) \
    $(wildcard include/config/PRINTK_INDEX) \
    $(wildcard include/config/DYNAMIC_DEBUG) \
    $(wildcard include/config/DYNAMIC_DEBUG_CORE) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/stdarg.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/kern_levels.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/linkage.h \
    $(wildcard include/config/ARCH_USE_SYM_ANNOTATIONS) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/export.h \
    $(wildcard include/config/MODVERSIONS) \
    $(wildcard include/config/MODULE_REL_CRCS) \
    $(wildcard include/config/TRIM_UNUSED_KSYMS) \
  /root/kernel-e/kernel_platform/msm-kernel/arch/arm64/include/asm/linkage.h \
    $(wildcard include/config/ARM64_BTI_KERNEL) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/ratelimit_types.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/bits.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/const.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/vdso/const.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/uapi/linux/const.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/vdso/bits.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/build_bug.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/uapi/linux/param.h \
  /root/kernel-e/kernel_platform/msm-kernel/arch/arm64/include/uapi/asm/param.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/asm-generic/param.h \
    $(wildcard include/config/HZ) \
  /root/kernel-e/kernel_platform/msm-kernel/include/uapi/asm-generic/param.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/spinlock_types.h \
    $(wildcard include/config/PREEMPT_RT) \
    $(wildcard include/config/DEBUG_LOCK_ALLOC) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/spinlock_types_raw.h \
    $(wildcard include/config/DEBUG_SPINLOCK) \
  /root/kernel-e/kernel_platform/msm-kernel/arch/arm64/include/asm/spinlock_types.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/asm-generic/qspinlock_types.h \
    $(wildcard include/config/NR_CPUS) \
  /root/kernel-e/kernel_platform/msm-kernel/include/asm-generic/qrwlock_types.h \
  /root/kernel-e/kernel_platform/msm-kernel/arch/arm64/include/uapi/asm/byteorder.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/byteorder/little_endian.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/uapi/linux/byteorder/little_endian.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/swab.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/uapi/linux/swab.h \
  arch/arm64/include/generated/uapi/asm/swab.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/uapi/asm-generic/swab.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/byteorder/generic.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/lockdep_types.h \
    $(wildcard include/config/PROVE_RAW_LOCK_NESTING) \
    $(wildcard include/config/PREEMPT_LOCK) \
    $(wildcard include/config/LOCKDEP) \
    $(wildcard include/config/LOCK_STAT) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/rwlock_types.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/err.h \
  arch/arm64/include/generated/uapi/asm/errno.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/uapi/asm-generic/errno.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/uapi/asm-generic/errno-base.h \
  /root/kernel-e/kernel_platform/msm-kernel/arch/arm64/include/asm/io.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/pgtable.h \
    $(wildcard include/config/PGTABLE_LEVELS) \
    $(wildcard include/config/HIGHPTE) \
    $(wildcard include/config/TRANSPARENT_HUGEPAGE) \
    $(wildcard include/config/ARCH_HAS_NONLEAF_PMD_YOUNG) \
    $(wildcard include/config/GUP_GET_PTE_LOW_HIGH) \
    $(wildcard include/config/HAVE_ARCH_TRANSPARENT_HUGEPAGE_PUD) \
    $(wildcard include/config/HAVE_ARCH_SOFT_DIRTY) \
    $(wildcard include/config/ARCH_ENABLE_THP_MIGRATION) \
    $(wildcard include/config/ARCH_HAS_PTE_DEVMAP) \
    $(wildcard include/config/NUMA_BALANCING) \
    $(wildcard include/config/HAVE_ARCH_HUGE_VMAP) \
    $(wildcard include/config/X86_ESPFIX64) \
    $(wildcard include/config/HUGETLB_PAGE) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/pfn.h \
  /root/kernel-e/kernel_platform/msm-kernel/arch/arm64/include/asm/pgtable.h \
    $(wildcard include/config/ARM64_PA_BITS_52) \
    $(wildcard include/config/DEBUG_VM) \
    $(wildcard include/config/ARM64_64K_PAGES) \
    $(wildcard include/config/ARM64_MTE) \
  /root/kernel-e/kernel_platform/msm-kernel/arch/arm64/include/asm/proc-fns.h \
  /root/kernel-e/kernel_platform/msm-kernel/arch/arm64/include/asm/page.h \
  /root/kernel-e/kernel_platform/msm-kernel/arch/arm64/include/asm/page-def.h \
    $(wildcard include/config/ARM64_PAGE_SHIFT) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/personality.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/uapi/linux/personality.h \
  /root/kernel-e/kernel_platform/msm-kernel/arch/arm64/include/asm/pgtable-types.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/asm-generic/pgtable-nopud.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/asm-generic/pgtable-nop4d.h \
  /root/kernel-e/kernel_platform/msm-kernel/arch/arm64/include/asm/memory.h \
    $(wildcard include/config/ARM64_VA_BITS) \
    $(wildcard include/config/KASAN_SHADOW_OFFSET) \
    $(wildcard include/config/KASAN) \
    $(wildcard include/config/VMAP_STACK) \
    $(wildcard include/config/ARM64_4K_PAGES) \
    $(wildcard include/config/KASAN_HW_TAGS) \
    $(wildcard include/config/DEBUG_VIRTUAL) \
    $(wildcard include/config/EFI) \
    $(wildcard include/config/ARM_GIC_V3_ITS) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/sizes.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/bitops.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/typecheck.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/uapi/linux/kernel.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/uapi/linux/sysinfo.h \
  /root/kernel-e/kernel_platform/msm-kernel/arch/arm64/include/asm/bitops.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/asm-generic/bitops/builtin-__ffs.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/asm-generic/bitops/builtin-ffs.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/asm-generic/bitops/builtin-__fls.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/asm-generic/bitops/builtin-fls.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/asm-generic/bitops/ffz.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/asm-generic/bitops/fls64.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/asm-generic/bitops/find.h \
    $(wildcard include/config/GENERIC_FIND_FIRST_BIT) \
  /root/kernel-e/kernel_platform/msm-kernel/include/asm-generic/bitops/sched.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/asm-generic/bitops/hweight.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/asm-generic/bitops/arch_hweight.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/asm-generic/bitops/const_hweight.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/asm-generic/bitops/atomic.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/atomic.h \
  /root/kernel-e/kernel_platform/msm-kernel/arch/arm64/include/asm/atomic.h \
  /root/kernel-e/kernel_platform/msm-kernel/arch/arm64/include/asm/barrier.h \
    $(wildcard include/config/ARM64_PSEUDO_NMI) \
  /root/kernel-e/kernel_platform/msm-kernel/include/asm-generic/barrier.h \
  /root/kernel-e/kernel_platform/msm-kernel/arch/arm64/include/asm/cmpxchg.h \
  /root/kernel-e/kernel_platform/msm-kernel/arch/arm64/include/asm/lse.h \
    $(wildcard include/config/ARM64_LSE_ATOMICS) \
  /root/kernel-e/kernel_platform/msm-kernel/arch/arm64/include/asm/atomic_ll_sc.h \
    $(wildcard include/config/CC_HAS_K_CONSTRAINT) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/jump_label.h \
    $(wildcard include/config/JUMP_LABEL) \
    $(wildcard include/config/HAVE_ARCH_JUMP_LABEL_RELATIVE) \
  /root/kernel-e/kernel_platform/msm-kernel/arch/arm64/include/asm/jump_label.h \
  /root/kernel-e/kernel_platform/msm-kernel/arch/arm64/include/asm/insn.h \
  /root/kernel-e/kernel_platform/msm-kernel/arch/arm64/include/asm/alternative.h \
  /root/kernel-e/kernel_platform/msm-kernel/arch/arm64/include/asm/atomic_lse.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/atomic/atomic-arch-fallback.h \
    $(wildcard include/config/GENERIC_ATOMIC64) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/atomic/atomic-long.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/atomic/atomic-instrumented.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/instrumented.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/asm-generic/bitops/instrumented-atomic.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/asm-generic/bitops/lock.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/asm-generic/bitops/instrumented-lock.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/asm-generic/bitops/non-atomic.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/asm-generic/bitops/le.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/asm-generic/bitops/ext2-atomic-setbit.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/mmdebug.h \
    $(wildcard include/config/DEBUG_VM_PGFLAGS) \
  /root/kernel-e/kernel_platform/msm-kernel/include/asm-generic/memory_model.h \
    $(wildcard include/config/FLATMEM) \
    $(wildcard include/config/SPARSEMEM_VMEMMAP) \
    $(wildcard include/config/SPARSEMEM) \
  /root/kernel-e/kernel_platform/msm-kernel/include/asm-generic/getorder.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/log2.h \
    $(wildcard include/config/ARCH_HAS_ILOG2_U32) \
    $(wildcard include/config/ARCH_HAS_ILOG2_U64) \
  /root/kernel-e/kernel_platform/msm-kernel/arch/arm64/include/asm/mte.h \
  /root/kernel-e/kernel_platform/msm-kernel/arch/arm64/include/asm/mte-def.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/bitfield.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/page-flags.h \
    $(wildcard include/config/ARCH_USES_PG_UNCACHED) \
    $(wildcard include/config/MEMORY_FAILURE) \
    $(wildcard include/config/PAGE_IDLE_FLAG) \
    $(wildcard include/config/HIGHMEM) \
    $(wildcard include/config/SWAP) \
    $(wildcard include/config/THP_SWAP) \
    $(wildcard include/config/KSM) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/mm_types.h \
    $(wildcard include/config/HAVE_ALIGNED_STRUCT_PAGE) \
    $(wildcard include/config/MEMCG) \
    $(wildcard include/config/USERFAULTFD) \
    $(wildcard include/config/SPECULATIVE_PAGE_FAULT) \
    $(wildcard include/config/NUMA) \
    $(wildcard include/config/HAVE_ARCH_COMPAT_MMAP_BASES) \
    $(wildcard include/config/MEMBARRIER) \
    $(wildcard include/config/AIO) \
    $(wildcard include/config/MMU_NOTIFIER) \
    $(wildcard include/config/ARCH_WANT_BATCHED_UNMAP_TLB_FLUSH) \
    $(wildcard include/config/IOMMU_SUPPORT) \
    $(wildcard include/config/LRU_GEN) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/mm_types_task.h \
    $(wildcard include/config/SPLIT_PTLOCK_CPUS) \
    $(wildcard include/config/ARCH_ENABLE_SPLIT_PMD_PTLOCK) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/threads.h \
    $(wildcard include/config/BASE_SMALL) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/cpumask.h \
    $(wildcard include/config/CPUMASK_OFFSTACK) \
    $(wildcard include/config/HOTPLUG_CPU) \
    $(wildcard include/config/DEBUG_PER_CPU_MAPS) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/kernel.h \
    $(wildcard include/config/PREEMPT_VOLUNTARY) \
    $(wildcard include/config/PREEMPT_DYNAMIC) \
    $(wildcard include/config/PREEMPT_) \
    $(wildcard include/config/DEBUG_ATOMIC_SLEEP) \
    $(wildcard include/config/PROVE_LOCKING) \
    $(wildcard include/config/TRACING) \
    $(wildcard include/config/FTRACE_MCOUNT_RECORD) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/align.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/limits.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/uapi/linux/limits.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/vdso/limits.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/kstrtox.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/math.h \
  arch/arm64/include/generated/asm/div64.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/asm-generic/div64.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/minmax.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/static_call_types.h \
    $(wildcard include/config/HAVE_STATIC_CALL) \
    $(wildcard include/config/HAVE_STATIC_CALL_INLINE) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/bitmap.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/string.h \
    $(wildcard include/config/BINARY_PRINTF) \
    $(wildcard include/config/FORTIFY_SOURCE) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/errno.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/uapi/linux/errno.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/uapi/linux/string.h \
  /root/kernel-e/kernel_platform/msm-kernel/arch/arm64/include/asm/string.h \
    $(wildcard include/config/ARCH_HAS_UACCESS_FLUSHCACHE) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/sched.h \
    $(wildcard include/config/VIRT_CPU_ACCOUNTING_NATIVE) \
    $(wildcard include/config/SCHED_INFO) \
    $(wildcard include/config/SCHEDSTATS) \
    $(wildcard include/config/FAIR_GROUP_SCHED) \
    $(wildcard include/config/RT_GROUP_SCHED) \
    $(wildcard include/config/RT_MUTEXES) \
    $(wildcard include/config/UCLAMP_TASK) \
    $(wildcard include/config/UCLAMP_BUCKETS_COUNT) \
    $(wildcard include/config/KMAP_LOCAL) \
    $(wildcard include/config/THREAD_INFO_IN_TASK) \
    $(wildcard include/config/SCHED_CORE) \
    $(wildcard include/config/CGROUP_SCHED) \
    $(wildcard include/config/PREEMPT_NOTIFIERS) \
    $(wildcard include/config/BLK_DEV_IO_TRACE) \
    $(wildcard include/config/PREEMPT_RCU) \
    $(wildcard include/config/TASKS_RCU) \
    $(wildcard include/config/TASKS_TRACE_RCU) \
    $(wildcard include/config/PSI) \
    $(wildcard include/config/COMPAT_BRK) \
    $(wildcard include/config/CGROUPS) \
    $(wildcard include/config/BLK_CGROUP) \
    $(wildcard include/config/PAGE_OWNER) \
    $(wildcard include/config/EVENTFD) \
    $(wildcard include/config/STACKPROTECTOR) \
    $(wildcard include/config/ARCH_HAS_SCALED_CPUTIME) \
    $(wildcard include/config/CPU_FREQ_TIMES) \
    $(wildcard include/config/VIRT_CPU_ACCOUNTING_GEN) \
    $(wildcard include/config/NO_HZ_FULL) \
    $(wildcard include/config/POSIX_CPUTIMERS) \
    $(wildcard include/config/POSIX_CPU_TIMERS_TASK_WORK) \
    $(wildcard include/config/KEYS) \
    $(wildcard include/config/SYSVIPC) \
    $(wildcard include/config/DETECT_HUNG_TASK) \
    $(wildcard include/config/IO_URING) \
    $(wildcard include/config/AUDIT) \
    $(wildcard include/config/AUDITSYSCALL) \
    $(wildcard include/config/DEBUG_MUTEXES) \
    $(wildcard include/config/TRACE_IRQFLAGS) \
    $(wildcard include/config/UBSAN) \
    $(wildcard include/config/UBSAN_TRAP) \
    $(wildcard include/config/BLOCK) \
    $(wildcard include/config/COMPACTION) \
    $(wildcard include/config/TASK_XACCT) \
    $(wildcard include/config/CPUSETS) \
    $(wildcard include/config/X86_CPU_RESCTRL) \
    $(wildcard include/config/FUTEX) \
    $(wildcard include/config/COMPAT) \
    $(wildcard include/config/PERF_EVENTS) \
    $(wildcard include/config/DEBUG_PREEMPT) \
    $(wildcard include/config/RSEQ) \
    $(wildcard include/config/TASK_DELAY_ACCT) \
    $(wildcard include/config/FAULT_INJECTION) \
    $(wildcard include/config/LATENCYTOP) \
    $(wildcard include/config/FUNCTION_GRAPH_TRACER) \
    $(wildcard include/config/KCOV) \
    $(wildcard include/config/UPROBES) \
    $(wildcard include/config/BCACHE) \
    $(wildcard include/config/LIVEPATCH) \
    $(wildcard include/config/SECURITY) \
    $(wildcard include/config/BPF_SYSCALL) \
    $(wildcard include/config/GCC_PLUGIN_STACKLEAK) \
    $(wildcard include/config/X86_MCE) \
    $(wildcard include/config/KRETPROBES) \
    $(wildcard include/config/ARCH_HAS_PARANOID_L1D_FLUSH) \
    $(wildcard include/config/RT_SOFTINT_OPTIMIZATION) \
    $(wildcard include/config/ARCH_TASK_STRUCT_ON_STACK) \
    $(wildcard include/config/PREEMPTION) \
    $(wildcard include/config/DEBUG_RSEQ) \
  /root/kernel-e/kernel_platform/msm-kernel/include/uapi/linux/sched.h \
  /root/kernel-e/kernel_platform/msm-kernel/arch/arm64/include/asm/current.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/pid.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/rculist.h \
    $(wildcard include/config/PROVE_RCU_LIST) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/list.h \
    $(wildcard include/config/DEBUG_LIST) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/poison.h \
    $(wildcard include/config/ILLEGAL_POINTER_VALUE) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/rcupdate.h \
    $(wildcard include/config/TINY_RCU) \
    $(wildcard include/config/TASKS_RCU_GENERIC) \
    $(wildcard include/config/RCU_STALL_COMMON) \
    $(wildcard include/config/RCU_NOCB_CPU) \
    $(wildcard include/config/TASKS_RUDE_RCU) \
    $(wildcard include/config/TREE_RCU) \
    $(wildcard include/config/DEBUG_OBJECTS_RCU_HEAD) \
    $(wildcard include/config/PROVE_RCU) \
    $(wildcard include/config/ARCH_WEAK_RELEASE_ACQUIRE) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/irqflags.h \
    $(wildcard include/config/IRQSOFF_TRACER) \
    $(wildcard include/config/PREEMPT_TRACER) \
    $(wildcard include/config/DEBUG_IRQFLAGS) \
    $(wildcard include/config/TRACE_IRQFLAGS_SUPPORT) \
  /root/kernel-e/kernel_platform/msm-kernel/arch/arm64/include/asm/irqflags.h \
  /root/kernel-e/kernel_platform/msm-kernel/arch/arm64/include/asm/ptrace.h \
  /root/kernel-e/kernel_platform/msm-kernel/arch/arm64/include/asm/cpufeature.h \
    $(wildcard include/config/ARM64_PAN) \
    $(wildcard include/config/ARM64_SW_TTBR0_PAN) \
    $(wildcard include/config/ARM64_SVE) \
    $(wildcard include/config/ARM64_CNP) \
    $(wildcard include/config/ARM64_PTR_AUTH) \
    $(wildcard include/config/ARM64_DEBUG_PRIORITY_MASKING) \
    $(wildcard include/config/ARM64_BTI) \
    $(wildcard include/config/ARM64_TLB_RANGE) \
    $(wildcard include/config/ARM64_PA_BITS) \
    $(wildcard include/config/ARM64_HW_AFDBM) \
    $(wildcard include/config/ARM64_AMU_EXTN) \
  /root/kernel-e/kernel_platform/msm-kernel/arch/arm64/include/asm/cputype.h \
  /root/kernel-e/kernel_platform/msm-kernel/arch/arm64/include/asm/sysreg.h \
    $(wildcard include/config/BROKEN_GAS_INST) \
    $(wildcard include/config/ARM64_16K_PAGES) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/kasan-tags.h \
  /root/kernel-e/kernel_platform/msm-kernel/arch/arm64/include/asm/hwcap.h \
  /root/kernel-e/kernel_platform/msm-kernel/arch/arm64/include/uapi/asm/hwcap.h \
  /root/kernel-e/kernel_platform/msm-kernel/arch/arm64/include/uapi/asm/ptrace.h \
  /root/kernel-e/kernel_platform/msm-kernel/arch/arm64/include/uapi/asm/sve_context.h \
  /root/kernel-e/kernel_platform/msm-kernel/arch/arm64/include/asm/percpu.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/preempt.h \
    $(wildcard include/config/PREEMPT_COUNT) \
    $(wildcard include/config/TRACE_PREEMPT_TOGGLE) \
  /root/kernel-e/kernel_platform/msm-kernel/arch/arm64/include/asm/preempt.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/thread_info.h \
    $(wildcard include/config/GENERIC_ENTRY) \
    $(wildcard include/config/HAVE_ARCH_WITHIN_STACK_FRAMES) \
    $(wildcard include/config/HARDENED_USERCOPY) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/restart_block.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/time64.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/math64.h \
    $(wildcard include/config/ARCH_SUPPORTS_INT128) \
  /root/kernel-e/kernel_platform/msm-kernel/include/vdso/math64.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/vdso/time64.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/uapi/linux/time.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/uapi/linux/time_types.h \
  /root/kernel-e/kernel_platform/msm-kernel/arch/arm64/include/asm/thread_info.h \
    $(wildcard include/config/SHADOW_CALL_STACK) \
  /root/kernel-e/kernel_platform/msm-kernel/arch/arm64/include/asm/stack_pointer.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/asm-generic/percpu.h \
    $(wildcard include/config/HAVE_SETUP_PER_CPU_AREA) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/percpu-defs.h \
    $(wildcard include/config/DEBUG_FORCE_WEAK_PER_CPU) \
    $(wildcard include/config/AMD_MEM_ENCRYPT) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/bottom_half.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/lockdep.h \
    $(wildcard include/config/DEBUG_LOCKING_API_SELFTESTS) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/smp.h \
    $(wildcard include/config/UP_LATE_INIT) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/smp_types.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/llist.h \
    $(wildcard include/config/ARCH_HAVE_NMI_SAFE_CMPXCHG) \
  /root/kernel-e/kernel_platform/msm-kernel/arch/arm64/include/asm/smp.h \
    $(wildcard include/config/ARM64_ACPI_PARKING_PROTOCOL) \
  /root/kernel-e/kernel_platform/msm-kernel/arch/arm64/include/asm/processor.h \
    $(wildcard include/config/KUSER_HELPERS) \
    $(wildcard include/config/ARM64_FORCE_52BIT) \
    $(wildcard include/config/HAVE_HW_BREAKPOINT) \
    $(wildcard include/config/ARM64_PTR_AUTH_KERNEL) \
    $(wildcard include/config/ARM64_TAGGED_ADDR_ABI) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/cache.h \
    $(wildcard include/config/ARCH_HAS_CACHE_LINE_SIZE) \
  /root/kernel-e/kernel_platform/msm-kernel/arch/arm64/include/asm/cache.h \
    $(wildcard include/config/ARM64_KMALLOC64) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/kasan-enabled.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/static_key.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/android_vendor.h \
    $(wildcard include/config/ANDROID_VENDOR_OEM_DATA) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/android_kabi.h \
    $(wildcard include/config/ANDROID_KABI_RESERVE) \
  /root/kernel-e/kernel_platform/msm-kernel/include/vdso/processor.h \
  /root/kernel-e/kernel_platform/msm-kernel/arch/arm64/include/asm/vdso/processor.h \
  /root/kernel-e/kernel_platform/msm-kernel/arch/arm64/include/asm/hw_breakpoint.h \
  /root/kernel-e/kernel_platform/msm-kernel/arch/arm64/include/asm/virt.h \
    $(wildcard include/config/KVM) \
  /root/kernel-e/kernel_platform/msm-kernel/arch/arm64/include/asm/sections.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/asm-generic/sections.h \
  /root/kernel-e/kernel_platform/msm-kernel/arch/arm64/include/asm/kasan.h \
  /root/kernel-e/kernel_platform/msm-kernel/arch/arm64/include/asm/mte-kasan.h \
  /root/kernel-e/kernel_platform/msm-kernel/arch/arm64/include/asm/pgtable-hwdef.h \
    $(wildcard include/config/ARM64_CONT_PTE_SHIFT) \
    $(wildcard include/config/ARM64_CONT_PMD_SHIFT) \
    $(wildcard include/config/ARM64_VA_BITS_52) \
  /root/kernel-e/kernel_platform/msm-kernel/arch/arm64/include/asm/pointer_auth.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/uapi/linux/prctl.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/random.h \
    $(wildcard include/config/ARCH_RANDOM) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/once.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/uapi/linux/random.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/uapi/linux/ioctl.h \
  arch/arm64/include/generated/uapi/asm/ioctl.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/asm-generic/ioctl.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/uapi/asm-generic/ioctl.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/irqnr.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/uapi/linux/irqnr.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/prandom.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/percpu.h \
    $(wildcard include/config/NEED_PER_CPU_EMBED_FIRST_CHUNK) \
    $(wildcard include/config/NEED_PER_CPU_PAGE_FIRST_CHUNK) \
  /root/kernel-e/kernel_platform/msm-kernel/arch/arm64/include/asm/archrandom.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/arm-smccc.h \
    $(wildcard include/config/ARM64) \
    $(wildcard include/config/HAVE_ARM_SMCCC) \
    $(wildcard include/config/ARM) \
  /root/kernel-e/kernel_platform/msm-kernel/arch/arm64/include/asm/spectre.h \
  /root/kernel-e/kernel_platform/msm-kernel/arch/arm64/include/asm/fpsimd.h \
  /root/kernel-e/kernel_platform/msm-kernel/arch/arm64/include/uapi/asm/sigcontext.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/rcutree.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/wait.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/spinlock.h \
  arch/arm64/include/generated/asm/mmiowb.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/asm-generic/mmiowb.h \
    $(wildcard include/config/MMIOWB) \
  /root/kernel-e/kernel_platform/msm-kernel/arch/arm64/include/asm/spinlock.h \
  arch/arm64/include/generated/asm/qspinlock.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/asm-generic/qspinlock.h \
  arch/arm64/include/generated/asm/qrwlock.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/asm-generic/qrwlock.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/rwlock.h \
    $(wildcard include/config/PREEMPT) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/spinlock_api_smp.h \
    $(wildcard include/config/INLINE_SPIN_LOCK) \
    $(wildcard include/config/INLINE_SPIN_LOCK_BH) \
    $(wildcard include/config/INLINE_SPIN_LOCK_IRQ) \
    $(wildcard include/config/INLINE_SPIN_LOCK_IRQSAVE) \
    $(wildcard include/config/INLINE_SPIN_TRYLOCK) \
    $(wildcard include/config/INLINE_SPIN_TRYLOCK_BH) \
    $(wildcard include/config/UNINLINE_SPIN_UNLOCK) \
    $(wildcard include/config/INLINE_SPIN_UNLOCK_BH) \
    $(wildcard include/config/INLINE_SPIN_UNLOCK_IRQ) \
    $(wildcard include/config/INLINE_SPIN_UNLOCK_IRQRESTORE) \
    $(wildcard include/config/GENERIC_LOCKBREAK) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/rwlock_api_smp.h \
    $(wildcard include/config/INLINE_READ_LOCK) \
    $(wildcard include/config/INLINE_WRITE_LOCK) \
    $(wildcard include/config/INLINE_READ_LOCK_BH) \
    $(wildcard include/config/INLINE_WRITE_LOCK_BH) \
    $(wildcard include/config/INLINE_READ_LOCK_IRQ) \
    $(wildcard include/config/INLINE_WRITE_LOCK_IRQ) \
    $(wildcard include/config/INLINE_READ_LOCK_IRQSAVE) \
    $(wildcard include/config/INLINE_WRITE_LOCK_IRQSAVE) \
    $(wildcard include/config/INLINE_READ_TRYLOCK) \
    $(wildcard include/config/INLINE_WRITE_TRYLOCK) \
    $(wildcard include/config/INLINE_READ_UNLOCK) \
    $(wildcard include/config/INLINE_WRITE_UNLOCK) \
    $(wildcard include/config/INLINE_READ_UNLOCK_BH) \
    $(wildcard include/config/INLINE_WRITE_UNLOCK_BH) \
    $(wildcard include/config/INLINE_READ_UNLOCK_IRQ) \
    $(wildcard include/config/INLINE_WRITE_UNLOCK_IRQ) \
    $(wildcard include/config/INLINE_READ_UNLOCK_IRQRESTORE) \
    $(wildcard include/config/INLINE_WRITE_UNLOCK_IRQRESTORE) \
  /root/kernel-e/kernel_platform/msm-kernel/include/uapi/linux/wait.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/refcount.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/sem.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/uapi/linux/sem.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/ipc.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/uidgid.h \
    $(wildcard include/config/MULTIUSER) \
    $(wildcard include/config/USER_NS) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/highuid.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/rhashtable-types.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/mutex.h \
    $(wildcard include/config/MUTEX_SPIN_ON_OWNER) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/osq_lock.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/debug_locks.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/workqueue.h \
    $(wildcard include/config/DEBUG_OBJECTS_WORK) \
    $(wildcard include/config/FREEZER) \
    $(wildcard include/config/SYSFS) \
    $(wildcard include/config/WQ_WATCHDOG) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/timer.h \
    $(wildcard include/config/DEBUG_OBJECTS_TIMERS) \
    $(wildcard include/config/NO_HZ_COMMON) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/ktime.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/time.h \
    $(wildcard include/config/POSIX_TIMERS) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/time32.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/timex.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/uapi/linux/timex.h \
  /root/kernel-e/kernel_platform/msm-kernel/arch/arm64/include/asm/timex.h \
  /root/kernel-e/kernel_platform/msm-kernel/arch/arm64/include/asm/arch_timer.h \
    $(wildcard include/config/ARM_ARCH_TIMER_OOL_WORKAROUND) \
  /root/kernel-e/kernel_platform/msm-kernel/include/clocksource/arm_arch_timer.h \
    $(wildcard include/config/ARM_ARCH_TIMER) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/timecounter.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/asm-generic/timex.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/vdso/time32.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/vdso/time.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/jiffies.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/vdso/jiffies.h \
  include/generated/timeconst.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/vdso/ktime.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/timekeeping.h \
    $(wildcard include/config/GENERIC_CMOS_UPDATE) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/clocksource_ids.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/debugobjects.h \
    $(wildcard include/config/DEBUG_OBJECTS) \
    $(wildcard include/config/DEBUG_OBJECTS_FREE) \
  /root/kernel-e/kernel_platform/msm-kernel/include/uapi/linux/ipc.h \
  arch/arm64/include/generated/uapi/asm/ipcbuf.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/uapi/asm-generic/ipcbuf.h \
  arch/arm64/include/generated/uapi/asm/sembuf.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/uapi/asm-generic/sembuf.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/shm.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/uapi/linux/shm.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/uapi/asm-generic/hugetlb_encode.h \
  arch/arm64/include/generated/uapi/asm/shmbuf.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/uapi/asm-generic/shmbuf.h \
  /root/kernel-e/kernel_platform/msm-kernel/arch/arm64/include/asm/shmparam.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/asm-generic/shmparam.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/plist.h \
    $(wildcard include/config/DEBUG_PLIST) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/hrtimer.h \
    $(wildcard include/config/HIGH_RES_TIMERS) \
    $(wildcard include/config/TIME_LOW_RES) \
    $(wildcard include/config/TIMERFD) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/hrtimer_defs.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/rbtree.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/rbtree_types.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/seqlock.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/ww_mutex.h \
    $(wildcard include/config/DEBUG_RT_MUTEXES) \
    $(wildcard include/config/DEBUG_WW_MUTEX_SLOWPATH) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/rtmutex.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/timerqueue.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/seccomp.h \
    $(wildcard include/config/SECCOMP) \
    $(wildcard include/config/HAVE_ARCH_SECCOMP_FILTER) \
    $(wildcard include/config/SECCOMP_FILTER) \
    $(wildcard include/config/CHECKPOINT_RESTORE) \
    $(wildcard include/config/SECCOMP_CACHE_DEBUG) \
  /root/kernel-e/kernel_platform/msm-kernel/include/uapi/linux/seccomp.h \
  /root/kernel-e/kernel_platform/msm-kernel/arch/arm64/include/asm/seccomp.h \
  /root/kernel-e/kernel_platform/msm-kernel/arch/arm64/include/asm/unistd.h \
  /root/kernel-e/kernel_platform/msm-kernel/arch/arm64/include/uapi/asm/unistd.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/uapi/asm-generic/unistd.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/asm-generic/seccomp.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/uapi/linux/unistd.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/nodemask.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/numa.h \
    $(wildcard include/config/NODES_SHIFT) \
    $(wildcard include/config/NUMA_KEEP_MEMINFO) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/resource.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/uapi/linux/resource.h \
  arch/arm64/include/generated/uapi/asm/resource.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/asm-generic/resource.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/uapi/asm-generic/resource.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/latencytop.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/sched/prio.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/sched/types.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/signal_types.h \
    $(wildcard include/config/OLD_SIGACTION) \
  /root/kernel-e/kernel_platform/msm-kernel/include/uapi/linux/signal.h \
  /root/kernel-e/kernel_platform/msm-kernel/arch/arm64/include/asm/signal.h \
  /root/kernel-e/kernel_platform/msm-kernel/arch/arm64/include/uapi/asm/signal.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/asm-generic/signal.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/uapi/asm-generic/signal.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/uapi/asm-generic/signal-defs.h \
  arch/arm64/include/generated/uapi/asm/siginfo.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/uapi/asm-generic/siginfo.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/syscall_user_dispatch.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/task_io_accounting.h \
    $(wildcard include/config/TASK_IO_ACCOUNTING) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/posix-timers.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/alarmtimer.h \
    $(wildcard include/config/RTC_CLASS) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/task_work.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/uapi/linux/rseq.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/kcsan.h \
  arch/arm64/include/generated/asm/kmap_size.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/asm-generic/kmap_size.h \
    $(wildcard include/config/DEBUG_KMAP_LOCAL) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/auxvec.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/uapi/linux/auxvec.h \
  /root/kernel-e/kernel_platform/msm-kernel/arch/arm64/include/uapi/asm/auxvec.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/kref.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/rwsem.h \
    $(wildcard include/config/RWSEM_SPIN_ON_OWNER) \
    $(wildcard include/config/DEBUG_RWSEMS) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/completion.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/swait.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/uprobes.h \
  /root/kernel-e/kernel_platform/msm-kernel/arch/arm64/include/asm/uprobes.h \
  /root/kernel-e/kernel_platform/msm-kernel/arch/arm64/include/asm/debug-monitors.h \
  /root/kernel-e/kernel_platform/msm-kernel/arch/arm64/include/asm/esr.h \
  /root/kernel-e/kernel_platform/msm-kernel/arch/arm64/include/asm/probes.h \
    $(wildcard include/config/KPROBES) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/page-flags-layout.h \
  include/generated/bounds.h \
  /root/kernel-e/kernel_platform/msm-kernel/arch/arm64/include/asm/sparsemem.h \
    $(wildcard include/config/ARM64_MEMMAP_ON_MEMORY) \
  /root/kernel-e/kernel_platform/msm-kernel/arch/arm64/include/asm/mmu.h \
  /root/kernel-e/kernel_platform/msm-kernel/arch/arm64/include/asm/pgtable-prot.h \
  /root/kernel-e/kernel_platform/msm-kernel/arch/arm64/include/asm/tlbflush.h \
    $(wildcard include/config/ARM64_WORKAROUND_REPEAT_TLBI) \
  /root/kernel-e/kernel_platform/msm-kernel/arch/arm64/include/asm/fixmap.h \
    $(wildcard include/config/ACPI_APEI_GHES) \
    $(wildcard include/config/ARM_SDE_INTERFACE) \
    $(wildcard include/config/UNMAP_KERNEL_AT_EL0) \
  /root/kernel-e/kernel_platform/msm-kernel/arch/arm64/include/asm/boot.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/asm-generic/fixmap.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/asm-generic/pgtable_uffd.h \
    $(wildcard include/config/HAVE_ARCH_USERFAULTFD_WP) \
  arch/arm64/include/generated/asm/early_ioremap.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/asm-generic/early_ioremap.h \
    $(wildcard include/config/GENERIC_EARLY_IOREMAP) \
  /root/kernel-e/kernel_platform/msm-kernel/include/asm-generic/io.h \
    $(wildcard include/config/GENERIC_IOMAP) \
    $(wildcard include/config/TRACE_MMIO_ACCESS) \
    $(wildcard include/config/GENERIC_IOREMAP) \
    $(wildcard include/config/VIRT_TO_BUS) \
    $(wildcard include/config/GENERIC_DEVMEM_IS_ALLOWED) \
  /root/kernel-e/kernel_platform/msm-kernel/include/asm-generic/pci_iomap.h \
    $(wildcard include/config/NO_GENERIC_PCI_IOPORT_MAP) \
    $(wildcard include/config/GENERIC_PCI_IOMAP) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/tracepoint-defs.h \
    $(wildcard include/config/TRACEPOINTS) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/logic_pio.h \
    $(wildcard include/config/INDIRECT_PIO) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/fwnode.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/vmalloc.h \
    $(wildcard include/config/KASAN_VMALLOC) \
    $(wildcard include/config/HAVE_ARCH_HUGE_VMALLOC) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/overflow.h \
  /root/kernel-e/kernel_platform/msm-kernel/arch/arm64/include/asm/vmalloc.h \
    $(wildcard include/config/PTDUMP_DEBUGFS) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/delay.h \
  arch/arm64/include/generated/asm/delay.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/asm-generic/delay.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/slab.h \
    $(wildcard include/config/DEBUG_SLAB) \
    $(wildcard include/config/FAILSLAB) \
    $(wildcard include/config/MEMCG_KMEM) \
    $(wildcard include/config/HAVE_HARDENED_USERCOPY_ALLOCATOR) \
    $(wildcard include/config/SLAB) \
    $(wildcard include/config/SLUB) \
    $(wildcard include/config/SLOB) \
    $(wildcard include/config/ZONE_DMA) \
    $(wildcard include/config/CC_IS_GCC) \
    $(wildcard include/config/CLANG_VERSION) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/gfp.h \
    $(wildcard include/config/CMA) \
    $(wildcard include/config/ZONE_DMA32) \
    $(wildcard include/config/ZONE_DEVICE) \
    $(wildcard include/config/PM_SLEEP) \
    $(wildcard include/config/CONTIG_ALLOC) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/mmzone.h \
    $(wildcard include/config/FORCE_MAX_ZONEORDER) \
    $(wildcard include/config/MEMORY_ISOLATION) \
    $(wildcard include/config/LRU_GEN_STATS) \
    $(wildcard include/config/MEMORY_HOTPLUG) \
    $(wildcard include/config/PAGE_EXTENSION) \
    $(wildcard include/config/DEFERRED_STRUCT_PAGE_INIT) \
    $(wildcard include/config/HAVE_MEMORYLESS_NODES) \
    $(wildcard include/config/SPARSEMEM_EXTREME) \
    $(wildcard include/config/HAVE_ARCH_PFN_VALID) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/pageblock-flags.h \
    $(wildcard include/config/HUGETLB_PAGE_SIZE_VARIABLE) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/local_lock.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/local_lock_internal.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/memory_hotplug.h \
    $(wildcard include/config/ARCH_HAS_ADD_PAGES) \
    $(wildcard include/config/HAVE_ARCH_NODEDATA_EXTENSION) \
    $(wildcard include/config/MEMORY_HOTREMOVE) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/notifier.h \
    $(wildcard include/config/TREE_SRCU) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/srcu.h \
    $(wildcard include/config/TINY_SRCU) \
    $(wildcard include/config/SRCU) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/rcu_segcblist.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/srcutree.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/rcu_node_tree.h \
    $(wildcard include/config/RCU_FANOUT) \
    $(wildcard include/config/RCU_FANOUT_LEAF) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/topology.h \
    $(wildcard include/config/USE_PERCPU_NUMA_NODE_ID) \
    $(wildcard include/config/SCHED_SMT) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/arch_topology.h \
    $(wildcard include/config/GENERIC_ARCH_TOPOLOGY) \
  /root/kernel-e/kernel_platform/msm-kernel/arch/arm64/include/asm/topology.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/asm-generic/topology.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/percpu-refcount.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/kasan.h \
    $(wildcard include/config/KASAN_STACK) \
    $(wildcard include/config/KASAN_INLINE) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/stat.h \
  /root/kernel-e/kernel_platform/msm-kernel/arch/arm64/include/asm/stat.h \
  arch/arm64/include/generated/uapi/asm/stat.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/uapi/asm-generic/stat.h \
  /root/kernel-e/kernel_platform/msm-kernel/arch/arm64/include/asm/compat.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/asm-generic/compat.h \
    $(wildcard include/config/COMPAT_FOR_U64_ALIGNMENT) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/sched/task_stack.h \
    $(wildcard include/config/STACK_GROWSUP) \
    $(wildcard include/config/DEBUG_STACK_USAGE) \
  /root/kernel-e/kernel_platform/msm-kernel/include/uapi/linux/magic.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/uapi/linux/stat.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/iopoll.h \
  /root/kernel-e/kernel_platform/msm-kernel/vendor/qcom/opensource/display-drivers/include/linux/msm_hdcp.h \
  /root/kernel-e/kernel_platform/msm-kernel/vendor/qcom/opensource/display-drivers/include/uapi/display/hdcp/msm_hdmi_hdcp_mgr.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/drm/drm_dp_helper.h \
    $(wildcard include/config/DRM_KMS_HELPER) \
    $(wildcard include/config/BACKLIGHT_CLASS_DEVICE) \
    $(wildcard include/config/DRM_DP_CEC) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/i2c.h \
    $(wildcard include/config/I2C) \
    $(wildcard include/config/I2C_SLAVE) \
    $(wildcard include/config/I2C_BOARDINFO) \
    $(wildcard include/config/I2C_MUX) \
    $(wildcard include/config/OF) \
    $(wildcard include/config/ACPI) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/acpi.h \
    $(wildcard include/config/ACPI_DEBUGGER) \
    $(wildcard include/config/X86) \
    $(wildcard include/config/IA64) \
    $(wildcard include/config/LOONGARCH) \
    $(wildcard include/config/ACPI_PROCESSOR_CSTATE) \
    $(wildcard include/config/ACPI_HOTPLUG_CPU) \
    $(wildcard include/config/ACPI_HOTPLUG_IOAPIC) \
    $(wildcard include/config/X86_IO_APIC) \
    $(wildcard include/config/ACPI_WMI) \
    $(wildcard include/config/ACPI_NUMA) \
    $(wildcard include/config/HIBERNATION) \
    $(wildcard include/config/ACPI_HOTPLUG_MEMORY) \
    $(wildcard include/config/ACPI_CONTAINER) \
    $(wildcard include/config/ACPI_GTDT) \
    $(wildcard include/config/PM) \
    $(wildcard include/config/GPIOLIB) \
    $(wildcard include/config/ACPI_TABLE_UPGRADE) \
    $(wildcard include/config/ACPI_WATCHDOG) \
    $(wildcard include/config/ACPI_SPCR_TABLE) \
    $(wildcard include/config/ACPI_GENERIC_GSI) \
    $(wildcard include/config/ACPI_LPIT) \
    $(wildcard include/config/ACPI_PPTT) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/ioport.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/irqdomain.h \
    $(wildcard include/config/IRQ_DOMAIN_HIERARCHY) \
    $(wildcard include/config/GENERIC_IRQ_DEBUGFS) \
    $(wildcard include/config/IRQ_DOMAIN) \
    $(wildcard include/config/IRQ_DOMAIN_NOMAP) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/irqhandler.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/of.h \
    $(wildcard include/config/OF_DYNAMIC) \
    $(wildcard include/config/SPARC) \
    $(wildcard include/config/OF_PROMTREE) \
    $(wildcard include/config/OF_KOBJ) \
    $(wildcard include/config/OF_NUMA) \
    $(wildcard include/config/OF_OVERLAY) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/kobject.h \
    $(wildcard include/config/UEVENT_HELPER) \
    $(wildcard include/config/DEBUG_KOBJECT_RELEASE) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/sysfs.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/kernfs.h \
    $(wildcard include/config/KERNFS) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/idr.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/radix-tree.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/xarray.h \
    $(wildcard include/config/XARRAY_MULTI) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/kobject_ns.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/mod_devicetable.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/uuid.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/uapi/linux/uuid.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/property.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/resource_ext.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/device.h \
    $(wildcard include/config/ENERGY_MODEL) \
    $(wildcard include/config/GENERIC_MSI_IRQ_DOMAIN) \
    $(wildcard include/config/PINCTRL) \
    $(wildcard include/config/GENERIC_MSI_IRQ) \
    $(wildcard include/config/DMA_OPS) \
    $(wildcard include/config/DMA_DECLARE_COHERENT) \
    $(wildcard include/config/DMA_CMA) \
    $(wildcard include/config/SWIOTLB) \
    $(wildcard include/config/ARCH_HAS_SYNC_DMA_FOR_DEVICE) \
    $(wildcard include/config/ARCH_HAS_SYNC_DMA_FOR_CPU) \
    $(wildcard include/config/ARCH_HAS_SYNC_DMA_FOR_CPU_ALL) \
    $(wildcard include/config/DMA_OPS_BYPASS) \
    $(wildcard include/config/DEVTMPFS) \
    $(wildcard include/config/SYSFS_DEPRECATED) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/dev_printk.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/ratelimit.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/energy_model.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/sched/cpufreq.h \
    $(wildcard include/config/CPU_FREQ) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/sched/topology.h \
    $(wildcard include/config/SCHED_DEBUG) \
    $(wildcard include/config/SCHED_MC) \
    $(wildcard include/config/CPU_FREQ_GOV_SCHEDUTIL) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/sched/idle.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/sched/sd_flags.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/klist.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/pm.h \
    $(wildcard include/config/VT_CONSOLE_SLEEP) \
    $(wildcard include/config/PM_CLK) \
    $(wildcard include/config/PM_GENERIC_DOMAINS) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/device/bus.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/device/class.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/device/driver.h \
  /root/kernel-e/kernel_platform/msm-kernel/arch/arm64/include/asm/device.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/pm_wakeup.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/acpi/acpi.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/acpi/platform/acenv.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/acpi/platform/acgcc.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/acpi/platform/aclinux.h \
    $(wildcard include/config/ACPI_REDUCED_HARDWARE_ONLY) \
    $(wildcard include/config/ACPI_DEBUG) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/ctype.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/acpi/acnames.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/acpi/actypes.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/acpi/acexcep.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/acpi/actbl.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/acpi/actbl1.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/acpi/actbl2.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/acpi/actbl3.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/acpi/acrestyp.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/acpi/platform/acenvex.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/acpi/platform/aclinuxex.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/acpi/platform/acgccex.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/acpi/acoutput.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/acpi/acpiosxf.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/acpi/acpixf.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/acpi/acconfig.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/acpi/acbuffer.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/acpi/acpi_numa.h \
    $(wildcard include/config/ACPI_HMAT) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/regulator/consumer.h \
    $(wildcard include/config/REGULATOR) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/suspend.h \
    $(wildcard include/config/VT) \
    $(wildcard include/config/SUSPEND) \
    $(wildcard include/config/HIBERNATION_SNAPSHOT_DEV) \
    $(wildcard include/config/PM_SLEEP_DEBUG) \
    $(wildcard include/config/PM_AUTOSLEEP) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/swap.h \
    $(wildcard include/config/DEVICE_PRIVATE) \
    $(wildcard include/config/MIGRATION) \
    $(wildcard include/config/FRONTSWAP) \
    $(wildcard include/config/MEMCG_SWAP) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/memcontrol.h \
    $(wildcard include/config/CGROUP_WRITEBACK) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/cgroup.h \
    $(wildcard include/config/CGROUP_CPUACCT) \
    $(wildcard include/config/SOCK_CGROUP_DATA) \
    $(wildcard include/config/CGROUP_DATA) \
    $(wildcard include/config/CGROUP_BPF) \
  /root/kernel-e/kernel_platform/msm-kernel/include/uapi/linux/cgroupstats.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/uapi/linux/taskstats.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/fs.h \
    $(wildcard include/config/READ_ONLY_THP_FOR_FS) \
    $(wildcard include/config/FS_POSIX_ACL) \
    $(wildcard include/config/IMA) \
    $(wildcard include/config/FILE_LOCKING) \
    $(wildcard include/config/FSNOTIFY) \
    $(wildcard include/config/FS_ENCRYPTION) \
    $(wildcard include/config/FS_VERITY) \
    $(wildcard include/config/EPOLL) \
    $(wildcard include/config/UNICODE) \
    $(wildcard include/config/QUOTA) \
    $(wildcard include/config/FS_DAX) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/wait_bit.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/kdev_t.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/uapi/linux/kdev_t.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/dcache.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/rculist_bl.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/list_bl.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/bit_spinlock.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/lockref.h \
    $(wildcard include/config/ARCH_USE_CMPXCHG_LOCKREF) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/stringhash.h \
    $(wildcard include/config/DCACHE_WORD_ACCESS) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/hash.h \
    $(wildcard include/config/HAVE_ARCH_HASH) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/path.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/list_lru.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/shrinker.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/capability.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/uapi/linux/capability.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/semaphore.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/fcntl.h \
    $(wildcard include/config/ARCH_32BIT_OFF_T) \
  /root/kernel-e/kernel_platform/msm-kernel/include/uapi/linux/fcntl.h \
  /root/kernel-e/kernel_platform/msm-kernel/arch/arm64/include/uapi/asm/fcntl.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/uapi/asm-generic/fcntl.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/uapi/linux/openat2.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/migrate_mode.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/percpu-rwsem.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/rcuwait.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/sched/signal.h \
    $(wildcard include/config/SCHED_AUTOGROUP) \
    $(wildcard include/config/BSD_PROCESS_ACCT) \
    $(wildcard include/config/TASKSTATS) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/signal.h \
    $(wildcard include/config/PROC_FS) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/sched/jobctl.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/sched/task.h \
    $(wildcard include/config/HAVE_EXIT_THREAD) \
    $(wildcard include/config/ARCH_WANTS_DYNAMIC_TASK_STRUCT) \
    $(wildcard include/config/HAVE_ARCH_THREAD_STRUCT_WHITELIST) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/uaccess.h \
    $(wildcard include/config/SET_FS) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/fault-inject-usercopy.h \
    $(wildcard include/config/FAULT_INJECTION_USERCOPY) \
  /root/kernel-e/kernel_platform/msm-kernel/arch/arm64/include/asm/uaccess.h \
  /root/kernel-e/kernel_platform/msm-kernel/arch/arm64/include/asm/kernel-pgtable.h \
    $(wildcard include/config/RANDOMIZE_BASE) \
  /root/kernel-e/kernel_platform/msm-kernel/arch/arm64/include/asm/extable.h \
    $(wildcard include/config/BPF_JIT) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/cred.h \
    $(wildcard include/config/DEBUG_CREDENTIALS) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/key.h \
    $(wildcard include/config/KEY_NOTIFICATIONS) \
    $(wildcard include/config/NET) \
    $(wildcard include/config/SYSCTL) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/sysctl.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/uapi/linux/sysctl.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/assoc_array.h \
    $(wildcard include/config/ASSOCIATIVE_ARRAY) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/sched/user.h \
    $(wildcard include/config/WATCH_QUEUE) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/percpu_counter.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/rcu_sync.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/delayed_call.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/errseq.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/ioprio.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/sched/rt.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/iocontext.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/uapi/linux/ioprio.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/fs_types.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/mount.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/uapi/linux/fs.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/quota.h \
    $(wildcard include/config/QUOTA_NETLINK_INTERFACE) \
  /root/kernel-e/kernel_platform/msm-kernel/include/uapi/linux/dqblk_xfs.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/dqblk_v1.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/dqblk_v2.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/dqblk_qtree.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/projid.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/uapi/linux/quota.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/nfs_fs_i.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/seq_file.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/ns_common.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/nsproxy.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/user_namespace.h \
    $(wildcard include/config/INOTIFY_USER) \
    $(wildcard include/config/FANOTIFY) \
    $(wildcard include/config/PERSISTENT_KEYRINGS) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/kernel_stat.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/interrupt.h \
    $(wildcard include/config/IRQ_FORCED_THREADING) \
    $(wildcard include/config/GENERIC_IRQ_PROBE) \
    $(wildcard include/config/IRQ_TIMINGS) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/irqreturn.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/hardirq.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/context_tracking_state.h \
    $(wildcard include/config/CONTEXT_TRACKING) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/ftrace_irq.h \
    $(wildcard include/config/HWLAT_TRACER) \
    $(wildcard include/config/OSNOISE_TRACER) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/vtime.h \
    $(wildcard include/config/VIRT_CPU_ACCOUNTING) \
    $(wildcard include/config/IRQ_TIME_ACCOUNTING) \
  /root/kernel-e/kernel_platform/msm-kernel/arch/arm64/include/asm/hardirq.h \
  /root/kernel-e/kernel_platform/msm-kernel/arch/arm64/include/asm/irq.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/asm-generic/irq.h \
  /root/kernel-e/kernel_platform/msm-kernel/arch/arm64/include/asm/kvm_arm.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/asm-generic/hardirq.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/irq.h \
    $(wildcard include/config/GENERIC_IRQ_EFFECTIVE_AFF_MASK) \
    $(wildcard include/config/GENERIC_IRQ_IPI) \
    $(wildcard include/config/GENERIC_IRQ_MIGRATION) \
    $(wildcard include/config/GENERIC_PENDING_IRQ) \
    $(wildcard include/config/HARDIRQS_SW_RESEND) \
    $(wildcard include/config/GENERIC_IRQ_LEGACY) \
    $(wildcard include/config/GENERIC_IRQ_MULTI_HANDLER) \
  arch/arm64/include/generated/asm/irq_regs.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/asm-generic/irq_regs.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/irqdesc.h \
    $(wildcard include/config/SPARSE_IRQ) \
    $(wildcard include/config/HANDLE_DOMAIN_IRQ) \
  arch/arm64/include/generated/asm/hw_irq.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/asm-generic/hw_irq.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/cgroup-defs.h \
    $(wildcard include/config/CGROUP_NET_CLASSID) \
    $(wildcard include/config/CGROUP_NET_PRIO) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/u64_stats_sync.h \
  arch/arm64/include/generated/asm/local64.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/asm-generic/local64.h \
  arch/arm64/include/generated/asm/local.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/asm-generic/local.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/bpf-cgroup.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/bpf.h \
    $(wildcard include/config/BPF_JIT_ALWAYS_ON) \
    $(wildcard include/config/INET) \
  /root/kernel-e/kernel_platform/msm-kernel/include/uapi/linux/bpf.h \
    $(wildcard include/config/BPF_LIRC_MODE2) \
    $(wildcard include/config/EFFICIENT_UNALIGNED_ACCESS) \
    $(wildcard include/config/IP_ROUTE_CLASSID) \
    $(wildcard include/config/BPF_KPROBE_OVERRIDE) \
    $(wildcard include/config/FUNCTION_ERROR_INJECTION) \
    $(wildcard include/config/XFRM) \
  /root/kernel-e/kernel_platform/msm-kernel/include/uapi/linux/bpf_common.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/file.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/rbtree_latch.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/module.h \
    $(wildcard include/config/MODULES_TREE_LOOKUP) \
    $(wildcard include/config/STACKTRACE_BUILD_ID) \
    $(wildcard include/config/KALLSYMS) \
    $(wildcard include/config/BPF_EVENTS) \
    $(wildcard include/config/DEBUG_INFO_BTF_MODULES) \
    $(wildcard include/config/EVENT_TRACING) \
    $(wildcard include/config/MODULE_UNLOAD) \
    $(wildcard include/config/CONSTRUCTORS) \
    $(wildcard include/config/RETPOLINE) \
    $(wildcard include/config/MODULE_SIG) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/buildid.h \
    $(wildcard include/config/CRASH_CORE) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/kmod.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/umh.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/elf.h \
    $(wildcard include/config/ARCH_USE_GNU_PROPERTY) \
    $(wildcard include/config/ARCH_HAVE_ELF_PROT) \
  /root/kernel-e/kernel_platform/msm-kernel/arch/arm64/include/asm/elf.h \
    $(wildcard include/config/COMPAT_VDSO) \
  arch/arm64/include/generated/asm/user.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/asm-generic/user.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/uapi/linux/elf.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/uapi/linux/elf-em.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/moduleparam.h \
    $(wildcard include/config/ALPHA) \
    $(wildcard include/config/PPC64) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/error-injection.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/asm-generic/error-injection.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/cfi.h \
    $(wildcard include/config/CFI_CLANG_SHADOW) \
  /root/kernel-e/kernel_platform/msm-kernel/arch/arm64/include/asm/module.h \
    $(wildcard include/config/ARM64_MODULE_PLTS) \
    $(wildcard include/config/DYNAMIC_FTRACE) \
    $(wildcard include/config/ARM64_ERRATUM_843419) \
  /root/kernel-e/kernel_platform/msm-kernel/include/asm-generic/module.h \
    $(wildcard include/config/HAVE_MOD_ARCH_SPECIFIC) \
    $(wildcard include/config/MODULES_USE_ELF_REL) \
    $(wildcard include/config/MODULES_USE_ELF_RELA) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/kallsyms.h \
    $(wildcard include/config/KALLSYMS_ALL) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/mm.h \
    $(wildcard include/config/HAVE_ARCH_MMAP_RND_BITS) \
    $(wildcard include/config/HAVE_ARCH_MMAP_RND_COMPAT_BITS) \
    $(wildcard include/config/DEBUG_INFO_BTF) \
    $(wildcard include/config/MEM_SOFT_DIRTY) \
    $(wildcard include/config/ARCH_USES_HIGH_VMA_FLAGS) \
    $(wildcard include/config/ARCH_HAS_PKEYS) \
    $(wildcard include/config/PPC) \
    $(wildcard include/config/PARISC) \
    $(wildcard include/config/SPARC64) \
    $(wildcard include/config/HAVE_ARCH_USERFAULTFD_MINOR) \
    $(wildcard include/config/SHMEM) \
    $(wildcard include/config/DEV_PAGEMAP_OPS) \
    $(wildcard include/config/PCI_P2PDMA) \
    $(wildcard include/config/ARCH_HAS_PTE_SPECIAL) \
    $(wildcard include/config/DEBUG_VM_RB) \
    $(wildcard include/config/PAGE_POISONING) \
    $(wildcard include/config/INIT_ON_ALLOC_DEFAULT_ON) \
    $(wildcard include/config/INIT_ON_FREE_DEFAULT_ON) \
    $(wildcard include/config/DEBUG_PAGEALLOC) \
    $(wildcard include/config/HUGETLBFS) \
    $(wildcard include/config/MAPPING_DIRTY_HELPERS) \
    $(wildcard include/config/ANON_VMA_NAME) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/mmap_lock.h \
    $(wildcard include/config/SPECULATIVE_PAGE_FAULT_STATS) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/vm_event.h \
    $(wildcard include/config/VM_EVENT_COUNTERS) \
    $(wildcard include/config/DEBUG_TLBFLUSH) \
    $(wildcard include/config/DEBUG_VM_VMACACHE) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/vm_event_item.h \
    $(wildcard include/config/MEMORY_BALLOON) \
    $(wildcard include/config/BALLOON_COMPACTION) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/range.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/page_ext.h \
    $(wildcard include/config/PAGE_PINNER) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/stacktrace.h \
    $(wildcard include/config/STACKTRACE) \
    $(wildcard include/config/ARCH_STACKWALK) \
    $(wildcard include/config/HAVE_RELIABLE_STACKTRACE) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/stackdepot.h \
    $(wildcard include/config/STACKDEPOT) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/page_ref.h \
    $(wildcard include/config/DEBUG_PAGE_REF) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/memremap.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/huge_mm.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/sched/coredump.h \
    $(wildcard include/config/CORE_DUMP_DEFAULT_ELF_HEADERS) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/vmstat.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/sched/mm.h \
    $(wildcard include/config/ARCH_HAS_MEMBARRIER_CALLBACKS) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/sync_core.h \
    $(wildcard include/config/ARCH_HAS_SYNC_CORE_BEFORE_USERMODE) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/bpfptr.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/sockptr.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/bpf_types.h \
    $(wildcard include/config/BPF_LSM) \
    $(wildcard include/config/XDP_SOCKETS) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/psi_types.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/kthread.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/cgroup_subsys.h \
    $(wildcard include/config/CGROUP_DEVICE) \
    $(wildcard include/config/CGROUP_FREEZER) \
    $(wildcard include/config/CGROUP_PERF) \
    $(wildcard include/config/CGROUP_HUGETLB) \
    $(wildcard include/config/CGROUP_PIDS) \
    $(wildcard include/config/CGROUP_RDMA) \
    $(wildcard include/config/CGROUP_MISC) \
    $(wildcard include/config/CGROUP_DEBUG) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/page_counter.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/vmpressure.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/eventfd.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/writeback.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/flex_proportions.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/backing-dev-defs.h \
    $(wildcard include/config/DEBUG_FS) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/blk_types.h \
    $(wildcard include/config/FAIL_MAKE_REQUEST) \
    $(wildcard include/config/BLK_CGROUP_IOCOST) \
    $(wildcard include/config/BLK_INLINE_ENCRYPTION) \
    $(wildcard include/config/DM_DEFAULT_KEY) \
    $(wildcard include/config/BLK_DEV_INTEGRITY) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/bvec.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/highmem.h \
  /root/kernel-e/kernel_platform/msm-kernel/arch/arm64/include/asm/cacheflush.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/kgdb.h \
    $(wildcard include/config/HAVE_ARCH_KGDB) \
    $(wildcard include/config/KGDB) \
    $(wildcard include/config/SERIAL_KGDB_NMI) \
    $(wildcard include/config/KGDB_HONOUR_BLOCKLIST) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/kprobes.h \
    $(wildcard include/config/KPROBES_SANITY_TEST) \
    $(wildcard include/config/OPTPROBES) \
    $(wildcard include/config/KPROBES_ON_FTRACE) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/ftrace.h \
    $(wildcard include/config/FUNCTION_TRACER) \
    $(wildcard include/config/HAVE_DYNAMIC_FTRACE_WITH_ARGS) \
    $(wildcard include/config/DYNAMIC_FTRACE_WITH_REGS) \
    $(wildcard include/config/DYNAMIC_FTRACE_WITH_DIRECT_CALLS) \
    $(wildcard include/config/HAVE_DYNAMIC_FTRACE_WITH_DIRECT_CALLS) \
    $(wildcard include/config/STACK_TRACER) \
    $(wildcard include/config/FRAME_POINTER) \
    $(wildcard include/config/FUNCTION_PROFILER) \
    $(wildcard include/config/FTRACE_SYSCALLS) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/trace_recursion.h \
    $(wildcard include/config/FTRACE_RECORD_RECURSION) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/trace_clock.h \
  arch/arm64/include/generated/asm/trace_clock.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/asm-generic/trace_clock.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/ptrace.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/pid_namespace.h \
    $(wildcard include/config/PID_NS) \
  /root/kernel-e/kernel_platform/msm-kernel/include/uapi/linux/ptrace.h \
  /root/kernel-e/kernel_platform/msm-kernel/arch/arm64/include/asm/ftrace.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/compat.h \
    $(wildcard include/config/ARCH_HAS_SYSCALL_WRAPPER) \
    $(wildcard include/config/X86_X32_ABI) \
    $(wildcard include/config/COMPAT_OLD_SIGACTION) \
    $(wildcard include/config/ODD_RT_SIGACTION) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/socket.h \
  arch/arm64/include/generated/uapi/asm/socket.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/uapi/asm-generic/socket.h \
  arch/arm64/include/generated/uapi/asm/sockios.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/uapi/asm-generic/sockios.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/uapi/linux/sockios.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/uio.h \
    $(wildcard include/config/ARCH_HAS_COPY_MC) \
  /root/kernel-e/kernel_platform/msm-kernel/include/uapi/linux/uio.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/uapi/linux/socket.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/uapi/linux/if.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/uapi/linux/libc-compat.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/uapi/linux/hdlc/ioctl.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/uapi/linux/aio_abi.h \
  /root/kernel-e/kernel_platform/msm-kernel/arch/arm64/include/asm/syscall_wrapper.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/freelist.h \
  /root/kernel-e/kernel_platform/msm-kernel/arch/arm64/include/asm/kprobes.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/asm-generic/kprobes.h \
  /root/kernel-e/kernel_platform/msm-kernel/arch/arm64/include/asm/kgdb.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/asm-generic/cacheflush.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/highmem-internal.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/blk-cgroup.h \
    $(wildcard include/config/BLK_CGROUP_FC_APPID) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/blkdev.h \
    $(wildcard include/config/BLK_RQ_ALLOC_TIME) \
    $(wildcard include/config/BLK_WBT) \
    $(wildcard include/config/BLK_DEV_ZONED) \
    $(wildcard include/config/BLK_DEV_THROTTLING) \
    $(wildcard include/config/BLK_DEBUG_FS) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/sched/clock.h \
    $(wildcard include/config/HAVE_UNSTABLE_SCHED_CLOCK) \
  /root/kernel-e/kernel_platform/msm-kernel/include/uapi/linux/major.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/genhd.h \
    $(wildcard include/config/BLOCK_HOLDER_DEPRECATED) \
    $(wildcard include/config/CDROM) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/mempool.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/bio.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/scatterlist.h \
    $(wildcard include/config/NEED_SG_DMA_LENGTH) \
    $(wildcard include/config/DEBUG_SG) \
    $(wildcard include/config/SGL_ALLOC) \
    $(wildcard include/config/ARCH_NO_SG_CHAIN) \
    $(wildcard include/config/SG_POOL) \
  /root/kernel-e/kernel_platform/msm-kernel/include/uapi/linux/blkzoned.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/sbitmap.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/elevator.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/hashtable.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/blk-mq.h \
    $(wildcard include/config/FAIL_IO_TIMEOUT) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/node.h \
    $(wildcard include/config/HMEM_REPORTING) \
    $(wildcard include/config/MEMORY_HOTPLUG_SPARSE) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/pagemap.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/hugetlb_inline.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/sched/debug.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/uapi/linux/mempolicy.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/freezer.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/uapi/linux/i2c.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/drm/drm_connector.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/hdmi.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/drm/drm_mode_object.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/drm/drm_lease.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/drm/drm_util.h \
    $(wildcard include/config/DRM_EXPORT_FOR_TESTS) \
  /root/kernel-e/kernel_platform/msm-kernel/include/uapi/drm/drm_mode.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/uapi/drm/drm.h \
  /root/kernel-e/kernel_platform/msm-kernel/vendor/qcom/opensource/display-drivers/msm/sde_hdcp.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/debugfs.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/of_device.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/cpu.h \
    $(wildcard include/config/PM_SLEEP_SMP) \
    $(wildcard include/config/PM_SLEEP_SMP_NONZERO_CPU) \
    $(wildcard include/config/HOTPLUG_SMT) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/cpuhotplug.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/platform_device.h \
    $(wildcard include/config/HIBERNATE_CALLBACKS) \
    $(wildcard include/config/SUPERH) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/of_platform.h \
    $(wildcard include/config/OF_ADDRESS) \
  /root/kernel-e/kernel_platform/msm-kernel/include/drm/drm_crtc.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/fb.h \
    $(wildcard include/config/GUMSTIX_AM200EPD) \
    $(wildcard include/config/FB_NOTIFY) \
    $(wildcard include/config/FB_DEFERRED_IO) \
    $(wildcard include/config/FB_TILEBLITTING) \
    $(wildcard include/config/FB_BACKLIGHT) \
    $(wildcard include/config/FB_FOREIGN_ENDIAN) \
    $(wildcard include/config/FB_BOTH_ENDIAN) \
    $(wildcard include/config/FB_BIG_ENDIAN) \
    $(wildcard include/config/FB_LITTLE_ENDIAN) \
  /root/kernel-e/kernel_platform/msm-kernel/include/uapi/linux/fb.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/backlight.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/uapi/linux/media-bus-format.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/uapi/drm/drm_fourcc.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/drm/drm_modeset_lock.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/drm/drm_rect.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/drm/drm_framebuffer.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/drm/drm_fourcc.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/drm/drm_modes.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/drm/drm_device.h \
    $(wildcard include/config/DRM_LEGACY) \
  /root/kernel-e/kernel_platform/msm-kernel/include/drm/drm_hashtab.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/drm/drm_mode_config.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/drm/drm_property.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/drm/drm_edid.h \
    $(wildcard include/config/DRM_LOAD_EDID_FIRMWARE) \
  /root/kernel-e/kernel_platform/msm-kernel/include/drm/drm_plane.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/drm/drm_color_mgmt.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/drm/drm_blend.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/drm/drm_debugfs_crc.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/hdcp_qseecom.h \
  /root/kernel-e/kernel_platform/msm-kernel/vendor/qcom/opensource/display-drivers/msm/sde/sde_kms.h \
    $(wildcard include/config/DRM_LOW_MSM_MEM_FOOTPRINT) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/pm_domain.h \
    $(wildcard include/config/CPU_IDLE) \
    $(wildcard include/config/PM_GENERIC_DOMAINS_SLEEP) \
    $(wildcard include/config/PM_GENERIC_DOMAINS_OF) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/pm_qos.h \
  /root/kernel-e/kernel_platform/msm-kernel/vendor/qcom/opensource/display-drivers/msm/msm_drv.h \
    $(wildcard include/config/DRM_MSM_HDMI) \
    $(wildcard include/config/DRM_MSM_EDP) \
    $(wildcard include/config/DRM_MSM_MDP5) \
    $(wildcard include/config/MSM_SDE_ROTATOR) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/clk.h \
    $(wildcard include/config/COMMON_CLK) \
    $(wildcard include/config/HAVE_CLK_PREPARE) \
    $(wildcard include/config/HAVE_CLK) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/cpufreq.h \
    $(wildcard include/config/CPU_FREQ_STAT) \
    $(wildcard include/config/CPU_THERMAL) \
    $(wildcard include/config/PM_OPP) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/pm_opp.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/component.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/pm_runtime.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/iommu.h \
    $(wildcard include/config/IOMMU_API) \
    $(wildcard include/config/IOMMU_DEBUGFS) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/ioasid.h \
    $(wildcard include/config/IOASID) \
  /root/kernel-e/kernel_platform/msm-kernel/include/uapi/linux/iommu.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/of_graph.h \
  /root/kernel-e/kernel_platform/msm-kernel/vendor/qcom/opensource/display-drivers/include/linux/sde_io_util.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/gpio.h \
    $(wildcard include/config/ARCH_HAVE_CUSTOM_GPIO_H) \
  /root/kernel-e/kernel_platform/msm-kernel/include/asm-generic/gpio.h \
    $(wildcard include/config/ARCH_NR_GPIO) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/gpio/driver.h \
    $(wildcard include/config/GPIO_GENERIC) \
    $(wildcard include/config/GPIOLIB_IRQCHIP) \
    $(wildcard include/config/OF_GPIO) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/irqchip/chained_irq.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/pinctrl/pinctrl.h \
    $(wildcard include/config/GENERIC_PINCONF) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/pinctrl/pinctrl-state.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/pinctrl/devinfo.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/pinctrl/consumer.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/pinctrl/pinconf-generic.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/pinctrl/machine.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/gpio/consumer.h \
    $(wildcard include/config/GPIO_SYSFS) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/soc/qcom/msm_mmrm.h \
  /root/kernel-e/kernel_platform/msm-kernel/vendor/qcom/opensource/display-drivers/include/linux/sde_vm_event.h \
  include/generated/uapi/linux/version.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/drm/drm_atomic.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/drm/drm_atomic_helper.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/drm/drm_modeset_helper_vtables.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/drm/drm_encoder.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/drm/drm_modeset_helper.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/drm/drm_atomic_state_helper.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/drm/drm_plane_helper.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/drm/drm_fb_helper.h \
    $(wildcard include/config/DRM_FBDEV_EMULATION) \
  /root/kernel-e/kernel_platform/msm-kernel/include/drm/drm_client.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/dma-buf-map.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/uapi/drm/msm_drm.h \
  /root/kernel-e/kernel_platform/msm-kernel/vendor/qcom/opensource/display-drivers/include/uapi/display/drm/sde_drm.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/drm/drm_file.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/drm/drm_prime.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/drm/drm_gem.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/dma-resv.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/dma-fence.h \
    $(wildcard include/config/DMA_FENCE_TRACE) \
  /root/kernel-e/kernel_platform/msm-kernel/include/drm/drm_vma_manager.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/drm/drm_mm.h \
    $(wildcard include/config/DRM_DEBUG_MM) \
  /root/kernel-e/kernel_platform/msm-kernel/include/drm/drm_print.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/drm/drm_dsc.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/drm/drm_bridge.h \
    $(wildcard include/config/DRM_PANEL_BRIDGE) \
  /root/kernel-e/kernel_platform/msm-kernel/vendor/qcom/opensource/display-drivers/msm/sde_power_handle.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/interconnect.h \
    $(wildcard include/config/INTERCONNECT) \
  /root/kernel-e/kernel_platform/msm-kernel/vendor/qcom/opensource/display-drivers/msm/msm_kms.h \
    $(wildcard include/config/DRM_MSM_MDP4) \
  /root/kernel-e/kernel_platform/msm-kernel/vendor/qcom/opensource/display-drivers/msm/msm_mmu.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/qcom-iommu-util.h \
    $(wildcard include/config/IOMMU_IO_PGTABLE_LPAE) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/dma-mapping.h \
    $(wildcard include/config/DMA_API_DEBUG) \
    $(wildcard include/config/HAS_DMA) \
    $(wildcard include/config/NEED_DMA_MAP_STATE) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/dma-direction.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/mem_encrypt.h \
    $(wildcard include/config/ARCH_HAS_MEM_ENCRYPT) \
  /root/kernel-e/kernel_platform/msm-kernel/arch/arm64/include/asm/mem_encrypt.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/iova.h \
    $(wildcard include/config/IOMMU_IOVA) \
  /root/kernel-e/kernel_platform/msm-kernel/include/soc/qcom/secure_buffer.h \
    $(wildcard include/config/QCOM_SECURE_BUFFER) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/dma-map-ops.h \
    $(wildcard include/config/DMA_PERNUMA_CMA) \
    $(wildcard include/config/DMA_GLOBAL_POOL) \
    $(wildcard include/config/ARCH_HAS_DMA_COHERENCE_H) \
    $(wildcard include/config/ARCH_HAS_DMA_PREP_COHERENT) \
    $(wildcard include/config/ARCH_HAS_DMA_MARK_CLEAN) \
    $(wildcard include/config/ARCH_HAS_DMA_MAP_DIRECT) \
    $(wildcard include/config/ARCH_HAS_SETUP_DMA_OPS) \
    $(wildcard include/config/ARCH_HAS_TEARDOWN_DMA_OPS) \
  arch/arm64/include/generated/asm/dma-mapping.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/asm-generic/dma-mapping.h \
  /root/kernel-e/kernel_platform/msm-kernel/vendor/qcom/opensource/display-drivers/msm/msm_gem.h \
  /root/kernel-e/kernel_platform/msm-kernel/vendor/qcom/opensource/display-drivers/msm/sde_dbg.h \
    $(wildcard include/config/DRM_MSM_LOW_MEM_FOOTPRINT) \
  /root/kernel-e/kernel_platform/msm-kernel/include/soc/qcom/minidump.h \
    $(wildcard include/config/ARCH_QTI_VM) \
    $(wildcard include/config/QCOM_MINIDUMP) \
    $(wildcard include/config/QCOM_MINIDUMP_PANIC_DUMP) \
    $(wildcard include/config/QCOM_VA_MINIDUMP) \
  /root/kernel-e/kernel_platform/msm-kernel/vendor/qcom/opensource/display-drivers/msm/sde/sde_hw_catalog.h \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/of_fdt.h \
    $(wildcard include/config/OF_FLATTREE) \
    $(wildcard include/config/OF_EARLY_FLATTREE) \
    $(wildcard include/config/CMDLINE_FORCE) \
    $(wildcard include/config/CMDLINE) \
    $(wildcard include/config/CMDLINE_EXTEND) \
  /root/kernel-e/kernel_platform/msm-kernel/vendor/qcom/opensource/display-drivers/msm/sde/sde_hw_mdss.h \
  /root/kernel-e/kernel_platform/msm-kernel/vendor/qcom/opensource/display-drivers/include/uapi/display/drm/msm_drm_pp.h \
  /root/kernel-e/kernel_platform/msm-kernel/vendor/qcom/opensource/display-drivers/msm/sde/sde_hw_ctl.h \
  /root/kernel-e/kernel_platform/msm-kernel/vendor/qcom/opensource/display-drivers/msm/sde/sde_hw_util.h \
  /root/kernel-e/kernel_platform/msm-kernel/vendor/qcom/opensource/display-drivers/msm/sde/sde_hw_sspp.h \
  /root/kernel-e/kernel_platform/msm-kernel/vendor/qcom/opensource/display-drivers/msm/sde/sde_reg_dma.h \
  /root/kernel-e/kernel_platform/msm-kernel/vendor/qcom/opensource/display-drivers/msm/sde/sde_hw_top.h \
  /root/kernel-e/kernel_platform/msm-kernel/vendor/qcom/opensource/display-drivers/msm/sde/sde_formats.h \
  /root/kernel-e/kernel_platform/msm-kernel/vendor/qcom/opensource/display-drivers/msm/sde/sde_color_processing.h \
  /root/kernel-e/kernel_platform/msm-kernel/vendor/qcom/opensource/display-drivers/msm/sde/sde_hw_vbif.h \
  /root/kernel-e/kernel_platform/msm-kernel/vendor/qcom/opensource/display-drivers/msm/sde/sde_fence.h \
    $(wildcard include/config/SW_SYNC) \
  /root/kernel-e/kernel_platform/msm-kernel/include/linux/soc/qcom/msm_hw_fence.h \
  /root/kernel-e/kernel_platform/msm-kernel/vendor/qcom/opensource/display-drivers/msm/sde/sde_hw_lm.h \
  /root/kernel-e/kernel_platform/msm-kernel/vendor/qcom/opensource/display-drivers/msm/sde/sde_hw_pingpong.h \
  /root/kernel-e/kernel_platform/msm-kernel/vendor/qcom/opensource/display-drivers/msm/sde/sde_hw_interrupts.h \
  /root/kernel-e/kernel_platform/msm-kernel/vendor/qcom/opensource/display-drivers/msm/sde/sde_hwio.h \
  /root/kernel-e/kernel_platform/msm-kernel/vendor/qcom/opensource/display-drivers/msm/sde/sde_hw_wb.h \
  /root/kernel-e/kernel_platform/msm-kernel/vendor/qcom/opensource/display-drivers/msm/sde/sde_hw_uidle.h \
  /root/kernel-e/kernel_platform/msm-kernel/vendor/qcom/opensource/display-drivers/msm/sde/sde_rm.h \
  /root/kernel-e/kernel_platform/msm-kernel/vendor/qcom/opensource/display-drivers/msm/sde/sde_irq.h \
  /root/kernel-e/kernel_platform/msm-kernel/vendor/qcom/opensource/display-drivers/msm/sde/sde_core_perf.h \
  /root/kernel-e/kernel_platform/msm-kernel/vendor/qcom/opensource/display-drivers/msm/dp/dp_reg.h \

/root/kernel-e/kernel_platform/msm-kernel/vendor/qcom/opensource/display-drivers/msm/sde_hdcp_1x.o: $(deps_/root/kernel-e/kernel_platform/msm-kernel/vendor/qcom/opensource/display-drivers/msm/sde_hdcp_1x.o)

$(deps_/root/kernel-e/kernel_platform/msm-kernel/vendor/qcom/opensource/display-drivers/msm/sde_hdcp_1x.o):
