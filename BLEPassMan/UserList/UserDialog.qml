// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls
import BLEPassMan

Dialog {
    id: dialog

    width: parent.width + 40
    height: implicitHeight

    x: (parent.width  - width)  / 2
    y: 0 - (titleBar.height + 20)

    signal finished(var user)

    function createContact() {
        form.username.clear();
        form.password.clear();
        form.winlogin.checked = false;
        form.sendEnter.checked = false;
        form.autoFinger.checked = false;
        form.fingerprintIndex.currentIndex = 0;
        form.loginType.currentIndex = 0;

        dialog.title = qsTr("Add User");
        dialog.open();
    }

    function editContact(contact) {
        form.username.text = contact.username;
        form.password.text = contact.password;
        form.winlogin.checked = contact.winlogin;
        form.sendEnter.checked = contact.sendEnter;
        form.autoFinger.checked = contact.autoFinger;
        form.fingerprintIndex.currentIndex = contact.fingerprintIndex + 1
        form.loginType.currentIndex = contact.loginType

        dialog.title = qsTr("Edit User");
        dialog.open();
    }

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
                sendEnter: form.sendEnter.checked,
                autoFinger: form.autoFinger.checked,
                fingerprintIndex: form.fingerprintIndex.currentValue,
                loginType: form.loginType.currentIndex
            });
        }
    }
}
