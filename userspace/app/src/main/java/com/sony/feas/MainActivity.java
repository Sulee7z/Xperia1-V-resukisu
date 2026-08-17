package com.sony.feas;

import android.app.Activity;
import android.util.Log;
import android.app.AlertDialog;
import android.content.res.Configuration;
import android.graphics.Color;
import android.graphics.Typeface;
import android.graphics.drawable.ColorDrawable;
import android.graphics.drawable.GradientDrawable;
import android.os.Build;
import android.os.Bundle;
import android.view.View;
import android.view.Window;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.TextView;
import android.widget.Toast;

import java.io.BufferedReader;
import java.io.InputStreamReader;

/**
 * FEAS 管理界面 - Material 3 风格(纯原生,零依赖)。
 * 深/亮色自动跟随系统。完整功能:开关、手动帧率、统计、root 检查。
 */
public class MainActivity extends Activity {

    private int PRIMARY = 0xFF6750A4;
    private int ON_PRIMARY = 0xFFFFFFFF;
    private int SURFACE = 0xFFFEF7FF;
    private int SURFACE_VARIANT = 0xFFE7E0EC;
    private int ON_SURFACE = 0xFF1D1B20;
    private int OUTLINE = 0xFF79747E;

    private TextView statusText;
    private LinearLayout btnRow;
    private LinearLayout root;
    private Button toggleBtn;
    private Button dfpsBtn;
    private Button hmdBtn;
    private int currentFps;
    private boolean currentEnabled;
    private boolean dfpsEnabled;
    private boolean hmdEnabled;
    private boolean isDark;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        PerfMgrSysfs.init(this);
        applyTheme();

        // 隐藏系统 ActionBar(顶部 FEAS 栏)
        if (getActionBar() != null)
            getActionBar().hide();

        Window win = getWindow();
        // 彻底去除系统遮罩:透明 window 背景,自己画 SURFACE
        win.setBackgroundDrawable(new ColorDrawable(Color.TRANSPARENT));
        win.setStatusBarColor(Color.TRANSPARENT);
        win.setNavigationBarColor(Color.TRANSPARENT);

        root = new LinearLayout(this);
        root.setOrientation(LinearLayout.VERTICAL);
        root.setPadding(dp(24), dp(32), dp(24), dp(24));
        root.setBackgroundColor(SURFACE);

        TextView title = new TextView(this);
        title.setText("FEAS 帧率感知调度");
        title.setTextSize(28);
        title.setTypeface(Typeface.DEFAULT_BOLD);
        title.setTextColor(ON_SURFACE);
        root.addView(title);

        TextView subtitle = new TextView(this);
        subtitle.setText("根据渲染帧率动态调节 CPU 频率 · 静止零开销");
        subtitle.setTextSize(14);
        subtitle.setTextColor(OUTLINE);
        LinearLayout.LayoutParams sp = new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                LinearLayout.LayoutParams.WRAP_CONTENT);
        sp.bottomMargin = dp(24);
        subtitle.setLayoutParams(sp);
        root.addView(subtitle);

        statusText = new TextView(this);
        statusText.setTextSize(15);
        statusText.setTextColor(ON_SURFACE);
        statusText.setPadding(dp(20), dp(20), dp(20), dp(20));
        statusText.setBackground(rounded(28, SURFACE_VARIANT));
        LinearLayout.LayoutParams st = new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                LinearLayout.LayoutParams.WRAP_CONTENT);
        st.bottomMargin = dp(24);
        statusText.setLayoutParams(st);
        root.addView(statusText);

        toggleBtn = new Button(this);
        toggleBtn.setAllCaps(false);
        toggleBtn.setTextSize(14);
        LinearLayout.LayoutParams tl = new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                LinearLayout.LayoutParams.WRAP_CONTENT);
        tl.bottomMargin = dp(16);
        toggleBtn.setLayoutParams(tl);
        toggleBtn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                currentEnabled = !currentEnabled;
                PerfMgrSysfs.setEnabled(currentEnabled);
                refreshStatus();
            }
        });
        root.addView(toggleBtn);

        // dfps 开关:动态刷新率(触摸→120Hz,静止→60Hz)
        dfpsBtn = new Button(this);
        dfpsBtn.setAllCaps(false);
        dfpsBtn.setTextSize(14);
        LinearLayout.LayoutParams dl = new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                LinearLayout.LayoutParams.WRAP_CONTENT);
        dl.bottomMargin = dp(16);
        dfpsBtn.setLayoutParams(dl);
        dfpsBtn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                dfpsEnabled = !dfpsEnabled;
                DfpsController.setEnabled(dfpsEnabled);
                refreshStatus();
                Toast.makeText(MainActivity.this,
                        dfpsEnabled ? "dfps: 触摸→120Hz,静止→60Hz" : "dfps: 已关闭",
                        Toast.LENGTH_SHORT).show();
            }
        });
        root.addView(dfpsBtn);

        // 240Hz MBR(BFI):黑帧插入,120Hz 触摸时等效 240Hz 无拖影(需 dfps 开启时配合)
        hmdBtn = new Button(this);
        hmdBtn.setAllCaps(false);
        hmdBtn.setTextSize(14);
        LinearLayout.LayoutParams hl = new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                LinearLayout.LayoutParams.WRAP_CONTENT);
        hl.bottomMargin = dp(16);
        hmdBtn.setLayoutParams(hl);
        hmdBtn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                hmdEnabled = !hmdEnabled;
                FeasBinderClient.setHmd(hmdEnabled);
                refreshStatus();
                Toast.makeText(MainActivity.this,
                        hmdEnabled ? "240Hz MBR: 触摸时插黑,运动无拖影(亮度略降)"
                                : "240Hz MBR: 已关闭",
                        Toast.LENGTH_SHORT).show();
            }
        });
        root.addView(hmdBtn);

        TextView label = new TextView(this);
        label.setText("目标帧率");
        label.setTextSize(14);
        label.setTextColor(OUTLINE);
        LinearLayout.LayoutParams lp = new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                LinearLayout.LayoutParams.WRAP_CONTENT);
        lp.bottomMargin = dp(8);
        label.setLayoutParams(lp);
        root.addView(label);

        btnRow = new LinearLayout(this);
        btnRow.setOrientation(LinearLayout.HORIZONTAL);
        root.addView(btnRow);

        setContentView(root);
        refreshStatus();
        checkRoot();
    }

    private void applyTheme() {
        int nightMode = getResources().getConfiguration().uiMode
                & Configuration.UI_MODE_NIGHT_MASK;
        isDark = nightMode == Configuration.UI_MODE_NIGHT_YES;
        if (isDark) {
            PRIMARY = 0xFFD0BCFF;
            ON_PRIMARY = 0xFF381E72;
            SURFACE = 0xFF1C1B1F;
            SURFACE_VARIANT = 0xFF49454F;
            ON_SURFACE = 0xFFE6E1E5;
            OUTLINE = 0xFF938F99;
        } else {
            PRIMARY = 0xFF6750A4;
            ON_PRIMARY = 0xFFFFFFFF;
            SURFACE = 0xFFFEF7FF;
            SURFACE_VARIANT = 0xFFE7E0EC;
            ON_SURFACE = 0xFF1D1B20;
            OUTLINE = 0xFF79747E;
        }
        // 状态栏图标:深色模式用浅色图标,浅色模式用深色图标
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            View decor = getWindow().getDecorView();
            int flags = decor.getSystemUiVisibility();
            if (isDark) {
                flags |= View.SYSTEM_UI_FLAG_LIGHT_STATUS_BAR;
            } else {
                flags &= ~View.SYSTEM_UI_FLAG_LIGHT_STATUS_BAR;
            }
            decor.setSystemUiVisibility(flags);
        }
    }

    @Override
    public void onConfigurationChanged(Configuration newConfig) {
        super.onConfigurationChanged(newConfig);
        applyTheme();
        recreate();
    }

    private void checkRoot() {
        new Thread(new Runnable() {
            @Override
            public void run() {
                boolean hasRoot = false;
                try {
                    Process p = new ProcessBuilder("su", "-c", "id")
                            .redirectErrorStream(true).start();
                    BufferedReader r = new BufferedReader(
                            new InputStreamReader(p.getInputStream()));
                    String line = r.readLine();
                    r.close();
                    p.waitFor();
                    hasRoot = line != null && line.contains("uid=0");
                } catch (Throwable ignored) {
                }
                final boolean root = hasRoot;
                runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        if (!root) {
                            new AlertDialog.Builder(MainActivity.this)
                                    .setTitle("需要 Root 权限")
                                    .setMessage("FEAS 需要 Root 权限才能写入内核调频参数。\n请在 KernelSU 管理器中授予 com.sony.feas Root 权限后重试。")
                                    .setPositiveButton("知道了", null)
                                    .show();
                        } else {
                            ensureVectorModuleEnabled();
                        }
                    }
                });
            }
        }).start();
    }

    /**
     * 自修复:如果 Vector 未启用 FEAS 模块(重装 APK 后常见),
     * 用 Vector CLI(root)自动启用并添加 systemui scope。
     * 完成后提示重启 SystemUI 生效。
     */
    private void ensureVectorModuleEnabled() {
        new Thread(new Runnable() {
            @Override
            public void run() {
                try {
                    // 检查模块状态
                    Process p = new ProcessBuilder("su", "-c",
                            "/data/adb/lspd/cli modules ls 2>/dev/null")
                            .redirectErrorStream(true).start();
                    BufferedReader r = new BufferedReader(
                            new InputStreamReader(p.getInputStream()));
                    StringBuilder sb = new StringBuilder();
                    String line;
                    while ((line = r.readLine()) != null) sb.append(line).append("\n");
                    r.close();
                    p.waitFor();
                    String out = sb.toString();

                    // 精确判断 FEAS 自己的状态行,而非整个列表
                    // (其他 disabled 模块会让全文 contains("disabled") 误判)
                    boolean feasDisabled = false;
                    boolean feasEnabled = false;
                    for (String statusLine : out.split("\n")) {
                        if (!statusLine.contains("com.sony.feas")) continue;
                        if (statusLine.contains("disabled")) feasDisabled = true;
                        if (statusLine.contains("enabled")) feasEnabled = true;
                        break;
                    }

                    if (feasDisabled) {
                        // 确实未启用:自动启用
                        execSu("/data/adb/lspd/cli modules enable com.sony.feas");
                        execSu("/data/adb/lspd/cli scope add com.sony.feas com.android.systemui/0");
                        runOnUiThread(new Runnable() {
                            @Override
                            public void run() {
                                new AlertDialog.Builder(MainActivity.this)
                                        .setTitle("FEAS 模块已自动启用")
                                        .setMessage("检测到 FEAS Xposed 模块未启用,已自动通过 Vector CLI 启用。\n\n请重启 SystemUI(或重启手机)后,帧上报统计才会生效。")
                                        .setPositiveButton("知道了", null)
                                        .show();
                            }
                        });
                    } else if (feasEnabled) {
                        // 已启用:无需操作(不再弹窗)
                        android.util.Log.i("FEAS", "模块已启用,无需自修复");
                    }
                } catch (Throwable ignored) {
                }
            }
        }).start();
    }

    private void execSu(String cmd) {
        try {
            Process p = new ProcessBuilder("su", "-c", cmd)
                    .redirectErrorStream(true).start();
            BufferedReader r = new BufferedReader(
                    new InputStreamReader(p.getInputStream()));
            while (r.readLine() != null) { /* drain */ }
            r.close();
            p.waitFor();
        } catch (Throwable ignored) {
        }
    }

    private void addFpsButton(String label, int fps) {
        Button b = new Button(this);
        b.setText(label);
        b.setAllCaps(false);
        b.setTextSize(14);
        LinearLayout.LayoutParams lp = new LinearLayout.LayoutParams(0,
                LinearLayout.LayoutParams.WRAP_CONTENT, 1);
        lp.leftMargin = dp(4);
        lp.rightMargin = dp(4);
        b.setLayoutParams(lp);

        boolean selected = (fps == currentFps) ||
                (fps == 0 && currentFps == 0);
        b.setBackground(rounded(dp(20), selected ? PRIMARY : SURFACE_VARIANT));
        b.setTextColor(selected ? ON_PRIMARY : ON_SURFACE);
        b.setTypeface(Typeface.DEFAULT_BOLD, selected ? Typeface.BOLD : Typeface.NORMAL);

        final int target = fps;
        b.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                PerfMgrSysfs.setManualFps(target);
                currentFps = target;
                refreshStatus();
                Toast.makeText(MainActivity.this,
                        target > 0 ? "目标帧率: " + target : "目标帧率: 自动",
                        Toast.LENGTH_SHORT).show();
            }
        });
        btnRow.addView(b);
    }

    @Override
    protected void onResume() {
        super.onResume();
        loadState();
        refreshStatus();
    }

    private void loadState() {
        // Binder 一次取回全部状态,不再多次 su cat(零进程 spawn)
        int[] st = FeasBinderClient.getState();
        if (st != null) {
            currentEnabled = st[0] == 1;
            dfpsEnabled = st[1] == 1;
            hmdEnabled = st[2] == 1;
            currentFps = st[3];
        } else {
            // daemon 不可用:仅用本地 prefs 默认(不再 su 读文件,零兜底)
            currentFps = PerfMgrSysfs.getManualFps();
            currentEnabled = PerfMgrSysfs.isEnabled();
            dfpsEnabled = DfpsController.isEnabled();
            hmdEnabled = false;  // daemon 不可用,默认关(无 su 兜底)
        }
    }

    /**
     * 从 Binder 读统计(GET_STATE 附带):st[7]=frameTotal, st[8]=reports, st[9]=fails。
     * 替代旧的内核 diag su cat(统计走 Binder 通道,零进程 spawn,无 SELinux 问题)。
     */
    private long[] readStats() {
        long[] stats = {0, 0, 0};
        int[] st = FeasBinderClient.getState();
        if (st != null && st.length >= 10) {
            stats[0] = st[7];  /* 帧数(模块累计) */
            stats[1] = st[8];  /* 成功(binder 送达批数) */
            stats[2] = st[9];  /* 失败(binder 帧上报失败) */
        }
        return stats;
    }

    private void refreshStatus() {
        btnRow.removeAllViews();
        addFpsButton("自动", 0);
        addFpsButton("60", 60);
        addFpsButton("90", 90);
        addFpsButton("120", 120);

        toggleBtn.setText("模块开关: " + (currentEnabled ? "开" : "关"));
        toggleBtn.setBackground(rounded(dp(20), currentEnabled ? PRIMARY : SURFACE_VARIANT));
        toggleBtn.setTextColor(currentEnabled ? ON_PRIMARY : ON_SURFACE);

        dfpsBtn.setText("动态刷新率(dfps): " + (dfpsEnabled ? "开" : "关"));
        dfpsBtn.setBackground(rounded(dp(20), dfpsEnabled ? PRIMARY : SURFACE_VARIANT));
        dfpsBtn.setTextColor(dfpsEnabled ? ON_PRIMARY : ON_SURFACE);

        hmdBtn.setText("240Hz MBR(插黑): " + (hmdEnabled ? "开" : "关"));
        hmdBtn.setBackground(rounded(dp(20), hmdEnabled ? PRIMARY : SURFACE_VARIANT));
        hmdBtn.setTextColor(hmdEnabled ? ON_PRIMARY : ON_SURFACE);

        int kernel = PerfMgrSysfs.readTargetFps();
        StringBuilder sb = new StringBuilder();
        sb.append("模块状态: ").append(currentEnabled ? "已启用" : "已停用");
        sb.append("\n");

        sb.append("内核驱动: ");
        sb.append(PerfMgrSysfs.isDriverPresent() ? "已连接" : "未检测到驱动");
        sb.append("\n");

        sb.append("目标帧率: ");
        if (currentFps > 0) {
            sb.append(currentFps).append(" fps (手动)");
        } else {
            sb.append(kernel > 0 ? kernel : "自动").append(" fps (自动)");
        }
        sb.append("\n\n");

        sb.append("上报统计:\n");
        long[] st = readStats();
        sb.append("  帧数: ").append(st[0]).append("\n");
        sb.append("  成功: ").append(st[1]).append("\n");
        sb.append("  失败: ").append(st[2]).append("\n");

        sb.append("\n静止时零开销\n滑动/游戏时按需调频");
        statusText.setText(sb.toString());
    }

    private GradientDrawable rounded(int radius, int color) {
        GradientDrawable g = new GradientDrawable();
        g.setCornerRadius(radius);
        g.setColor(color);
        return g;
    }

    private int dp(int v) {
        return Math.round(getResources().getDisplayMetrics().density * v);
    }
}
