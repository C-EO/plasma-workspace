/*
    SPDX-FileCopyrightText: 2020 Devin Lin <espidev@gmail.com>
    SPDX-FileCopyrightText: 2024 Nicolas Fella <nicolas.fella@gmx.de>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

pragma ComponentBehavior: Bound

import QtQuick 2.12
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.5 as QQC2

import org.kde.kirigami 2.12 as Kirigami

ColumnLayout {
    id: root

    required property string currentError
    required property var model

    signal deleteFinger(string name)
    signal reenrollFinger(string name)

    spacing: Kirigami.Units.smallSpacing

    Kirigami.InlineMessage {
        id: errorMessage
        type: Kirigami.MessageType.Error
        visible: root.currentError !== ""
        text: root.currentError
        Layout.fillWidth: true
        actions: [
            Kirigami.Action {
                icon.name: "dialog-close"
                onTriggered: root.currentError = ""
            }
        ]
    }

    ListView {
        id: fingerprintsList
        model: root.model
        Layout.fillWidth: true
        Layout.fillHeight: true

        delegate: Kirigami.SwipeListItem {
            id: delegate

            required property string internalName
            required property string friendlyName

            // Don't need a background or hover effect for this use case
            hoverEnabled: false
            backgroundColor: "transparent"
            contentItem: RowLayout {
                Kirigami.Icon {
                    source: "fingerprint"
                    height: Kirigami.Units.iconSizes.medium
                    width: Kirigami.Units.iconSizes.medium
                }
                QQC2.Label {
                    Layout.fillWidth: true
                    elide: Text.ElideRight
                    text: delegate.friendlyName
                    textFormat: Text.PlainText
                }
            }
            actions: [
                Kirigami.Action {
                    icon.name: "edit-entry"
                    tooltip: i18n("Re-enroll finger")
                    onTriggered: root.reenrollFinger(delegate.internalName)
                },
                Kirigami.Action {
                    icon.name: "entry-delete"
                    tooltip: i18n("Delete fingerprint")
                    onTriggered: root.deleteFinger(delegate.internalName)
                }
            ]
        }

        Kirigami.PlaceholderMessage {
            anchors.centerIn: parent
            width: parent.width - (Kirigami.Units.largeSpacing * 4)
            visible: fingerprintsList.count == 0
            text: i18n("No fingerprints added")
            icon.name: "fingerprint"
        }
    }
}