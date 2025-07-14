// import QtQuick
// import QtQuick.Controls

// ListView {
//     id: listView

//     delegate: UserDelegate {
//         width: listView.width

//         // Passa l'intero oggetto del modello al delegato
//         userData: model
//     }

//     header: Item {
//         width: listView.width
//         height: 40
//         Text {
//             anchors.centerIn: parent
//             text: "Lista Utenti"
//             font.pixelSize: 18
//             font.bold: true
//         }
//     }
// }


pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls


ListView {
    id: listView

    signal pressAndHold(int index)

    width: 320
    height: 480

    focus: true
    boundsBehavior: Flickable.StopAtBounds

    section.property: "fullName"
    section.criteria: ViewSection.FirstCharacter
    section.delegate: SectionDelegate {
        width: listView.width
    }

    delegate: UserDelegate {
        id: delegate
        width: listView.width

        required property int index

        onPressAndHold: listView.pressAndHold(index)
    }

    // model: ContactModel {
    //     id: contactModel
    // }

    ScrollBar.vertical: ScrollBar { }
}
