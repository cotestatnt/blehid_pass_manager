// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

pragma ComponentBehavior: Bound
import QtQuick

Rectangle {
    id: titleBar

    property var __titles: ["< CONNECTIONS", ""]
    property int currentIndex: 0

    signal titleClicked(int index)

    height: Settings.fieldHeight
    color: Settings.titleColor

    function getBatteryColor() {
        if (deviceHandler.batteryLevel > 50) return Settings.textColor
        if (deviceHandler.batteryLevel > 20) return "#FFA500" // Arancione
        return "#FF4444" // Rosso
    }

    Rectangle {
        anchors.bottom: parent.bottom
        width: parent.width
        height: parent.height
        x: titleBar.currentIndex * width
        color: Settings.selectedTitleColor

        Behavior on x {
            NumberAnimation {
                duration: 200
            }
        }
    }

    Repeater {
        model: 2
        Rectangle {
            id: caption
            required property int index
            property bool hoveredOrPressed: mouseArea.pressed || mouseArea.containsMouse
            width: titleBar.width / 2
            height: titleBar.height -5
            x: index * width
            color: (titleBar.currentIndex !== index) && hoveredOrPressed ? Settings.hoverTitleColor : "transparent"

            // Primo rettangolo - pulsante per tornare indietro
            Text {
                anchors.fill: parent
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignBottom
                text: titleBar.__titles[caption.index]
                font.pixelSize: Settings.microFontSize
                color: Settings.textColor
                visible: app.__currentIndex && caption.index === 0
            }

            // Secondo rettangolo - indicatore batteria
            Item {
                anchors.fill: parent
                visible: app.__currentIndex && caption.index === 1 && deviceHandler.batteryLevel >= 0

                Row {
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.bottom: parent.bottom
                    spacing: 10

                    // Icona batteria
                    Rectangle {
                        width: 20
                        height: 10
                        border.color: getBatteryColor()
                        border.width: 1
                        color: "transparent"
                        anchors.bottom: parent.bottom
                        anchors.bottomMargin: 5

                        // Terminale positivo della batteria
                        Rectangle {
                            width: 2
                            height: 4
                            anchors.right: parent.right
                            anchors.rightMargin: -2
                            anchors.verticalCenter: parent.verticalCenter
                            color: getBatteryColor()
                        }

                        // Riempimento batteria
                        Rectangle {
                            width: Math.max(1, (parent.width - 2) * deviceHandler.batteryLevel / 100)
                            height: parent.height - 2
                            anchors.left: parent.left
                            anchors.leftMargin: 1
                            anchors.verticalCenter: parent.verticalCenter
                            color: getBatteryColor()

                            Behavior on width {
                                NumberAnimation {
                                    duration: 300
                                    easing.type: Easing.OutQuad
                                }
                            }
                        }
                    }

                    // Testo percentuale
                    Text {
                        text: " " + deviceHandler.batteryLevel + "%"
                        color: getBatteryColor()
                        font.pixelSize: Settings.microFontSize
                        anchors.bottom: parent.bottom
                    }
                }
            }

            MouseArea {
                id: mouseArea
                anchors.fill: parent
                hoverEnabled: true
                onClicked: titleBar.titleClicked(caption.index)
                // Disabilita il click per il secondo rettangolo (batteria)
                enabled: caption.index === 0
            }
        }
    }
}
