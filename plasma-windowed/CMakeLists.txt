add_definitions(-DTRANSLATION_DOMAIN=\"plasmawindowed\")

set(plasmawindowed-app_SRCS
    plasmawindowedcorona.cpp
    plasmawindowedview.cpp
    plasmawindowedcorona.h
    plasmawindowedview.h
    main.cpp
)

add_executable(plasmawindowed ${plasmawindowed-app_SRCS})

target_link_libraries(
    plasmawindowed
    Qt::Widgets
    Qt::Quick
    Qt::Qml
    KF6::I18n
    KF6::IconThemes
    KF6::XmlGui
    Plasma::PlasmaQuick
    Plasma::Plasma
    KF6::DBusAddons
    KF6::Notifications
    KF6::Package
    KF6::StatusNotifierItem
)

install(TARGETS plasmawindowed ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
#even if hidden, the desktop file is needed anyways for kdbusservice::unique
configure_file(org.kde.plasmawindowed.desktop.cmake ${CMAKE_CURRENT_BINARY_DIR}/org.kde.plasmawindowed.desktop @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/org.kde.plasmawindowed.desktop DESTINATION ${KDE_INSTALL_APPDIR})
