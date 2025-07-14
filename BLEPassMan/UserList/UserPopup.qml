import QtQuick
import QtCore
import QtQuick.Layouts
import QtQuick.Controls


Frame {
    id: projectOptions
    y: 40
    x: parent.width / 2 - width / 2
    width: parent.width - 40
    padding: 20
    focus: true
    visible: false;

    signal finished(string username, string password)

    property int fieldWidth: 150
    property alias username: username.text
    property alias password: password.text

    property Component commonBackgroundComponent: Component {
        Rectangle {
            color: "black"
            border.color: "white"
            border.width: 1
        }
    }

    function createContact() {
        username.clear();
        password.clear();
        visible = true;
    }

    function editContact(contact) {
        username.text = contact.username;
        password.text = contact.password;
        visible = true;
    }


    function close() {
        visible = false;
    }

    background: Rectangle {
        color: "black"
        border.color: "white"
        border.width: 1
        radius: 10
    }

    Column {
        spacing: 10
        anchors.fill: parent
        anchors.margins: 20

        Label {
            text: qsTr("Username")
            color: "white"
            font.pixelSize: 20
            font.bold: true
        }

        TextField {
            id: username
            placeholderText: (text.length === 0 && !focus) ? qsTr("Insert username") : ""
            color: "white"
            font.pixelSize: 20
            anchors.left: parent.left
            anchors.right: parent.right
            background: projectOptions.commonBackgroundComponent.createObject(this)
        }

        Label {
            text: qsTr("Password")
            color: "white"
            font.pixelSize: 20
            font.bold: true
        }

        TextField {
            id: password
            placeholderText: (text.length === 0 && !focus) ? qsTr("Insert a password") : ""
            color: "white"
            font.pixelSize: 20
            anchors.left: parent.left
            anchors.right: parent.right
            background: projectOptions.commonBackgroundComponent.createObject(this)
        }

    }

    Button {
        text: qsTr("Add")
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.margins: 20

        onClicked: {
            if (username.text && password.text) {
                finished(username.text, password.text);
            }
            visible = false
        }
    }
}

