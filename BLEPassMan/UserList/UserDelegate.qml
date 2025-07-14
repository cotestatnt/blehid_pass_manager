// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

ItemDelegate {
    id: delegate
    checkable: true

    required property string username
    required property string password

    contentItem: ColumnLayout {
        spacing: 10

        Label {
            text: delegate.username
            font.bold: true
            elide: Text.ElideRight
            Layout.fillWidth: true
        }

        GridLayout {
            id: grid
            visible: false

            columns: 2
            rowSpacing: 10
            columnSpacing: 10

            Label {
                text: qsTr("Username:")
                Layout.leftMargin: 60
            }

            Label {
                text: delegate.username
                font.bold: true
                elide: Text.ElideRight
                Layout.fillWidth: true
            }

            Label {
                text: qsTr("Password:")
                Layout.leftMargin: 60
            }

            Label {
                text: delegate.password
                font.bold: true
                elide: Text.ElideRight
                Layout.fillWidth: true
            }
        }
    }

    states: [
        State {
            name: "expanded"
            when: delegate.checked

            PropertyChanges {
                // TODO: When Qt Design Studio supports generalized grouped properties, change to:
                //       grid.visible: true
                // qmllint disable Quick.property-changes-parsed
                target: grid
                visible: true
            }
        }
    ]
}
