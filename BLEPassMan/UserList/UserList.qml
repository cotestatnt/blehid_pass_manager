import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import BLEPassMan

Item {
    id: root
    property int currentContact: -1
    property bool syncEnabled: true
    property var userModel: []

    signal addUser(var user)
    signal editUser(int index, var user)
    signal removeUser(int index)
    signal readList()


    // Log per vedere quando il modello viene aggiornato dall'esterno (da Measure.qml)
    // onUserModelChanged: {
    //     console.log("LOG (UserList.qml): Il modello dati 'userModel' è stato aggiornato. Numero di utenti:", userModel.length)
    // }

    // Component.onCompleted: {
    //     console.log("LOG (UserList.qml): Componente UserList creato.")
    // }

    UserDialog {
        id: contactDialog
        onFinished: function(user) {
            if (root.currentContact === -1)
                root.addUser(user)
            else
                root.editUser(root.currentContact, user)
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

    DelayButton {
        id: clearUserDB
        checked: true
        delay: 10000

        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.bottomMargin: 20
        anchors.leftMargin: 20

        onActivated: {
            deviceHandler.clearUserDB()
            clearUserDB.progress = 0
        }

        contentItem: Text {
            // Usa l'icona Unicode per il cestino
            text: "🗑"  // Oppure usa "\uF1F8" se hai FontAwesome
            font.pixelSize: 20
            color: Material.foreground
            opacity: enabled ? 1.0 : 0.6
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }

        background: Rectangle {
            implicitWidth: 40
            implicitHeight: 40
            opacity: enabled ? 1 : 0.6

            // Colore grigio scuro per Material Dark theme
            color: clearUserDB.down ? "#616161" : "#424242"

            radius: size / 2

            readonly property real size: Math.min(clearUserDB.width, clearUserDB.height)
            width: size
            height: size
            anchors.centerIn: parent

            // Ombra
            Rectangle {
                anchors.fill: parent
                anchors.topMargin: clearUserDB.down ? 1 : 3
                anchors.leftMargin: clearUserDB.down ? 0 : 1
                radius: parent.radius
                color: "#30000000"
                z: -1
            }

            Canvas {
                id: canvas
                anchors.fill: parent

                Connections {
                    target: clearUserDB
                    function onProgressChanged() { canvas.requestPaint(); }
                }

                onPaint: {
                    var ctx = getContext("2d")
                    ctx.clearRect(0, 0, width, height)
                    ctx.strokeStyle = Material.foreground
                    ctx.lineWidth = parent.size / 20
                    ctx.beginPath()
                    var startAngle = Math.PI / 5 * 3
                    var endAngle = startAngle + clearUserDB.progress * Math.PI / 5 * 9
                    ctx.arc(width / 2, height / 2, width / 2 - ctx.lineWidth / 2 - 2, startAngle, endAngle)
                    ctx.stroke()
                }
            }
        }
    }


    RowLayout {
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        anchors.bottomMargin: 20
        anchors.rightMargin: 20
        spacing: 15

        RoundButton {
            text : "👤+ "
            font.pixelSize: 20
            width: 40
            height: 40
            onClicked: {
                root.currentContact = -1
                contactDialog.createContact()
            }
        }

        RoundButton {
            id: readlistButton            
            text: "👤\u2B94"
            font.pixelSize: 18
            width: 40
            height: 40
            enabled: syncEnabled            
            onClicked: root.readList()
        }
    }
}
