# Keep Xposed module entry points
-keep class com.sony.feas.FeasModule { *; }
-keep class com.sony.feas.FeasModule$FrameHooker { *; }
-keep class com.sony.feas.PerfMgrClient { *; }
-keep class com.sony.feas.MainActivity { *; }

# libxposed API
-keep class io.github.libxposed.api.** { *; }

# Keep annotations used by framework
-keepattributes *Annotation*
-dontwarn io.github.libxposed.**
