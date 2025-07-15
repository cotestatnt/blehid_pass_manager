// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import BLEPassMan

Item {
    id: page

    property string errorMessage: ""
    property string infoMessage: ""
    property real messageHeight: msg.height
    property bool hasError: errorMessage != ""
    property bool hasInfo: infoMessage != ""
    property int iconType: BluetoothBaseClass.IconNone

    function iconTypeToName(icon: int) : string {
        switch (icon) {
        case BluetoothBaseClass.IconNone: return ""
        case BluetoothBaseClass.IconBluetooth: return "qrc:/images/bluetooth.svg"
        case BluetoothBaseClass.IconError: return "qrc:/images/alert.svg"
        case BluetoothBaseClass.IconProgress: return "qrc:/images/progress.svg"
        case BluetoothBaseClass.IconSearch: return "qrc:/images/search.svg"
        }
    }

    Rectangle {
        id: msg
        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
            topMargin: Settings.fieldMargin * 0.5
            leftMargin: Settings.fieldMargin
            rightMargin: Settings.fieldMargin
        }
        height: Settings.fieldHeight
        radius: Settings.buttonRadius
        color: page.hasError ? Settings.errorColor : "transparent"
        visible: page.hasError || page.hasInfo
        border {
            width: 1
            color: page.hasError ? Settings.errorColor : Settings.infoColor
        }

        Image {
            id: icon
            readonly property int imgSize: Settings.fieldHeight * 0.5
            anchors {
                left: parent.left
                leftMargin: Settings.fieldMargin * 0.5
                verticalCenter: parent.verticalCenter
            }
            visible: source.toString() !== ""
            source: page.iconTypeToName(page.iconType)
            sourceSize.width: imgSize
            sourceSize.height: imgSize
            fillMode: Image.PreserveAspectFit
        }

        Text {
            id: error
            anchors {
                fill: parent
                leftMargin: Settings.fieldMargin + icon.width
                rightMargin: Settings.fieldMargin + icon.width
            }
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            minimumPixelSize: 5
            font.pixelSize: Settings.microFontSize
            fontSizeMode: Text.Fit
            color: page.hasError ? Settings.textColor : Settings.infoColor
            text: page.hasError ? page.errorMessage : page.infoMessage
        }
    }
}
