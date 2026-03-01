[Desktop Entry]
Exec=@CMAKE_INSTALL_PREFIX@/bin/plasmashell
X-DBUS-StartupType=Unique
Name=Plasma
Type=Application
StartupNotify=false
X-DBUS-ServiceName=org.kde.plasmashell
OnlyShowIn=KDE;
X-KDE-autostart-phase=0
Icon=plasma-symbolic
NoDisplay=true
X-systemd-skip=true

X-KDE-Wayland-Interfaces=org_kde_plasma_window_management,org_kde_kwin_keystate,zkde_screencast_unstable_v1,org_kde_plasma_activation_feedback,kde_lockscreen_overlay_v1
X-KDE-DBUS-Restricted-Interfaces=org.kde.KWin.ScreenShot2
