# SPDX-License-Identifier: BSD-2-Clause
# SPDX-FileCopyrightText: 2022 Han Young <hanyoung@protonmail.com>

add_definitions(-DTRANSLATION_DOMAIN=\"kcm_regionandlang\")
################## Set sources files #################
set(kcm_regionandlang_example_SRCS
    exampleutility.cpp exampleutility.h
)
add_library(kcm_regionandlang_example_static STATIC ${kcm_regionandlang_example_SRCS})
set_property(TARGET kcm_regionandlang_example_static PROPERTY POSITION_INDEPENDENT_CODE ON)
target_link_libraries(kcm_regionandlang_example_static
    KF6::I18n
    KF6::CoreAddons
)
set(kcm_regionandlang_PART_SRCS
    settingtype.h
    kcmregionandlang.cpp kcmregionandlang.h
    localelistmodel.cpp localelistmodel.h
    optionsmodel.cpp optionsmodel.h
    languagelistmodel.cpp languagelistmodel.h
    localegenerator.cpp localegenerator.h
    localegeneratorbase.cpp localegeneratorbase.h
    localegeneratorgeneratedglibc.cpp localegeneratorgeneratedglibc.h
    regionandlangsettings.cpp regionandlangsettings.h
    binarydialectmodel.h binarydialectmodel.cpp
)
kconfig_add_kcfg_files(kcm_regionandlang_PART_SRCS regionandlangsettingsbase.kcfgc GENERATE_MOC)

if(GLIBC_LOCALE_GEN)
    set(kcm_regionandlang_PART_SRCS
        ${kcm_regionandlang_PART_SRCS}
        localegeneratorglibc.cpp localegeneratorglibc.h)
endif()
if(UBUNTU_LOCALE)
    set(kcm_regionandlang_PART_SRCS
        ${kcm_regionandlang_PART_SRCS}
        localegeneratorubuntu.cpp localegeneratorubuntu.h)
endif()
ecm_qt_declare_logging_category(
    kcm_regionandlang_PART_SRCS
    HEADER kcm_regionandlang_debug.h
    IDENTIFIER KCM_REGIONANDLANG
    CATEGORY_NAME org.kde.kcm_regionandlang
    DESCRIPTION "Region and Language KCM"
    EXPORT kcm_regionandlang
)

################ Build helper and add additional source files #############
if(REGION_LANG_GENERATE_LOCALE_HELPER)
    add_subdirectory("localegenhelper")
    qt_generate_dbus_interface(
        ${CMAKE_CURRENT_SOURCE_DIR}/localegenhelper/localegenhelper.h
        org.kde.localegenhelper.xml
        OPTIONS -s -m
    )
    qt_add_dbus_interface(kcm_regionandlang_PART_SRCS ${CMAKE_CURRENT_BINARY_DIR}/org.kde.localegenhelper.xml localegenhelperinterface)
endif()

#################### Declare target #######################
kcmutils_add_qml_kcm(kcm_regionandlang SOURCES ${kcm_regionandlang_PART_SRCS})

ecm_qt_install_logging_categories(
    EXPORT kcm_regionandlang
    DESTINATION "${KDE_INSTALL_LOGGINGCATEGORIESDIR}"
)

################# Link libraries ####################
target_link_libraries(kcm_regionandlang PRIVATE
    kcm_regionandlang_example_static
    Qt::Core
    Qt::DBus
    KF6::I18n
    KF6::KCMUtils
    KF6::KCMUtilsQuick
    KF6::ItemModels
    KF6::GuiAddons
    Qt::Quick
)
if(UBUNTU_LOCALE)
    target_link_libraries(kcm_regionandlang PRIVATE PK::packagekitqt6)
endif()

########### unit tests ##############
if(BUILD_TESTING)
    add_subdirectory(autotests)
endif()
