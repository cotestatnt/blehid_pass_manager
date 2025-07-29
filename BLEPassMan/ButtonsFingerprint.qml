import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl
import QtQuick.Layouts
import QtQuick.Controls.Material


Item {
    id: fpSection
    anchors.horizontalCenter: parent.horizontalCenter
    anchors.bottom: parent.bottom
    width: userList.width
    property bool expanded: false

    Rectangle {
        id: gestureHandle
        anchors {
            horizontalCenter: parent.horizontalCenter
            bottom: parent.bottom
            bottomMargin: 0
        }
        width: userList.width
        height: 20           // altezza “invisible” sensibile al tap
        color: "transparent"

        // Piccolo indicatore visivo (triangolino)
        Rectangle {
            id: indicator
            anchors.centerIn: parent
            width: 30; height: 4
            radius: 2
            rotation: fpSection.expanded ? 180 : 0
            Behavior on rotation { NumberAnimation { duration: 200 } }
        }

        // Gestore del tap
        TapHandler {
            onTapped: fpSection.expanded = !fpSection.expanded
        }
    }


    Column {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 20
        width: parent.width

        Rectangle {
            id: collapsibleContent
            width: parent.width
            height: fpSection.expanded ? contentRow.implicitHeight : 0
            clip: true
            color: "transparent"

            Behavior on height { NumberAnimation { duration: 250; easing.type: Easing.InOutQuad } }

            Row {
                id: contentRow
                width: parent.width
                spacing: 15
                anchors.horizontalCenter: parent.horizontalCenter

                DelayButton {
                    id: enrollButton
                    height: Settings.fieldHeight
                    width: (parent.width - 20) / 2
                    delay: 2000
                    text: qsTr("ENROLL FINGERPRINT")

                    // Sostituisci l’intero stile di Material
                    background: Rectangle {
                        color: Settings.buttonColor
                        radius: Settings.buttonRadius

                        // Barra di progresso Material
                        Rectangle {
                            radius: Settings.buttonRadius
                            anchors.left: parent.left
                            anchors.bottom: parent.bottom
                            width: parent.width * enrollButton.progress
                            height: parent.height
                            color: Settings.disabledButtonColor
                        }
                    }

                    onActivated: {
                        deviceHandler.enrollFingerprint()
                        enrollButton.progress = 0
                    }
                }

                DelayButton {
                    id: clearButton
                    height: Settings.fieldHeight
                    width: (parent.width - 20) / 2
                    delay: 5000
                    text: qsTr("CLEAR FINGERPRINT DB")

                    // Sostituisci l’intero stile di Material
                    background: Rectangle {
                        color: Settings.buttonColor
                        radius: Settings.buttonRadius

                        // Barra di progresso Material
                        Rectangle {
                            radius: Settings.buttonRadius
                            anchors.left: parent.left
                            anchors.bottom: parent.bottom
                            width: parent.width * clearButton.progress
                            height: parent.height
                            color: Settings.disabledButtonColor
                        }
                    }

                    onActivated: {
                        deviceHandler.clearFingerprintDB()
                        clearButton.progress = 0
                    }
                }
            }
        }
    }

}
