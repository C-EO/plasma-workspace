/*
  SPDX-FileCopyrightLabel: 2021 Han Young <hanyoung@protonmail.com>

  SPDX-License-Identifier: LGPL-3.0-or-later
*/
import QtQuick 2.12
import QtQuick.Controls 2.12 as QQC2
import QtQuick.Layouts 1.15

import org.kde.kirigami 2.8 as Kirigami
import org.kde.kcm 1.2
import LocaleListModel 1.0

Kirigami.Page {
    property string setting: ""
    id: root
    title: ""

    onSettingChanged: {
        localeListModel.selectedConfig = setting;
        switch (setting) {
        case "lang":
            return i18n("Language");
        case "numeric":
            return i18n("Number");
        case "time":
            return i18n("Time");
        case "currency":
            return i18n("Currency");
        case "measurement":
            return i18n("Measurement");
        case "collate":
            return i18n("Collate");
        }
    }

    LocaleListModel {
        id: localeListModel
    }
    ColumnLayout {
        anchors.fill: parent
        Kirigami.SearchField {
            id: searchField
            Layout.fillWidth: true
            onTextChanged: localeListModel.filter = text
        }
        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            ListView {
                model: localeListModel
                delegate: Kirigami.BasicListItem {
                    icon: flag
                    text: display
                    subtitle: localeName
                    trailing: QQC2.Label {
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
