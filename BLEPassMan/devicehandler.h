// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef DEVICEHANDLER_H
#define DEVICEHANDLER_H

#include "bluetoothbaseclass.h"

#include <QLowEnergyController>
#include <QLowEnergyService>

#include <QDateTime>
#include <QList>
#include <QMap>
#include <QTimer>
#include <QQmlEngine>

struct UserEntry {
    QString username;
    QString password;
};


class DeviceInfo;

class DeviceHandler : public BluetoothBaseClass
{
    Q_OBJECT

    // Q_PROPERTY(bool measuring READ measuring NOTIFY measuringChanged)
    // Q_PROPERTY(int hr READ hr NOTIFY statsChanged)
    // Q_PROPERTY(int maxHR READ maxHR NOTIFY statsChanged)
    // Q_PROPERTY(int minHR READ minHR NOTIFY statsChanged)
    // Q_PROPERTY(float average READ average NOTIFY statsChanged)
    // Q_PROPERTY(int time READ time NOTIFY statsChanged)
    // Q_PROPERTY(float calories READ calories NOTIFY statsChanged)

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

signals:
    void aliveChanged();
    Q_SIGNAL void userListUpdated(QVariantList list);


public slots:
    void getUserList();
    void disconnectService();

private:
    //QLowEnergyController
    void serviceDiscovered(const QBluetoothUuid &);
    void serviceScanDone();

    //QLowEnergyService
    void serviceStateChanged(QLowEnergyService::ServiceState s);
    void updateCharacteristicValue(const QLowEnergyCharacteristic &c, const QByteArray &value);
    void confirmedDescriptorWrite(const QLowEnergyDescriptor &d, const QByteArray &value);
    void writeCustomCharacteristic(const QByteArray &data);

    void requestNextUser(quint8 cmd);

private:
    bool m_foundService = false;

    QLowEnergyController *m_control = nullptr;
    QLowEnergyService *m_service = nullptr;
    QLowEnergyDescriptor m_notificationDesc;
    DeviceInfo *m_currentDevice = nullptr;
    QLowEnergyController::RemoteAddressType m_addressType = QLowEnergyController::PublicAddress;

    QBluetoothUuid m_customService;
    QBluetoothUuid m_customCharacteristic;

    QMap<int, UserEntry> m_userList;
    bool m_readingUsers = false;
    int m_currentUserIndex = 0;

};

#endif // DEVICEHANDLER_H
