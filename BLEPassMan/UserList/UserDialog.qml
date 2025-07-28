// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls
import BLEPassMan

Dialog {
    id: dialog
    /* larghezza voluta */
    width: parent.width + 40
    height: implicitHeight

    /* centratura orizzontale */
    x: (parent.width  - width)  / 2
    y: 0 - titleBar.height + 10


    signal finished(var user)

    function createContact() {
        form.username.clear();
        form.password.clear();
        form.winlogin.checked = false;
        form.autoFinger.checked = false;
        // form.fingerprintIndex.currentValue = 0;

        dialog.title = qsTr("Add User");
        dialog.open();
    }

    function editContact(contact) {
        form.username.text = contact.username;
        form.password.text = contact.password;
        form.winlogin.checked = contact.winlogin;
        form.autoFinger.checked = contact.autoFinger;
        form.fingerprintIndex.currentValue = contact.fingerprintIndex;

        dialog.title = qsTr("Edit User");
        dialog.open();
    }

    // y: 5
    // x: parent.width / 2 - width / 2
    // width: parent.width
    // padding: 10

    focus: true
    modal: true
    title: qsTr("Add Contact")
    standardButtons: Dialog.Ok | Dialog.Cancel

    contentItem: UserForm {
        id: form
    }

    onAccepted: {
        if (form.username.text && form.password.text) {
            finished({
                username: form.username.text,
                password: form.password.text,
                winlogin: form.winlogin.checked,
                autoFinger: form.autoFinger.checked,
                fingerprintIndex: form.fingerprintIndex.currentValue
            });
        }
    }
}
