set(ion_dwd_SRCS ion_dwd.cpp ion_dwd.h)
ecm_qt_declare_logging_category(ion_dwd_SRCS
    HEADER ion_dwddebug.h
    IDENTIFIER IONENGINE_dwd
    CATEGORY_NAME kde.dataengine.ion.dwd
    DEFAULT_SEVERITY Info
    DESCRIPTION "DataEngine ion dwd"
    EXPORT PLASMAWORKSPACE
)
add_library(plasma_engine_dwd MODULE ${ion_dwd_SRCS})
target_link_libraries(plasma_engine_dwd
    weather_ion
    KF6::KIOCore
    KF6::UnitConversion
    KF6::I18n
)

install(TARGETS plasma_engine_dwd DESTINATION ${KDE_INSTALL_PLUGINDIR}/plasma5support/dataengine)

