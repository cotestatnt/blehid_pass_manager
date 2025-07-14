// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls

Dialog {
    id: dialog

    signal finished(string username, string password)

    function createContact() {
        form.username.clear();
        form.password.clear();

        dialog.title = qsTr("Add User");
        dialog.open();
    }

    function editContact(contact) {
        form.username.text = contact.username;
        form.password.text = contact.password;

        dialog.title = qsTr("Edit User");
        dialog.open();
    }

    y: 40
    x: parent.width / 2 - width / 2
    width: parent.width - 40
    padding: 20

    focus: true
    modal: true
    title: qsTr("Add Contact")
    standardButtons: Dialog.Ok | Dialog.Cancel

    contentItem: UserForm {
        id: form
    }

    onAccepted: {
        if (form.username.text && form.password.text) {
            finished(form.username.text, form.password.text);
        }
    }
}
