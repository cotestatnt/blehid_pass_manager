// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import BLEPassMan


GamePage {
    id: measurePage

    required property DeviceHandler deviceHandler

    errorMessage: deviceHandler.error
    infoMessage: deviceHandler.info
    iconType: deviceHandler.icon

    function close() {        
        deviceHandler.disconnectService()
    }

    function start() {
        deviceHandler.getUserList()
    }

    Column {
        anchors.centerIn: parent
        spacing: GameSettings.fieldHeight * 0.5

        Rectangle {
            id: userList            
            readonly property real innerSpacing: Math.min(width * 0.05, 25)

            anchors.horizontalCenter: parent.horizontalCenter
            width: Math.min(measurePage.width, measurePage.height - GameSettings.fieldHeight * 4) - 2 * GameSettings.fieldMargin
            // height: 550
            radius: 5
            color: GameSettings.viewColor

            UserList {
                id: userListWidget
                userList: measurePage.deviceHandler.userList  // Assumi che DeviceHandler abbia property var userList, aggiornata dal segnale
                onAddUser: {
                    // Apri dialog per inserire nuovo utente
                    // Alla conferma, chiama: measurePage.deviceHandler.addUser(username, password)
                }
                onEditUser: function(index, username, password) {
                    // Apri dialog con username/password già compilati
                    // Alla conferma, chiama: measurePage.deviceHandler.editUser(index, username, password)
                }
                onRemoveUser: function(index) {
                    measurePage.deviceHandler.removeUser(index)
                }
            }


        }
    }

    GameButton {
        id: startButton
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: GameSettings.fieldMargin
        width: userList.width
        height: GameSettings.fieldHeight
        enabled: measurePage.deviceHandler.alive && !measurePage.deviceHandler.measuring
                 && measurePage.errorMessage === ""
        radius: GameSettings.buttonRadius

        onClicked: measurePage.start()

        Text {
            anchors.centerIn: parent
            font.pixelSize: GameSettings.microFontSize
            text: qsTr("START")
            color: GameSettings.textDarkColor
        }
    }
}
