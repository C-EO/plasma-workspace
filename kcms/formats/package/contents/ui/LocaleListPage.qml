/*
  SPDX-FileCopyrightLabel: 2021 Han Young <hanyoung@protonmail.com>

  SPDX-License-Identifier: LGPL-3.0-or-later
*/
import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.15

import org.kde.kirigami 2.7 as Kirigami
import org.kde.kcm 1.2
import LocaleListModel 1.0
Kirigami.Page {
    property string setting: ""
    id: root
    title: ""

    onSettingChanged: {
        switch (setting) {
        case "lang":
            title = i18n("Language");
            break;
        case "numeric":
            title = i18n("Number");
            break;
        case "time":
            title = i18n("Time");
            break;
        case "currency":
            title = i18n("Currency");
            break;
        case "measurement":
            title = i18n("Measurement");
            break;
        case "collate":
            title = i18n("Collate");
            break;
        }
        localeListModel.selectedConfig = setting;
    }

    LocaleListModel {
        id: localeListModel
    }
    ColumnLayout {
        anchors.fill: parent
        TextField {
            id: searchField
            Layout.fillWidth: true
            focus: true
            placeholderText: i18n("Search...")
            onTextChanged: localeListModel.filter = text
        }
        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            ScrollBar.vertical.policy: ScrollBar.AlwaysOn
            ListView {
                model: localeListModel
                delegate: Kirigami.BasicListItem {
                    icon: flag
                    text: display
                    subtitle: localeName
                    trailing: Text {
                        color: Kirigami.Theme.disabledTextColor
                        text: example
                    }
                    onClicked: {
                        switch (setting) {
                        case "lang":
                            kcm.settings.lang = localeName;
                            break;
                        case "numeric":
                            kcm.settings.numeric = localeName;
                            break;
                        case "time":
                            kcm.settings.time = localeName;
                            break;
                        case "currency":
                            kcm.settings.monetary = localeName;
                            break;
                        case "measurement":
                            kcm.settings.measurement = localeName;
                            break;
                        case "collate":
                            kcm.settings.collate = localeName;
                            break;
                        }
                        kcm.currentIndex = 0;
                        kcm.showPassiveNotification(i18n("Your changes will take effect the next time you log in."));
                    }
                }
            }
        }
    }
}
