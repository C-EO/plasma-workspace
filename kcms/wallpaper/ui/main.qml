/*
    SPDX-FileCopyrightText: 2013 Marco Martin <mart@kde.org>
    SPDX-FileCopyrightText: 2014 Kai Uwe Broulik <kde@privat.broulik.de>
    SPDX-FileCopyrightText: 2019 David Redondo <kde@david-redondo.de>
    SPDX-FileCopyrightText: 2023 Méven Car <meven@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import QtQml

import org.kde.newstuff as NewStuff
import org.kde.kirigami as Kirigami

import org.kde.kcmutils as KCM

import org.kde.plasma.kcm.wallpaper
import org.kde.plasma.configuration 2.0

// Not using AbstractKCM because we're not using any of it features, not even one
Kirigami.ScrollablePage {
    id: appearanceRoot

    title: i18nc("@title:window", "Wallpaper")

    signal configurationChanged

    property alias parentLayout: parentLayout

    implicitWidth: Kirigami.Units.gridUnit * 15
    implicitHeight: Kirigami.Units.gridUnit * 30

    padding: 0

    actions: [
        Kirigami.Action {
            id: allScreensAction
            text: i18nc("@option:check Set the wallpaper for all screens","Set for all screens")
            visible: kcm.screens.length > 1
            checkable: true
            checked: kcm.allScreens
            onTriggered: kcm.allScreens = checked
            displayComponent: QQC2.Switch {
                text: allScreensAction.text
                checked: allScreensAction.checked
                visible: allScreensAction.visible
                onToggled: allScreensAction.trigger()
            }
        }
    ]

    function onConfigurationChanged() {
        kcm.configuration.keys().forEach(key => {
            const cfgKey = "cfg_" + key;
            if (main.currentItem[cfgKey] !== undefined) {
                kcm.configuration[key] = main.currentItem[cfgKey]
            }
        })
    }

    ColumnLayout {
        height: Math.max(implicitHeight, appearanceRoot.availableHeight)
        width: appearanceRoot.availableWidth

        spacing: 0

        ScreenView {
            visible: !kcm.allScreens && kcm.screens.length > 1

            Layout.fillWidth: true
            Layout.bottomMargin: Kirigami.Units.smallSpacing
            implicitHeight: Kirigami.Units.gridUnit * 10

            outputs: kcm.screens
            selectedScreen: kcm.selectedScreen

            onScreenSelected: (screenName) => { kcm.setSelectedScreen(screenName) }
        }

        Kirigami.FormLayout {
            id: parentLayout // needed for twinFormLayouts to work in wallpaper plugins
            Layout.fillWidth: true

            RowLayout {
                Layout.fillWidth: true
                Kirigami.FormData.label: i18nd("plasma_shell_org.kde.plasma.desktop", "Wallpaper type:")
                Kirigami.FormData.buddyFor: wallpaperComboBox

                QQC2.ComboBox {
                    id: wallpaperComboBox
                    model: kcm.wallpaperConfigModel
                    textRole: "name"
                    onActivated: {
                        var pluginName = kcm.wallpaperConfigModel.data(kcm.wallpaperConfigModel.index(currentIndex, 0), ConfigModel.PluginNameRole)
                        if (appearanceRoot.currentWallpaper === pluginName) {
                            return;
                        }
                        kcm.currentWallpaper = pluginName
                    }

                    KCM.SettingHighlighter {
                        highlight: kcm.currentWallpaper !== "org.kde.image"
                    }
                }
                NewStuff.Button {
                    configFile: "wallpaperplugin.knsrc"
                    text: i18nd("plasma_shell_org.kde.plasma.desktop", "Get New Plugins…")
                    visibleWhenDisabled: true // don't hide on disabled
                    Layout.preferredHeight: wallpaperComboBox.height
                }
            }
        }

        Item {
            id: emptyConfig
        }

        QQC2.StackView {
            id: main
            implicitHeight: main.empty ? 0 : (currentItem?.implicitHeight ?? 0)

            Layout.fillHeight: true;
            Layout.fillWidth: true;

            Connections {
                target: kcm
                function onCurrentWallpaperChanged () { main.loadSourceFile() }
                function onSelectedScreenChanged () { main.onScreenChanged() }

                function onConfigurationChanged() { main.onWallpaperConfigurationChanged() }
            }

            Connections {
                enabled: main.currentItem?.hasOwnProperty("saveConfig") ?? false
                target: kcm
                function onSettingsSaved() { main.currentItem.saveConfig(); }
            }

            function onWallpaperConfigurationChanged() {
                let wallpaperConfig = kcm.configuration
                wallpaperConfig.keys().forEach(key => {
                    const cfgKey = "cfg_" + key;
                    if (cfgKey in main.currentItem) {

                        var changedSignal = main.currentItem[cfgKey + "Changed"]
                        if (changedSignal) {
                            changedSignal.disconnect(appearanceRoot.onConfigurationChanged);
                        }
                        main.currentItem[cfgKey] = wallpaperConfig[key];

                        changedSignal = main.currentItem[cfgKey + "Changed"]
                        if (changedSignal) {
                            changedSignal.connect(appearanceRoot.onConfigurationChanged)
                        }
                    }
                })
            }

            function onScreenChanged() {
                if (!main.currentItem) {
                    main.loadSourceFile();
                    return ;
                }
                main.currentItem.screen = kcm.selectedScreen;
            }

            function loadSourceFile() {
                for (var i = 0; i < kcm.wallpaperConfigModel.count; ++i) {
                    var pluginName = kcm.wallpaperConfigModel.data(kcm.wallpaperConfigModel.index(i, 0), ConfigModel.PluginNameRole)
                    if (kcm.currentWallpaper === pluginName) {
                        wallpaperComboBox.currentIndex = i;
                        break;
                    }
                }

                const wallpaperConfig = kcm.configuration;
                const wallpaperPluginSource = kcm.wallpaperPluginSource
                // BUG 407619: wallpaperConfig can be null before calling `ContainmentItem::loadWallpaper()`
                if (wallpaperConfig && wallpaperPluginSource) {
                    var props = {
                        "configDialog": kcm,
                        "wallpaperConfiguration": wallpaperConfig
                    };

                    var newItem = replace(Qt.resolvedUrl(wallpaperPluginSource), props)

                    if ("screen" in newItem) {
                        newItem.screen = kcm.selectedScreen
                    }

                    wallpaperConfig.keys().forEach(key => {
                        const cfgKey = "cfg_" + key;
                        if (cfgKey in newItem) {
                            newItem[cfgKey] = wallpaperConfig[key];
                            let changedSignal = main.currentItem[cfgKey + "Changed"]
                            if (changedSignal) {
                                changedSignal.connect(appearanceRoot.onConfigurationChanged)
                            }
                        }
                    });

                    const configurationChangedSignal = newItem.configurationChanged
                    if (configurationChangedSignal) {
                        configurationChangedSignal.connect(appearanceRoot.onConfigurationChanged)
                    }
                } else {
                    replace(emptyConfig)
                }
            }
        }
    }

}
