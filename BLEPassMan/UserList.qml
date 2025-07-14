import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root
    color: "transparent"
    property var userList: []
    signal addUser()
    signal editUser(int index, string username, string password)
    signal removeUser(int index)

    ListView {
        id: listView
        width: Math.max(340, parent.width)
        height: Math.min(userList.length * 76 + 48, 400)
        model: userList
        anchors.top: parent.top
        anchors.horizontalCenter: parent.horizontalCenter
        spacing: 6
        clip: true

        delegate: ItemDelegate {
            id: delegate
            width: listView.width - 16
            height: 70
            checkable: false
            padding: 0
            highlighted: false
            background: Rectangle {
                color: Qt.lighter("#dedede", 1.05)
                border.color: "#bdbdbd"
                radius: 14
                border.width: 1
                anchors.fill: parent
            }
            contentItem: RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 18
                anchors.rightMargin: 10
                spacing: 16

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 2

                    Label {
                        text: model.username
                        font.pixelSize: 20
                        font.bold: true
                        color: "#222"
                        elide: Text.ElideRight
                        horizontalAlignment: Text.AlignLeft
                    }
                    Label {
                        text: "Password: " + model.password
                        font.pixelSize: 13
                        color: "#777"
                        elide: Text.ElideMiddle
                        horizontalAlignment: Text.AlignLeft
                    }
                }

                ToolButton {
                    icon.name: "edit"
                    ToolTip.text: qsTr("Edit user")
                    onClicked: root.editUser(index, model.username, model.password)
                }
                ToolButton {
                    icon.name: "delete"
                    ToolTip.text: qsTr("Delete user")
                    onClicked: root.removeUser(index)
                }
            }

            // Effetto hover/material
            states: State {
                when: delegate.pressed || delegate.hovered
                PropertyChanges { target: delegate; highlighted: true }
            }
            transitions: Transition {
                NumberAnimation { properties: "opacity"; duration: 120 }
            }
        }

        // Animazione inserimento/rimozione
        add: Transition {
            NumberAnimation { properties: "opacity,y"; from: 0; duration: 180 }
        }
        remove: Transition {
            NumberAnimation { properties: "opacity,y"; to: 0; duration: 130 }
        }

        // Placeholder
        footer: Label {
            text: userList.length === 0 ? qsTr("No users. Click + to add.") : ""
            font.italic: true
            color: "#aaa"
            horizontalAlignment: Text.AlignHCenter
            width: listView.width
            height: 40
        }
    }



    // Floating + button
    RoundButton {
        id: addButton
        anchors.right: listView.right
        anchors.bottom: listView.bottom
        anchors.margins: 12
        width: 50
        height: 50
        radius: 25
        icon.name: "plus"
        onClicked: root.addUser()
        visible: true
        ToolTip.text: qsTr("Add new user")
    }
}
