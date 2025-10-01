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
    property color labelColor: "white"
    property color textColor: "black"
    property color accentColor: "#ffb347"

    rows: 3
    columns: 2
    columnSpacing: 5
    rowSpacing: 2

    Label {
        text: qsTr("Username")
        color: grid.labelColor
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
        color: grid.labelColor
        Layout.alignment: Qt.AlignLeft | Qt.AlignBaseline
    }

    TextField {
        id: password
        placeholderText: qsTr("Enter password or placeholders (mypwd{ENTER})")
        echoMode: TextInput.Password
        Layout.fillWidth: true
        Layout.minimumWidth: grid.minimumInputSize
        Layout.alignment: Qt.AlignLeft | Qt.AlignBaseline
        rightPadding: eyeButton.width + (helpButton.visible ? helpButton.width + 4 : 0) + 8

        Item {
            id: eyeButton
            width: 26
            height: 26
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            // Text {
            //     anchors.centerIn: parent
            //     text: password.echoMode === TextInput.Password ? "👁" : "🙈"
            //     font.pixelSize: 16
            //     color: grid.textColor
            // }


            Image {
                anchors.centerIn: parent
                source: password.echoMode === TextInput.Password ? "qrc:/images/eye.png" : "qrc:/images/no-eye.png"
                width: 20; height: 20
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

            // Rectangle {
            //     anchors.fill: parent
            //     radius: 4
            //     color: Qt.alpha(grid.accentColor, helpButton.hovered ? 0.35 : 0.22)
            //     border.color: grid.accentColor
            // }

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
        color: grid.labelColor        
    }
    ComboBox {
        id: loginType
        model: ["BLE", "USB", "Both"]
        onAccepted: loginTypeIndex = loginType.currentIndex
        palette.text: grid.labelColor        
    }

    Label {
        text: qsTr("CTRL+ALT+DEL")
        color: grid.labelColor
        Layout.alignment: Qt.AlignLeft | Qt.AlignBaseline        
    }
    CheckBox {
        id: winlogin
        checked: false
        contentItem: Text { text: parent.text; visible: false }   
        Layout.leftMargin: 5
    }

    Label {
        text: qsTr("Send ENTER key")
        color: grid.labelColor
        Layout.alignment: Qt.AlignLeft | Qt.AlignBaseline
        visible: false
    }
    CheckBox {
        id: sendEnter
        checked: false
        contentItem: Text { text: parent.text; visible: false }
        visible: false
    }

    Label {
        text: qsTr("Magic finger")
        color: grid.labelColor
        Layout.alignment: Qt.AlignLeft | Qt.AlignBaseline
    }
    CheckBox {
        id: autoFinger
        checked: false
        contentItem: Text { text: parent.text; visible: false }
        Layout.leftMargin: 5
    }

    Label {
        text: qsTr("Fingerprint ID")
        visible: autoFinger.checked
        color: grid.labelColor
    }
    ComboBox {
        id: fingerprintIndex
        model: [-1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9]
        visible: autoFinger.checked
        palette.text: grid.labelColor
    }

    Dialog {
        id: placeholderDialog
        title: qsTr("Available placeholders")
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
                    padding: 10
                    topPadding: 60

                    // NOTA: Text usa width = parent.width per wrapping stabile
                    Component.onCompleted: {} // evita modifiche dinamiche post-layout

                    Repeater {
                        model: [
                            { code: "{ENTER}",        desc: "Enter" },
                            { code: "{TAB}",          desc: "Tab" },
                            { code: "{ESC}",          desc: "Escape" },
                            { code: "{BACKSPACE}",    desc: "Backspace (alias {BKSP})" },
                            { code: "{DELAY:500}",    desc: "Pause 500 ms" },
                            { code: "{DELAY:1000}",   desc: "Pause 1000 ms" },
                            { code: "{CTRL+ALT+DEL}", desc: "Ctrl + Alt + Canc (login Windows)" },
                            { code: "{SHIFT+TAB}",    desc: "Shift + Tab" },
                            { code: "{SLEEP}",        desc: "Sleep after" },
                            { code: "",               desc: "" }
                        ]
                        delegate: Row {
                            spacing: 4
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
                                color: grid.labelColor
                                width: parent.width - 140 - 8
                            }
                        }
                    }

                    Rectangle {
                        width: parent.width - 30
                        height: 90
                        color: "#666"
                        opacity: 0.55

                        Column {
                            spacing: 10
                            width: contentWrapper.width
                            padding: 10

                            Text {
                                text: qsTr("Example: ") + "<b>admin{TAB}{DELAY:1000}123456{ENTER}</b>"
                                wrapMode: Text.Wrap
                                width: parent.width
                                color: grid.labelColor
                                textFormat: Text.RichText
                            }

                            Text {
                                text: qsTr("To write a literal brace, use: {{ for { and }} for }")
                                wrapMode: Text.Wrap
                                width: parent.width
                                font.italic: true
                                color: grid.labelColor
                            }

                            Text {
                                text: qsTr("Only 500 e 1000 ms delay are supported.")
                                wrapMode: Text.Wrap
                                width: parent.width
                                color: "#ff6666"
                            }
                        }
                    }



                    // Spaziatore finale per aria
                    // Item { width: parent.width; height: 8 }
                }
            }
        }
    }
}
