set(appmenuapplet_SRCS
    appmenumodel.cpp
    appmenuplugin.cpp
    appmenumodel.h
    appmenuplugin.h
)

add_library(appmenuplugin SHARED ${appmenuapplet_SRCS})
target_link_libraries(appmenuplugin
                      Qt::Core
                      Qt::Widgets
                      Qt::Quick
                      Plasma::Plasma
                      KF6::WindowSystem
                      KF6::I18n
                      PW::LibTaskManager
                      dbusmenuqt)

install(TARGETS appmenuplugin DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/plasma/private/appmenu)

install(FILES qmldir DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/plasma/private/appmenu)
