add_subdirectory(krdb)

add_subdirectory(desktoptheme)
add_subdirectory(icons)

if(KF6UserFeedback_FOUND)
    add_subdirectory(feedback)
endif()

add_subdirectory(style)
add_subdirectory(lookandfeel)
add_subdirectory(colors)
if(X11_Xcursor_FOUND)
    add_subdirectory(cursortheme)
endif()

if(FONTCONFIG_FOUND)
    add_subdirectory( kfontinst )
    add_subdirectory( fonts )
endif()

add_subdirectory(autostart)
add_subdirectory(region_language)
add_subdirectory(notifications)
add_subdirectory(nightlight)
add_subdirectory(users)
add_subdirectory(soundtheme)
add_subdirectory(wallpaper)
add_subdirectory(componentchooser)
