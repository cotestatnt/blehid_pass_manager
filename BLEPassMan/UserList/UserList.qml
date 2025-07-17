import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import BLEPassMan

Item {
    id: root
    property int currentContact: -1
    property bool syncEnabled: true
    property var userModel: []
    signal addUser(string username, string password)
    signal editUser(int index, string username, string password)
    signal removeUser(int index)
    signal readList()


    // // Log per vedere quando il modello viene aggiornato dall'esterno (da Measure.qml)
    // onUserModelChanged: {
    //     console.log("LOG (UserList.qml): Il modello dati 'userModel' è stato aggiornato. Numero di utenti:", userModel.length)
    // }

    // Component.onCompleted: {
    //     console.log("LOG (UserList.qml): Componente UserList creato.")
    // }

    UserDialog {
        id: contactDialog
        onFinished: function(username, password) {            
            if (root.currentContact === -1)
                root.addUser(username, password)
            else
                root.editUser(root.currentContact, username, password)
        }
    }

    Menu {
        id: contactMenu
        x: parent.width / 2 - width / 2
        y: parent.height / 2 - height / 2
        modal: true

        Label {
            padding: 10
            font.bold: true
            width: parent.width
            horizontalAlignment: Qt.AlignHCenter
            // text: root.currentContact >= 0 ? userModel[root.currentContact].username : ""
        }
        MenuItem {
            text: qsTr("Edit...")
            onTriggered: contactDialog.editContact(userModel[root.currentContact] )
        }
        MenuItem {
            text: qsTr("Remove")
            onTriggered: {
                root.removeUser(root.currentContact)
            }
        }
    }

    UserListView {
        id: userView
        anchors.fill: parent
        // Passa il modello di dati alla vista
        model: root.userModel
        onPressAndHold: function(index) {
            root.currentContact = index
            contactMenu.open()
        }
    }

    RowLayout {
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        anchors.bottomMargin: 20
        anchors.rightMargin: 20
        spacing: 15

        RoundButton {
            icon.source: "qrc:/images/add.png"
            icon.width: 20
            icon.height: 20
            width: 40
            height: 40
            onClicked: {
                root.currentContact = -1
                contactDialog.createContact()
            }
        }

        RoundButton {
            id: readlistButton            
            icon.source: "qrc:/images/bluetooth.png"
            icon.width: 20
            icon.height: 20
            width: 40
            height: 40
            enabled: syncEnabled            
            onClicked: root.readList()
        }
    }
}
