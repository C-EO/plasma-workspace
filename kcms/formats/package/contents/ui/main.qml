/*
  SPDX-FileCopyrightLabel: 2021 Han Young <hanyoung@protonmail.com>

  SPDX-License-Identifier: LGPL-3.0-or-later
*/
import QtQuick 2.12
import QtQuick.Controls 2.12 as QQC2
import QtQuick.Layouts 1.15

import org.kde.kirigami 2.7 as Kirigami
import org.kde.kcm 1.2 as KCM

KCM.ScrollViewKCM {
    id: root
    implicitHeight: Kirigami.Units.gridUnit * 40
    implicitWidth: Kirigami.Units.gridUnit * 20
    view: ListView {
        model: kcm.optionsModel
        delegate: Kirigami.BasicListItem {
            text: name
            subtitle: localeName
            trailing: QQC2.Label {
                text: example
            }
            onClicked: {
                pushWithSetting(page)
            }
        }
    }

    function pushWithSetting(setting) {
        if (kcm.depth === 1) {
            kcm.push("LocaleListPage.qml", {"setting": setting});
        } else {
            kcm.getSubPage(0).setting = setting;
            kcm.currentIndex = 1;
        }
    }
}
