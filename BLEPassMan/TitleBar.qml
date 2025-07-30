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
            Text {
                anchors.fill: parent
                // anchors.bottom: parent.bottom
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignBottom
                text: titleBar.__titles[caption.index]
                font.pixelSize: Settings.microFontSize
                color: Settings.textColor

                visible: app.__currentIndex
            }
            MouseArea {
                id: mouseArea
                anchors.fill: parent
                hoverEnabled: true
                onClicked: titleBar.titleClicked(caption.index)
            }
        }
    }
}
