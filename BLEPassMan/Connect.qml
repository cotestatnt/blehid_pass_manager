// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

pragma ComponentBehavior: Bound
import QtQuick
import BLEPassMan

Page {
    id: connectPage

    required property ConnectionHandler connectionHandler
    required property DeviceFinder deviceFinder
    required property DeviceHandler deviceHandler

    signal showMeasurePage

    errorMessage: deviceFinder.error
    infoMessage: deviceFinder.info
    iconType: deviceFinder.icon

    Text {
        id: viewCaption
        anchors {
            top: parent.top
            topMargin: Settings.fieldMargin + connectPage.messageHeight
            horizontalCenter: parent.horizontalCenter
        }
        width: parent.width - Settings.fieldMargin * 2
        height: Settings.fieldHeight
        horizontalAlignment: Text.AlignLeft
        verticalAlignment: Text.AlignVCenter
        color: Settings.textColor
        font.pixelSize: Settings.smallFontSize
        text: qsTr("Found Devices")
    }

    Rectangle {
        id: viewContainer
        anchors.top: viewCaption.bottom
        // only BlueZ platform has address type selection
        anchors.bottom: connectPage.connectionHandler.requiresAddressType ? addressTypeButton.top : searchButton.top
        anchors.bottomMargin: Settings.fieldMargin
        anchors.horizontalCenter: parent.horizontalCenter
        width: parent.width - Settings.fieldMargin * 2
        color: Settings.viewColor
        radius: Settings.buttonRadius

        ListView {
            id: devices
            anchors.fill: parent
            model: connectPage.deviceFinder.devices
            clip: true

            delegate: Rectangle {
                id: box

                required property int index
                required property var modelData

                height: Settings.fieldHeight * 1.2
                width: devices.width
                color: index % 2 === 0 ? Settings.delegate1Color : Settings.delegate2Color

                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        connectPage.deviceFinder.connectToService(box.modelData.deviceAddress)
                        connectPage.showMeasurePage()
                    }
                }

                Text {
                    id: device
                    font.pixelSize: Settings.microFontSize
                    text: box.modelData.deviceName
                    anchors.top: parent.top
                    anchors.topMargin: parent.height * 0.15
                    anchors.leftMargin: parent.height * 0.15
                    anchors.left: parent.left
                    color: Settings.textColor
                }

                Text {
                    id: deviceAddress
                    font.pixelSize: Settings.microFontSize
                    text: box.modelData.deviceAddress
                    anchors.bottom: parent.bottom
                    anchors.bottomMargin: parent.height * 0.15
                    anchors.rightMargin: parent.height * 0.15
                    anchors.right: parent.right
                    color: Qt.darker(Settings.textColor)
                }
            }
        }
    }

    CustomButton {
        id: addressTypeButton
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: searchButton.top
        anchors.bottomMargin: Settings.fieldMargin * 0.5
        width: viewContainer.width
        height: Settings.fieldHeight
        visible: connectPage.connectionHandler.requiresAddressType // only required on BlueZ
        state: "public"
        onClicked: state === "public" ? state = "random" : state = "public"

        states: [
            State {
                name: "public"
                PropertyChanges {
                    addressTypeText.text: qsTr("PUBLIC ADDRESS")
                }
                PropertyChanges {
                    connectPage.deviceHandler.addressType: DeviceHandler.PublicAddress
                }
            },
            State {
                name: "random"
                PropertyChanges {
                    addressTypeText.text: qsTr("RANDOM ADDRESS")
                }
                PropertyChanges {
                    connectPage.deviceHandler.addressType: DeviceHandler.RandomAddress
                }
            }
        ]

        Text {
            id: addressTypeText
            anchors.centerIn: parent
            font.pixelSize: Settings.microFontSize
            color: Settings.textDarkColor
        }
    }

    CustomButton {
        id: searchButton
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: Settings.fieldMargin
        width: viewContainer.width
        height: Settings.fieldHeight
        enabled: !connectPage.deviceFinder.scanning
        onClicked: connectPage.deviceFinder.startSearch()

        Text {
            anchors.centerIn: parent
            font.pixelSize: Settings.microFontSize
            text: qsTr("START SEARCH")
            color: Settings.textDarkColor
        }
    }
}
