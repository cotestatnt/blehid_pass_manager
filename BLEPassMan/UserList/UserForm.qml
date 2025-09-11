// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

GridLayout {
    id: grid
    property alias username: username
    property alias password: password
    property alias winlogin: winlogin
    property alias sendEnter: sendEnter
    property alias autoFinger: autoFinger
    property alias fingerprintIndex: fingerprintIndex
    property alias loginType: loginType
    property int loginTypeIndex: -1
    property int minimumInputSize: 120

    rows: 3
    columns: 2

    Label {
        text: qsTr("Username")
        Layout.alignment: Qt.AlignLeft | Qt.AlignBaseline
    }

    TextField {
        id: username
        Layout.fillWidth: true
        Layout.minimumWidth: grid.minimumInputSize
        Layout.alignment: Qt.AlignLeft | Qt.AlignBaseline
        placeholderText: qsTr("Enter username")
    }

    Label {
        text: qsTr("Password")
        Layout.alignment: Qt.AlignLeft | Qt.AlignBaseline
    }

    TextField {
        id: password
        placeholderText: qsTr("Enter password or placeholders, e.g. mypwd{ENTER}")
        echoMode: TextInput.Password
        Layout.fillWidth: true
        Layout.minimumWidth: grid.minimumInputSize
        Layout.alignment: Qt.AlignLeft | Qt.AlignBaseline

        // spazio per il "pulsante" custom
        rightPadding: eyeButton.width + 8

        // Pulsante custom (nessun ToolButton => niente background grigio)
        Item {
            id: eyeButton
            width: 26
            height: 26
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter

            // Testo/emoji come icona
            Text {
                id: eyeGlyph
                anchors.centerIn: parent
                text: password.echoMode === TextInput.Password ? "👁" : "🙈"
                font.pixelSize: 16
            }

            // Area cliccabile
            MouseArea {
                anchors.fill: parent
                hoverEnabled: false   // niente hover
                onClicked: {
                    password.echoMode = (password.echoMode === TextInput.Password) ? TextInput.Normal : TextInput.Password
                }
                // (Opzionale) feedback tattile/sonoro disabilitato, se implementato altrove
            }
        }
    }

    Label {
        text: qsTr("Login type")
    }
    ComboBox {
        id: loginType
        model: ["BLE", "USB", "Both"]
        onAccepted: loginTypeIndex = loginType.currentIndex
    }

    Label {
        text: qsTr("CTRL+ALT+DEL")
        Layout.alignment: Qt.AlignLeft | Qt.AlignBaseline
    }
    CheckBox {
        id: winlogin
        checked: false
    }

    Label {
        text: qsTr("Send ENTER key")
        Layout.alignment: Qt.AlignLeft | Qt.AlignBaseline
    }
    CheckBox {
        id: sendEnter
        checked: false
    }

    Label {
        text: qsTr("Magic finger")
        Layout.alignment: Qt.AlignLeft | Qt.AlignBaseline
    }
    CheckBox {
        id: autoFinger
        checked: false
    }

    Label {
        text: qsTr("Fingerprint ID")
        visible: autoFinger.checked
    }
    ComboBox {
        id: fingerprintIndex
        model: [-1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9]
        visible: autoFinger.checked
    }
}
