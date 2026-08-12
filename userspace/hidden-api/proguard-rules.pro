# hidden-api stub module: compileOnly only, never packaged into the APK.
# Add hidden framework classes here when the standard android.jar lacks them
# (e.g. android.view.DisplayInfo is @hide).
-keep class android.view.DisplayInfo { *; }
