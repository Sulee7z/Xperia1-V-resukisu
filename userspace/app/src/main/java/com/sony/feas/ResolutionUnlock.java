package com.sony.feas;

import android.content.ContentResolver;
import android.content.Context;
import android.os.Build;
import android.provider.Settings;
import android.util.AttributeSet;
import android.view.Display;
import android.view.DisplayInfo;

import java.lang.reflect.Constructor;
import java.lang.reflect.Field;
import java.lang.reflect.Method;
import java.util.Arrays;
import java.util.Comparator;
import java.util.Locale;

import io.github.libxposed.api.XposedInterface;
import io.github.libxposed.api.XposedInterface.Chain;
import io.github.libxposed.api.XposedInterface.ExceptionMode;
import io.github.libxposed.api.XposedInterface.Hooker;

/**
 * 分辨率解锁(libxposed 移植,原 pdx234-resolution-unlock 的 XposedInit.java)。
 *
 * 原实现基于传统 XposedBridge API(de.robv.android.xposed),本文件将其
 * 重写为 libxposed 现代 API(io.github.libxposed.api)以并入 FEAS 单 APK:
 *  - system_server:  DisplayManagerService$BinderService.getDisplayInfo(int)
 *                     排序 supportedModes,把当前模式排第一(修复部分 App 依赖
 *                     "第一个模式就是当前模式" 的错误假设)
 *  - com.android.settings:
 *      * Android 14+: 改 RemovePreference.mTargetKey,阻止 Sony 移除分辨率设置项
 *      * Android 13:  ScreenResolutionController / ScreenResolutionFragment
 *                     显示 1096×2560 / 1644×3840 选项,选择时持久化到
 *                     Settings.Global.user_preferred_resolution_*
 *
 * 关键 API 语义(libxposed API 102 官方源码确认):
 *  - Chain.getArgs() 返回不可变 List,改参数必须构造新数组后 chain.proceed(args)
 *  - 不调用 proceed() 直接 return = 替换返回值(void/构造返回值被忽略)
 *  - exceptionMode=PROTECTIVE: hooker 自身异常被框架吞掉并按无 hook 继续
 *  - 本模块 targetApiVersion=102,禁止调用 legacy de.robv.android.xposed API
 */
final class ResolutionUnlock {

    private static final String TAG = "FEAS-Res";

    private ResolutionUnlock() {
    }

    /** system_server: DisplayManagerService$BinderService.getDisplayInfo(int)。 */
    static void hookSystemServer(XposedInterface api, ClassLoader loader) throws Throwable {
        Class<?> binderService = Class.forName(
                "com.android.server.display.DisplayManagerService$BinderService",
                false, loader);
        Method getDisplayInfo = findMethod(binderService, "getDisplayInfo", int.class);
        if (getDisplayInfo == null) {
            throw new NoSuchMethodException("DisplayManagerService$BinderService.getDisplayInfo(int)");
        }
        api.deoptimize(getDisplayInfo);
        api.hook(getDisplayInfo).setExceptionMode(ExceptionMode.PROTECTIVE)
                .intercept(new Hooker() {
                    @Override
                    public Object intercept(Chain chain) throws Throwable {
                        int displayId = (Integer) chain.getArg(0);
                        Object result = chain.proceed();
                        if (displayId != 0 || !(result instanceof DisplayInfo)) {
                            return result;
                        }
                        DisplayInfo info = (DisplayInfo) result;
                        Display.Mode[] supported = info.supportedModes;
                        if (supported == null || supported.length == 0) {
                            return result;
                        }
                        final Display.Mode active = info.getMode();
                        Display.Mode[] sorted = Arrays.copyOf(supported, supported.length);
                        Arrays.sort(sorted, new Comparator<Display.Mode>() {
                            @Override
                            public int compare(Display.Mode o1, Display.Mode o2) {
                                return Integer.compare(rank(o1), rank(o2));
                            }

                            private int rank(Display.Mode m) {
                                if (m.getModeId() == active.getModeId()) return 0;
                                if (m.getPhysicalWidth() == active.getPhysicalWidth()
                                        && m.getPhysicalHeight() == active.getPhysicalHeight()) {
                                    return 1;
                                }
                                return 2;
                            }
                        });
                        DisplayInfo copy = new DisplayInfo(info);
                        copy.supportedModes = sorted;
                        return copy;
                    }
                });
    }

    /** com.android.settings: 分辨率选项显示 + 选择持久化。 */
    static void hookSettings(XposedInterface api, ClassLoader loader) throws Throwable {
        if (Build.VERSION.SDK_INT >= 34) {
            hookRemovePreference(api, loader);
        }
        if (Build.VERSION.SDK_INT == 33) {
            hookScreenResolutionSdk33(api, loader);
        }
    }

    /** Android 14+: 阻止 Sony RemovePreference 移除分辨率设置项。 */
    private static void hookRemovePreference(XposedInterface api, ClassLoader loader)
            throws Throwable {
        Class<?> removePref = Class.forName(
                "com.sonymobile.settings.preference.RemovePreference", false, loader);
        final Field targetKey = findField(removePref, "mTargetKey");
        if (targetKey == null) {
            throw new NoSuchFieldException("RemovePreference.mTargetKey");
        }
        Constructor<?> ctor = findConstructor(
                removePref, Context.class, AttributeSet.class, int.class);
        if (ctor == null) {
            throw new NoSuchMethodException("RemovePreference(Context, AttributeSet, int)");
        }
        api.deoptimize(ctor);
        api.hook(ctor).setExceptionMode(ExceptionMode.PROTECTIVE)
                .intercept(new Hooker() {
                    @Override
                    public Object intercept(Chain chain) throws Throwable {
                        Object result = chain.proceed();
                        Object self = chain.getThisObject();
                        if (self != null) {
                            Object key = targetKey.get(self);
                            if ("screen_resolution".equals(key)) {
                                targetKey.set(self, "screen_resolution_1145141919");
                            }
                        }
                        return result;
                    }
                });
    }

    /** Android 13: ScreenResolutionController + ScreenResolutionFragment。 */
    private static void hookScreenResolutionSdk33(XposedInterface api, ClassLoader loader)
            throws Throwable {
        Class<?> controller = Class.forName(
                "com.android.settings.display.ScreenResolutionController", false, loader);

        // getAvailabilityStatus() -> 0 (AVAILABLE),让设置页显示分辨率选项
        Method getAvailabilityStatus = findMethod(controller, "getAvailabilityStatus");
        if (getAvailabilityStatus != null) {
            api.deoptimize(getAvailabilityStatus);
            api.hook(getAvailabilityStatus).setExceptionMode(ExceptionMode.PROTECTIVE)
                    .intercept(new Hooker() {
                        @Override
                        public Object intercept(Chain chain) {
                            return 0; // BasePreferenceController.AVAILABLE
                        }
                    });
        }

        // getDisplayWidth() -> 把 1644/1096 映射成 1440/1080,让系统设置知道
        // "这个分辨率属于支持列表"(1V 的 supportedModes 宽度是 1440/1080 归一化值)
        Method getDisplayWidth = findMethod(controller, "getDisplayWidth");
        if (getDisplayWidth != null) {
            api.deoptimize(getDisplayWidth);
            api.hook(getDisplayWidth).setExceptionMode(ExceptionMode.PROTECTIVE)
                    .intercept(new Hooker() {
                        @Override
                        public Object intercept(Chain chain) throws Throwable {
                            Object result = chain.proceed();
                            if (result instanceof Integer) {
                                int width = (Integer) result;
                                if (width == 1644) return 1440;
                                if (width == 1096) return 1080;
                            }
                            return result;
                        }
                    });
        }

        // getSummary() -> 显示当前实际分辨率 "1644×3840"
        Method getSummary = findMethod(controller, "getSummary");
        if (getSummary != null) {
            final Field displayField = findField(controller, "mDisplay");
            api.deoptimize(getSummary);
            api.hook(getSummary).setExceptionMode(ExceptionMode.PROTECTIVE)
                    .intercept(new Hooker() {
                        @Override
                        public Object intercept(Chain chain) throws Throwable {
                            Object self = chain.getThisObject();
                            if (self == null || displayField == null) return chain.proceed();
                            Object displayObj = displayField.get(self);
                            if (displayObj instanceof Display) {
                                Display.Mode mode = ((Display) displayObj).getMode();
                                return String.format(Locale.ROOT, "%d×%d",
                                        mode.getPhysicalWidth(), mode.getPhysicalHeight());
                            }
                            return chain.proceed();
                        }
                    });
        }

        Class<?> fragment = Class.forName(
                "com.android.settings.display.ScreenResolutionFragment", false, loader);
        final Method getPreferMode = findMethod(fragment, "getPreferMode", int.class);

        // getKeyForResolution(int) -> 1096/1644 归一到 1080/1440 再查 key
        Method getKeyForResolution = findMethod(fragment, "getKeyForResolution", int.class);
        if (getKeyForResolution != null) {
            api.deoptimize(getKeyForResolution);
            api.hook(getKeyForResolution).setExceptionMode(ExceptionMode.PROTECTIVE)
                    .intercept(new Hooker() {
                        @Override
                        public Object intercept(Chain chain) throws Throwable {
                            Object[] args = argsOf(chain);
                            if (args[0] instanceof Integer) {
                                int width = (Integer) args[0];
                                if (width == 1096) args[0] = 1080;
                                else if (width == 1644) args[0] = 1440;
                            }
                            return chain.proceed(args);
                        }
                    });
        }

        // setDisplayMode(int) -> 1080/1440 映射回真实 1096/1644,选择后持久化
        Method setDisplayMode = findMethod(fragment, "setDisplayMode", int.class);
        if (setDisplayMode != null) {
            api.deoptimize(setDisplayMode);
            api.hook(setDisplayMode).setExceptionMode(ExceptionMode.PROTECTIVE)
                    .intercept(new Hooker() {
                        @Override
                        public Object intercept(Chain chain) throws Throwable {
                            Object[] args = argsOf(chain);
                            if (args[0] instanceof Integer) {
                                int width = (Integer) args[0];
                                if (width == 1080) args[0] = 1096;
                                else if (width == 1440) args[0] = 1644;
                            }
                            Object result = chain.proceed(args);
                            persistPreferredResolution(chain, getPreferMode, args[0]);
                            return result;
                        }
                    });
        }

        // onAttach(Context) -> 覆写分辨率选项文案 + 隐藏示意图
        Method onAttach = findMethod(fragment, "onAttach", Context.class);
        if (onAttach != null) {
            api.deoptimize(onAttach);
            api.hook(onAttach).setExceptionMode(ExceptionMode.PROTECTIVE)
                    .intercept(new Hooker() {
                        @Override
                        public Object intercept(Chain chain) throws Throwable {
                            Object result = chain.proceed();
                            Object self = chain.getThisObject();
                            if (self != null) {
                                setField(self, "mScreenResolutionSummaries",
                                        new String[]{"1096×2560", "1644×3840"});
                                Object imgpref = getField(self, "mImagePreference");
                                if (imgpref != null) {
                                    Method setVisible = findMethod(
                                            imgpref.getClass(), "setVisible", boolean.class);
                                    if (setVisible != null) {
                                        setVisible.invoke(imgpref, false);
                                    }
                                }
                            }
                            return result;
                        }
                    });
        }
    }

    /** setDisplayMode 后把用户选择写入 Settings.Global(Sony 开机会读它恢复分辨率)。 */
    private static void persistPreferredResolution(Chain chain, Method getPreferMode,
            Object resolvedWidth) {
        try {
            Object self = chain.getThisObject();
            if (self == null || getPreferMode == null) return;
            // 缓存 getContext:Settings Fragment 继承链稳定,仅需一次查找
            Method getContext = findMethod(self.getClass(), "getContext");
            if (getContext == null) return;
            Object ctx = getContext.invoke(self);
            if (!(ctx instanceof Context)) return;
            Object modeObj = getPreferMode.invoke(self, resolvedWidth);
            if (!(modeObj instanceof Display.Mode)) return;
            Display.Mode mode = (Display.Mode) modeObj;
            ContentResolver resolver = ((Context) ctx).getContentResolver();
            Settings.Global.putInt(resolver, "user_preferred_resolution_width",
                    mode.getPhysicalWidth());
            Settings.Global.putInt(resolver, "user_preferred_resolution_height",
                    mode.getPhysicalHeight());
            Settings.Global.putFloat(resolver, "user_preferred_refresh_rate",
                    mode.getRefreshRate());
        } catch (Throwable ignored) {
            // 持久化失败不影响切换本身
        }
    }

    private static Object[] argsOf(Chain chain) {
        int n = chain.getArgs().size();
        Object[] args = new Object[n];
        for (int i = 0; i < n; i++) {
            args[i] = chain.getArgs().get(i);
        }
        return args;
    }

    private static Field findField(Class<?> clazz, String name) {
        for (Class<?> c = clazz; c != null; c = c.getSuperclass()) {
            try {
                Field f = c.getDeclaredField(name);
                f.setAccessible(true);
                return f;
            } catch (NoSuchFieldException ignored) {
            }
        }
        return null;
    }

    private static Object getField(Object obj, String name) {
        Field f = findField(obj.getClass(), name);
        if (f == null) return null;
        try {
            return f.get(obj);
        } catch (Throwable t) {
            return null;
        }
    }

    private static void setField(Object obj, String name, Object value) {
        Field f = findField(obj.getClass(), name);
        if (f == null) return;
        try {
            f.set(obj, value);
        } catch (Throwable ignored) {
        }
    }

    private static Method findMethod(Class<?> clazz, String name, Class<?>... params) {
        for (Class<?> c = clazz; c != null; c = c.getSuperclass()) {
            try {
                Method m = c.getDeclaredMethod(name, params);
                m.setAccessible(true);
                return m;
            } catch (NoSuchMethodException ignored) {
            }
        }
        return null;
    }

    private static Constructor<?> findConstructor(Class<?> clazz, Class<?>... params) {
        try {
            Constructor<?> c = clazz.getDeclaredConstructor(params);
            c.setAccessible(true);
            return c;
        } catch (NoSuchMethodException ignored) {
            return null;
        }
    }

}
