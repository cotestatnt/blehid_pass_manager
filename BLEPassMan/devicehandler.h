// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef DEVICEHANDLER_H
#define DEVICEHANDLER_H

#include "bluetoothbaseclass.h"

#include <QLowEnergyController>
#include <QLowEnergyService>
#include <QSoundEffect>
#include <QDateTime>
#include <QList>
#include <QMap>
#include <QTimer>
#include <QQmlEngine>


#define NOT_AUTHORIZED  0x99
#define GET_USERS_LIST  0xA1
#define ADD_NEW_USER    0xA2
#define EDIT_USER       0xA3
#define REMOVE_USER     0xA4
#define CLEAR_USER_DB   0xA5
#define BLE_MESSAGE     0xAA

#define ENROLL_FINGER   0xB0
#define CLEAR_LIBRARY   0xB2

#define LIST_EMPTY      0xFF

struct UserEntry {
    QString username;
    QString password;
    quint8 fingerprintIndex;
    bool autoFinger;
    bool winlogin;
};


class DeviceInfo;

class DeviceHandler : public BluetoothBaseClass
{
    Q_OBJECT
    Q_PROPERTY(bool alive READ alive NOTIFY aliveChanged)
    Q_PROPERTY(AddressType addressType READ addressType WRITE setAddressType)
    Q_PROPERTY(QVariantList userList READ userList NOTIFY userListUpdated)

    QVariantList userList() const;

    QML_ELEMENT

public:
    enum class AddressType { PublicAddress, RandomAddress};
    Q_ENUM(AddressType)

    DeviceHandler(QObject *parent = nullptr);

    void setDevice(DeviceInfo *device);
    void setAddressType(AddressType type);
    AddressType addressType() const;

    bool alive() const;
    DeviceInfo *currentDevice() const { return m_currentDevice; }

signals:
    void aliveChanged();
    Q_SIGNAL void userListUpdated(QVariantList list);
    Q_SIGNAL void serviceReady();


public slots:
    void getUserList();
    void disconnectService();

    Q_INVOKABLE void addUser(const QVariantMap &user);
    Q_INVOKABLE void editUser(int index, const QVariantMap &user);
    Q_INVOKABLE void removeUser(int index);
    Q_INVOKABLE void clearUserDB();

    void getUserFromDevice(int index);

    void enrollFingerprint();
    void clearFingerprintDB();

private:
    //QLowEnergyController
    void serviceDiscovered(const QBluetoothUuid &);
    void serviceScanDone();

    //QLowEnergyService
    void serviceStateChanged(QLowEnergyService::ServiceState s);
    void updateCharacteristicValue(const QLowEnergyCharacteristic &c, const QByteArray &value);
    void confirmedDescriptorWrite(const QLowEnergyDescriptor &d, const QByteArray &value);
    void writeCustomCharacteristic(const QByteArray &data);

    UserEntry parseUserEntry(const QByteArray &data);

private:
    bool m_foundService = false;

    QSoundEffect m_soundEffect;
    QLowEnergyController *m_control = nullptr;
    QLowEnergyService *m_service = nullptr;
    QLowEnergyDescriptor m_notificationDesc;
    DeviceInfo *m_currentDevice = nullptr;
    QLowEnergyController::RemoteAddressType m_addressType = QLowEnergyController::PublicAddress;

    QBluetoothUuid m_customService;
    QBluetoothUuid m_customCharacteristic;

    QMap<int, UserEntry> m_userList;    
    int m_currentUserIndex = 0;

};

#endif // DEVICEHANDLER_H
