kde_enable_exceptions()

add_definitions(-DTRANSLATION_DOMAIN=\"panelspacer\")

kcoreaddons_add_plugin(org.kde.plasma.panelspacer SOURCES panelspacer.cpp panelspacer.h INSTALL_NAMESPACE "plasma/applets")

target_link_libraries(org.kde.plasma.panelspacer Qt::Gui Qt::Core Qt::Qml Qt::Quick Plasma::Plasma Plasma::PlasmaQuick KF6::I18n)
