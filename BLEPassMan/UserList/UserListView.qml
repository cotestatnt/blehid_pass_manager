pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls

ListView {
    id: listView
    signal pressAndHold(int index)
    width: 320
    height: 480
    boundsBehavior: Flickable.StopAtBounds
    spacing: 5

    property int selectedIndex: -1

    delegate: UserDelegate {
        id: delegate
        width: listView.width
        required property int index
        onPressAndHold: listView.pressAndHold(index)

        checked: listView.selectedIndex === index
        onClicked: {
            if (listView.selectedIndex === index)
                listView.selectedIndex = -1; // toggle-off
            else
                listView.selectedIndex = index;
        }

    }

    ScrollBar.vertical: ScrollBar { }

}
