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
    property alias autoFinger: autoFinger
    property alias fingerprintIndex: fingerprintIndex
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
        placeholderText: qsTr("Enter password")
        echoMode: TextInput.Password
        Layout.fillWidth: true
        Layout.minimumWidth: grid.minimumInputSize
        Layout.alignment: Qt.AlignLeft | Qt.AlignBaseline
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
        text: qsTr("Auto fingerprint")
        Layout.alignment: Qt.AlignLeft | Qt.AlignBaseline
    }
    CheckBox {
        id: autoFinger
        checked: false
    }

    Label {
        text: qsTr("Fingerprint ID")
    }
    ComboBox {
        id: fingerprintIndex
        model: [0, 1, 2, 3, 4, 5, 6, 7, 8, 9]
    }

}
