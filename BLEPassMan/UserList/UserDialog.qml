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


    signal finished(string username, string password, bool winlogin)

    function createContact() {
        form.username.clear();
        form.password.clear();
        form.winlogin.checked = false;

        dialog.title = qsTr("Add User");
        dialog.open();
    }

    function editContact(contact) {
        form.username.text = contact.username;
        form.password.text = contact.password;
        form.winlogin.checked = contact.winlogin;

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
            finished(form.username.text, form.password.text, form.winlogin.checked);
        }
    }
}
