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
    property color textColor: "white"
    property color accentColor: "#ffb347"

    rows: 3
    columns: 2

    Label {
        text: qsTr("Username")
        color: grid.textColor
        Layout.alignment: Qt.AlignLeft | Qt.AlignBaseline
    }

    TextField {
        id: username
        Layout.fillWidth: true
        Layout.minimumWidth: grid.minimumInputSize
        Layout.alignment: Qt.AlignLeft | Qt.AlignBaseline
        placeholderText: qsTr("Enter username")
        color: grid.textColor
    }

    Label {
        text: qsTr("Password")
        color: grid.textColor
        Layout.alignment: Qt.AlignLeft | Qt.AlignBaseline
    }

    TextField {
        id: password
        placeholderText: qsTr("Enter password or placeholders, e.g. mypwd{ENTER}")
        echoMode: TextInput.Password
        Layout.fillWidth: true
        Layout.minimumWidth: grid.minimumInputSize
        Layout.alignment: Qt.AlignLeft | Qt.AlignBaseline
        font.family: "Monospace"
        color: grid.textColor
        selectionColor: accentColor
        selectedTextColor: "#000"
        rightPadding: eyeButton.width + (helpButton.visible ? helpButton.width + 4 : 0) + 8

        Item {
            id: eyeButton
            width: 26
            height: 26
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            Text {
                anchors.centerIn: parent
                text: password.echoMode === TextInput.Password ? "👁" : "🙈"
                font.pixelSize: 16
                color: grid.textColor
            }
            MouseArea {
                anchors.fill: parent
                onClicked: password.echoMode =
                               (password.echoMode === TextInput.Password
                                ? TextInput.Normal
                                : TextInput.Password)
            }
        }

        Item {
            id: helpButton
            visible: password.echoMode === TextInput.Normal
            width: 24
            height: 24
            anchors.verticalCenter: eyeButton.verticalCenter
            anchors.right: eyeButton.left
            anchors.rightMargin: 4

            Rectangle {
                anchors.fill: parent
                radius: 4
                color: Qt.alpha(grid.accentColor, helpButton.hovered ? 0.35 : 0.22)
                border.color: grid.accentColor
            }

            Text {
                anchors.centerIn: parent
                text: "?"
                font.bold: true
                font.pixelSize: 14
                color: grid.textColor
            }

            property bool hovered: false
            MouseArea {
                anchors.fill: parent
                hoverEnabled: true
                onEntered: helpButton.hovered = true
                onExited: helpButton.hovered = false
                onClicked: placeholderDialog.open()
            }
            ToolTip.visible: hovered
            ToolTip.text: qsTr("Show available placeholders")
        }
    }

    Label {
        text: qsTr("Login type")
        color: grid.textColor
    }
    ComboBox {
        id: loginType
        model: ["BLE", "USB", "Both"]
        onAccepted: loginTypeIndex = loginType.currentIndex
        palette.text: grid.textColor
    }

    Label {
        text: qsTr("CTRL+ALT+DEL")
        color: grid.textColor
        Layout.alignment: Qt.AlignLeft | Qt.AlignBaseline
    }
    CheckBox {
        id: winlogin
        checked: false
        contentItem: Text { text: parent.text; visible: false }
    }

    Label {
        text: qsTr("Send ENTER key")
        color: grid.textColor
        Layout.alignment: Qt.AlignLeft | Qt.AlignBaseline
    }
    CheckBox {
        id: sendEnter
        checked: false
        contentItem: Text { text: parent.text; visible: false }
    }

    Label {
        text: qsTr("Magic finger")
        color: grid.textColor
        Layout.alignment: Qt.AlignLeft | Qt.AlignBaseline
    }
    CheckBox {
        id: autoFinger
        checked: false
        contentItem: Text { text: parent.text; visible: false }
    }

    Label {
        text: qsTr("Fingerprint ID")
        visible: autoFinger.checked
        color: grid.textColor
    }
    ComboBox {
        id: fingerprintIndex
        model: [-1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9]
        visible: autoFinger.checked
        palette.text: grid.textColor
    }

    Dialog {
        id: placeholderDialog
        title: qsTr("Placeholder disponibili")
        modal: true

        property int dialogContentWidth: 460
        width: dialogContentWidth
        height: 500
        standardButtons: Dialog.Ok
        padding: 0

        background: Rectangle {
            color: "#222"
            radius: 6
            border.color: "#444"
        }

        contentItem: ScrollView {
            id: scroll
            // Riempi il dialog con margini interni
            anchors.fill: parent
            anchors.margins: 5

            clip: true
            ScrollBar.vertical.policy: ScrollBar.AsNeeded

            // Contenitore con larghezza fissa derivata
            Item {
                id: contentWrapper
                width: placeholderDialog.dialogContentWidth // fissa, non dipende da implicitWidth dei figli
                // Altezza determinata dalla Column (scroll gestisce overflow)
                Column {
                    id: helpColumn
                    spacing: 10
                    x: 0
                    y: 0
                    // Larghezza controllata senza riferimenti circolari
                    width: contentWrapper.width
                    padding: 16

                    // NOTA: Text usa width = parent.width per wrapping stabile
                    Component.onCompleted: {} // evita modifiche dinamiche post-layout

                    Repeater {
                        model: [
                            { code: "{ENTER}",        desc: "Invia Enter" },
                            { code: "{TAB}",          desc: "Invia Tab" },
                            { code: "{ESC}",          desc: "Invia Escape" },
                            { code: "{BACKSPACE}",    desc: "Invia Backspace (alias {BKSP})" },
                            { code: "{DELAY:500}",    desc: "Pausa 500 ms" },
                            { code: "{DELAY:1000}",   desc: "Pausa 1000 ms" },
                            { code: "{CTRL+ALT+DEL}", desc: "Ctrl + Alt + Canc (login Windows)" },
                            { code: "{SHIFT+TAB}",    desc: "Shift + Tab (naviga indietro)" }
                        ]
                        delegate: Row {
                            spacing: 8
                            width: parent.width
                            Text {
                                text: modelData.code
                                font.family: "Monospace"
                                color: grid.accentColor
                                font.bold: true
                                width: 140
                                wrapMode: Text.NoWrap
                            }
                            Text {
                                text: modelData.desc
                                wrapMode: Text.Wrap
                                color: grid.textColor
                                width: parent.width - 140 - 8
                            }
                        }
                    }

                    Rectangle {
                        width: parent.width
                        height: 1
                        color: "#666"
                        opacity: 0.35
                    }

                    Text {
                        text: qsTr("Esempio: ") + "<b>Abc{ENTER}{DELAY:1000}{CTRL+ALT+DEL}X</b>"
                        wrapMode: Text.Wrap
                        width: parent.width
                        color: grid.textColor
                        textFormat: Text.RichText
                    }

                    Text {
                        text: qsTr("Per scrivere una graffa letterale usa: {{ per { e }} per }")
                        wrapMode: Text.Wrap
                        width: parent.width
                        font.italic: true
                        color: grid.textColor
                    }

                    Text {
                        text: qsTr("Solo i delay 500 e 1000 ms sono supportati.")
                        wrapMode: Text.Wrap
                        width: parent.width
                        color: "#ff6666"
                    }

                    // Spaziatore finale per aria
                    Item { width: parent.width; height: 8 }
                }
            }
        }
    }
}
