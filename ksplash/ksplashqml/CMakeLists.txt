set(ksplashqml_SRCS
    main.cpp
    splashapp.cpp splashapp.h
    splashwindow.cpp splashwindow.h
   )

add_executable(ksplashqml ${ksplashqml_SRCS})

target_link_libraries(ksplashqml
    Qt::Quick
    Qt::DBus
    KF6::ConfigCore
    KF6::Package
    KF6::WindowSystem
    Plasma::PlasmaQuick
    PW::KWorkspace
    LayerShellQt::Interface
   )

ecm_qt_declare_logging_category(ksplashqml
   HEADER debug.h
   IDENTIFIER KSPLASHQML_DEBUG
   CATEGORY_NAME org.kde.plasma.ksplashqml
   DESCRIPTION "ksplashqml debug"
)

install(TARGETS ksplashqml ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES org.kde.KSplash.xml DESTINATION ${KDE_INSTALL_DBUSINTERFACEDIR})

ecm_install_configured_files(INPUT plasma-ksplash.service.in @ONLY DESTINATION  ${KDE_INSTALL_SYSTEMDUSERUNITDIR})
