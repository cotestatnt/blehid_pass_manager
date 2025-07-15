// // Copyright (C) 2023 The Qt Company Ltd.
// // SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material

import BLEPassMan

ItemDelegate {
    id: delegate
    checkable: true

    required property string username
    required property string password
    property bool passwordVisible: false

    background: Rectangle {
        radius: 8
        border.width: 2
        border.color: delegate.checked ? "orange" : "#AAAAAA"
        color: "#23272f"
    }

    contentItem: ColumnLayout {
        spacing: 10

        Label {
            text: delegate.username
            color: "white"
            font.bold: true
            elide: Text.ElideRight
            Layout.fillWidth: true
        }

        GridLayout {
            columns: 2
            visible: delegate.checked
            rowSpacing: 10
            columnSpacing: 10

            Label { text: qsTr("Username:"); color: "white"; Layout.leftMargin: 60 }
            Label { text: delegate.username; font.bold: true; color: "white"; Layout.fillWidth: true }

            Label { text: qsTr("Password:"); color: "white"; Layout.leftMargin: 60 }
            RowLayout {
                Layout.fillWidth: true
                Label {
                    text: passwordVisible ? delegate.password : "*****************"
                    font.bold: true
                    color: "white"
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }

                ToolButton {
                    text: passwordVisible ? "\u{1F441}" : "\u{1F576}" // 👁 / 🕶️
                    onClicked: passwordVisible = !passwordVisible
                    font.pixelSize: 18
                    contentItem: Text {
                        text: parent.text
                        color: "white"
                        font.pixelSize: 18
                    }
                    background: Rectangle { color: "transparent" }
                }
            }
        }
    }
}
