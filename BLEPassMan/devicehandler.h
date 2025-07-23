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

struct UserEntry {
    QString username;
    QString password;
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

    void addUser(const QString &username, const QString &password, const bool winlogin);
    void editUser(int index, const QString &username, const QString &password, const bool winlogin);
    void removeUser(int index);

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

    // void requestNextUser(quint8 cmd);

    void requestUser(quint8 idx);
    void requestPassword(quint8 idx);
    void requestWinlogin(quint8 idx);

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
