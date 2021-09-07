add_subdirectory(plasmaautostart)
add_subdirectory(kcminit)
add_subdirectory(waitforname)

if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    add_subdirectory(systemd)
endif()

if(X11_Xcursor_FOUND)
    set(HAVE_XCURSOR TRUE)
endif()

if(${Breeze_FOUND})
    if(${BREEZE_WITH_KDECORATION})
        set(HAVE_BREEZE_DECO TRUE)
    endif()
endif()

configure_file (${CMAKE_SOURCE_DIR}/kcms/lookandfeel/config-kcm.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-kcm.h )

add_definitions(-DQT_NO_CAST_FROM_ASCII -DQT_NO_CAST_TO_ASCII)
add_definitions(-DQT_NO_NARROWING_CONVERSIONS_IN_CONNECT)

qt_add_dbus_interface(
    startplasma_SRCS
    ${CMAKE_SOURCE_DIR}/ksplash/ksplashqml/org.kde.KSplash.xml
    ksplashinterface
)


set(START_PLASMA_COMMON_SRCS startplasma.cpp)

ecm_qt_declare_logging_category(START_PLASMA_COMMON_SRCS HEADER debug.h IDENTIFIER PLASMA_STARTUP CATEGORY_NAME org.kde.startup)
kcmutils_generate_module_data(
    lnf_SRCS
    MODULE_DATA_HEADER lookandfeeldata.h
    MODULE_DATA_CLASS_NAME LookAndFeelData
    SETTINGS_HEADERS lookandfeelsettings.h
    SETTINGS_CLASSES LookAndFeelSettings
)
kconfig_add_kcfg_files(lnf_SRCS ${CMAKE_SOURCE_DIR}/kcms/lookandfeel/lookandfeelsettings.kcfgc GENERATE_MOC)

add_executable(startplasma-x11 ${START_PLASMA_COMMON_SRCS} startplasma-x11.cpp kcheckrunning/kcheckrunning.cpp ${startplasma_SRCS})
add_executable(startplasma-wayland ${START_PLASMA_COMMON_SRCS}
                                   ${startplasma_SRCS}
                                   ${lnf_SRCS}
                                   startplasma-wayland.cpp
                                   ${CMAKE_SOURCE_DIR}/kcms/lookandfeel/lookandfeelmanager.cpp
                                   ${CMAKE_SOURCE_DIR}/kcms/colors/colorsapplicator.cpp
)
add_executable(startplasma-waylandsession ${START_PLASMA_COMMON_SRCS} startplasma-waylandsession.cpp ${startplasma_SRCS})

target_link_libraries(startplasma-x11 PRIVATE Qt::Core Qt::DBus KF5::ConfigCore PW::KWorkspace
    X11::X11 # for kcheckrunning
)

target_link_libraries(startplasma-wayland PRIVATE Qt::Gui Qt::DBus KF5::Service KF5::KCMUtils KF5::Package KF5::GuiAddons PW::KWorkspace PlasmaAutostart)
target_link_libraries(startplasma-waylandsession PRIVATE Qt::Core Qt::DBus KF5::ConfigCore PW::KWorkspace)
target_compile_definitions(startplasma-wayland PRIVATE -DDISABLE_NOTIFY)
add_subdirectory(plasma-session)
add_subdirectory(plasma-shutdown)

#FIXME: reconsider, looks fishy
if(NOT CMAKE_INSTALL_PREFIX STREQUAL "/usr")
    set_property(SOURCE startplasma.cpp APPEND PROPERTY COMPILE_DEFINITIONS
        XCURSOR_PATH="${KDE_INSTALL_FULL_DATAROOTDIR}/icons:$XCURSOR_PATH:~/.icons:/usr/share/icons:/usr/share/pixmaps:/usr/X11R6/lib/X11/icons")
endif()

configure_file(config-startplasma.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-startplasma.h)

install(TARGETS startplasma-x11 ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
install(TARGETS startplasma-wayland ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
install(TARGETS startplasma-waylandsession DESTINATION ${KDE_INSTALL_LIBEXECDIR})
install(PROGRAMS plasma-sourceenv.sh DESTINATION ${KDE_INSTALL_LIBEXECDIR})
install(PROGRAMS plasma-dbus-run-session-if-needed DESTINATION ${KDE_INSTALL_LIBEXECDIR})
