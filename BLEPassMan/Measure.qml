// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import BLEPassMan


GamePage {
    id: measurePage

    required property DeviceHandler deviceHandler

    errorMessage: deviceHandler.error
    infoMessage: deviceHandler.info
    iconType: deviceHandler.icon

    Connections {
        target: deviceHandler
        function onUserListUpdated(users) {            
            const jsArray = [];

            if (users && typeof users.length !== 'undefined') {
                for (var i = 0; i < users.length; i++) {                    
                    const cleanUserObject = {};                    
                    cleanUserObject.username = users[i].username;
                    cleanUserObject.password = users[i].password;
                    jsArray.push(cleanUserObject);
                }                
                // console.log("Contenuto di jsArray convertito:", JSON.stringify(jsArray, null, 2));

            } else {
                console.log("ERRORE: Dati ricevuti non validi per la conversione.")
            }

            // Passa l'array pulito al componente.
            if (userListComponentLoader.item) {
                userListComponentLoader.item.userModel = jsArray
            }            
        }

        function onServiceReady() {
            console.log("Sync user list with device")
            measurePage.start()
            userListComponentLoader.item.syncEnabled = true
        }
    }

    // Component.onCompleted: {
    //     console.log("Measure.qml caricato. Il loader caricherà:", userListComponentLoader.source)
    // }

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
            height: 550
            radius: 5
            color: GameSettings.viewColor

            // Loader per caricare il nostro componente UserList
            Loader {
                id: userListComponentLoader
                anchors.fill: parent                
                source: "qrc:/userlist/UserList.qml"
            }
        }

        // Connections per ascoltare i segnali emessi dall'item caricato dal Loader.
        Connections {
            // Il target è l'item caricato, non il loader stesso
            target: userListComponentLoader.item

            function onAddUser(username, password) {
                console.log("QML ha richiesto di aggiungere l'utente:", username)
                deviceHandler.addUser(username, password)
            }

            function onEditUser(index, username, password) {
                console.log("QML ha richiesto di modificare l'utente all'indice:", index)
                deviceHandler.editUser(index, username, password)
            }

            function onRemoveUser(index) {
                console.log("QML ha richiesto di rimuovere l'utente all'indice:", index)
                deviceHandler.removeUser(index)
            }

            function onReadList() {
                console.log("Sync user list with device")
                measurePage.start()
            }
        }
    }

    // GameButton {
    //     id: startButton
    //     anchors.horizontalCenter: parent.horizontalCenter
    //     anchors.bottom: parent.bottom
    //     anchors.bottomMargin: GameSettings.fieldMargin
    //     width: userList.width
    //     height: GameSettings.fieldHeight
    //     enabled: measurePage.deviceHandler.alive && !measurePage.deviceHandler.measuring
    //              && measurePage.errorMessage === ""
    //     radius: GameSettings.buttonRadius

    //     onClicked: measurePage.start()

    //     Text {
    //         anchors.centerIn: parent
    //         font.pixelSize: GameSettings.microFontSize
    //         text: qsTr("START")
    //         color: GameSettings.textDarkColor
    //     }
    // }
}
