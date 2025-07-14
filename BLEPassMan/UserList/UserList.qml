import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import BLEPassMan

Item {
    id: root
    property int currentContact: -1
    property bool syncEnabled: true

    // Questa proprietà riceverà la QVariantList dal backend
    property var userModel: []

    // 1. Define signals to notify the backend of desired actions
    signal addUser(string username, string password)
    signal editUser(int index, string username, string password)
    signal removeUser(int index)
    signal readList()

    // Log per vedere quando il modello viene aggiornato dall'esterno (da Measure.qml)
    onUserModelChanged: {
        console.log("LOG (UserList.qml): Il modello dati 'userModel' è stato aggiornato. Numero di utenti:", userModel.length)
    }

    Component.onCompleted: {
        console.log("LOG (UserList.qml): Componente UserList creato.")
    }

    UserDialog {
        id: contactDialog
        onFinished: function(username, password) {
            // 2. Emit signals instead of modifying the model directly
            if (root.currentContact === -1)
                root.addUser(username, password)
            else
                root.editUser(root.currentContact, username, password)
        }
    }

    // UserPopup {
    //     id: contactDialog
    //     onFinished: function(username, password) {
    //         // 2. Emit signals instead of modifying the model directly
    //         if (root.currentContact === -1)
    //             root.addUser(username, password)
    //         else
    //             root.editUser(root.currentContact, username, password)
    //     }
    // }

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
            text: root.currentContact >= 0 ? userModel[root.currentContact].username : ""
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
            text: qsTr("+")
            highlighted: true
            // anchors.margins: 10
            // anchors.right: parent.right
            // anchors.bottom: parent.bottom
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
            icon.name: "bluetooth"
            icon.width: 20
            icon.height: 20
            width: 40
            height: 40
            enabled: syncEnabled
            onClicked: root.readList()
        }

        Button {
            id: myRoundButton

            // 1. Imposta l'icona come prima
            icon.source: "file:///D:/Dropbox/blehid_pass_manager/BLEPassMan/images/clock.svg" // Assicurati che il percorso sia giusto!
            icon.width: 40
            icon.height: 40

            // 2. Rimuovi il testo
            text: ""

            // 3. Crea lo sfondo rotondo personalizzato
            background: Rectangle {
                // Fai in modo che lo sfondo si adatti alla dimensione del pulsante
                width: parent.width
                height: parent.height

                // La chiave per renderlo rotondo: il raggio deve essere metà della larghezza/altezza
                radius: width / 2

                color: myRoundButton.down ? "#cccccc" : "#e0e0e0" // Colore diverso se premuto
                border.color: "#aaaaaa"
                border.width: 1
            }
        }
    }
}
