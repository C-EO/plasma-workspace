/*
    SPDX-FileCopyrightText: 2015 Martin Gräßlin <mgraesslin@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Templates as T
import QtQuick.Layouts 1.1
import org.kde.plasma.components 3.0 as PlasmaComponents3
import org.kde.plasma.plasmoid 2.0
import org.kde.plasma.extras 2.0 as PlasmaExtras
import org.kde.kirigami 2.20 as Kirigami
import org.kde.kquickcontrolsaddons as KQuickControlsAddons

import org.kde.prison 1.0 as Prison

Item {
    id: barcodeView

    required property bool expanded
    required property T.StackView stack
    required property string barcodeType

    readonly property bool valid: barcodeItem.implicitWidth > 0 && barcodeItem.implicitHeight > 0
    readonly property bool fit: barcodeItem.implicitWidth <= barcodeItem.width && barcodeItem.implicitHeight <= barcodeItem.height
    property alias text: barcodeItem.content

    readonly property var barcodeMap: [
        {text: i18nd("klipper", "QR Code"), type: Prison.Barcode.QRCode, code: "QRCode"},
        {text: i18nd("klipper", "Data Matrix"), type: Prison.Barcode.DataMatrix, code: "DataMatrix"},
        {text: i18ndc("klipper", "Aztec barcode", "Aztec"), type: Prison.Barcode.Aztec, code: "Aztec"},
        {text: i18nd("klipper", "Code 39"), type: Prison.Barcode.Code39, code: "Code39"},
        {text: i18nd("klipper", "Code 93"), type: Prison.Barcode.Code93, code: "Code93"},
        {text: i18nd("klipper", "Code 128"), type: Prison.Barcode.Code128, code: "Code128"}
    ]

    Keys.onPressed: event => {
        if (event.key == Qt.Key_Escape) {
            barcodeView.stack.popCurrentItem();
            event.accepted = true;
        }
    }

    property PlasmaExtras.PlasmoidHeading header: PlasmaExtras.PlasmoidHeading {
        RowLayout {
            anchors.fill: parent
            PlasmaComponents3.Button {
                Layout.fillWidth: true
                icon.name: "go-previous-view"
                text: i18nd("klipper", "Return to Clipboard")
                onClicked: barcodeView.stack.popCurrentItem()
            }

            PlasmaComponents3.Menu {
                id: menu
            }

            Instantiator {
                id: menuItemInstantiator
                active: barcodeView.expanded && menu.opened
                asynchronous: true
                delegate: PlasmaComponents3.MenuItem {
                    required property var modelData
                    text: modelData.text
                    checkable: true
                    autoExclusive: true
                    checked: barcodeItem.barcodeType === modelData.type

                    onClicked: {
                        barcodeView.barcodeType = modelData.code;
                        Plasmoid.configuration.barcodeType = modelData.code;
                    }
                }
                model: barcodeView.barcodeMap

                onObjectAdded: (index, object) => menu.insertItem(index, object as PlasmaComponents3.MenuItem)
            }

            PlasmaComponents3.ToolButton {
                id: copyQRButton

                action: Kirigami.Action {
                    enabled: barcodeView.valid && barcodeView.fit
                    icon.name: "edit-copy"
                    shortcut: StandardKey.Copy
                    text: i18ndc("klipper", "@action:button Copy to clipboard", "Copy")

                    onTriggered: {
                        barcodeItem.grabToImage((result) => {
                            clipboard.content = result.image;
                            feedback.show(
                                i18ndc("klipper", "@info:status", "An image of the QR code has been copied to clipboard"), 5000)
                        });
                    }
                }

                PlasmaComponents3.ToolTip {
                    text: i18ndc("klipper", "@info:tooltip", "Copy QR code image to clipboard")
                }
            }

            PlasmaComponents3.ToolButton {
                id: configureButton
                checkable: true
                checked: menu.opened
                icon.name: "configure"

                display: PlasmaComponents3.AbstractButton.IconOnly
                text: i18nd("klipper", "Change the QR code type")

                onClicked: menu.opened ? menu.close() : menu.popup()

                PlasmaComponents3.ToolTip {
                    text: configureButton.text
                }
            }
        }
    }

    KQuickControlsAddons.Clipboard {
        id: clipboard
    }

    Rectangle {
        readonly property bool square: barcodeItem.dimensions == Prison.Barcode.TwoDimensions
        readonly property int shortestSize: Math.floor(Math.min(barcodeView.width, barcodeView.height))

        anchors.centerIn: parent
        width: (square ? shortestSize : barcodeView.width) - 2 * Kirigami.Units.largeSpacing
        height: (square ? shortestSize : barcodeView.height) - 2 * Kirigami.Units.largeSpacing

        color: barcodeItem.backgroundColor
        radius: Kirigami.Units.cornerRadius
        // Cannot set visible to false as we need it to re-render when changing its size
        opacity: barcodeView.valid && barcodeView.fit ? 1 : 0

        Prison.Barcode {
            id: barcodeItem

            anchors.fill: parent
            anchors.margins: Kirigami.Units.cornerRadius

            barcodeType: barcodeView.barcodeMap.find(data => data.code === barcodeView.barcodeType)?.type ?? barcodeView.barcodeMap[0].type

            Accessible.name: barcodeView.barcodeMap.find(data => data.type === barcodeItem.barcodeType)?.text ?? barcodeView.barcodeMap[0].text
            Accessible.role: Accessible.Graphic
            Drag.dragType: Drag.Automatic
            Drag.supportedActions: Qt.CopyAction

            HoverHandler {
                enabled: barcodeView.valid && barcodeView.fit
                cursorShape: Qt.OpenHandCursor
            }

            DragHandler {
                id: dragHandler
                enabled: barcodeView.valid && barcodeView.fit

                onActiveChanged: {
                    if (active) {
                        barcodeItem.grabToImage((result) => {
                            barcodeItem.Drag.mimeData = {
                                "image/png": result.image,
                            };
                            barcodeItem.Drag.active = dragHandler.active;
                        });
                    } else {
                        barcodeItem.Drag.active = false;
                    }
                }
            }
        }

        // This item is placed in the center of the barcode item
        // to position the tooltip that shows feedback notifications
        Item {
            id: feedbackAnchor
            anchors.centerIn: barcodeItem
            width: 5
            height: 5

            PlasmaComponents3.ToolTip {
                id: feedback
                parent: feedbackAnchor
            }
        }
    }

    PlasmaExtras.PlaceholderMessage {
        anchors.centerIn: parent
        width: parent.width - (Kirigami.Units.gridUnit * 4)
        visible: !barcodeView.valid || !barcodeView.fit
        iconName: barcodeView.valid ? "dialog-transform" : "data-error"
        text: barcodeView.valid ? i18nd("klipper", "There is not enough space to display the QR code. Try resizing this applet.") : i18nd("klipper", "Creating QR code failed")
    }
}
