// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "connectionhandler.h"
#include <qtbluetooth-config.h>

#include <QtSystemDetection>

#if QT_CONFIG(permissions)
#include <QCoreApplication>
#include <QPermissions>
#endif

ConnectionHandler::ConnectionHandler(QObject *parent) : QObject(parent)
{
    initLocalDevice();
}

bool ConnectionHandler::alive() const
{

#ifdef QT_PLATFORM_UIKIT
    return true;
#else
    return m_localDevice && m_localDevice->isValid()
            && m_localDevice->hostMode() != QBluetoothLocalDevice::HostPoweredOff;
#endif
}

bool ConnectionHandler::hasPermission() const
{
    return m_hasPermission;
}

bool ConnectionHandler::requiresAddressType() const
{
#if QT_CONFIG(bluez)
    return true;
#else
    return false;
#endif
}

QString ConnectionHandler::name() const
{
    return m_localDevice ? m_localDevice->name() : QString();
}

QString ConnectionHandler::address() const
{
    return m_localDevice ? m_localDevice->address().toString() : QString();
}

void ConnectionHandler::hostModeChanged(QBluetoothLocalDevice::HostMode /*mode*/)
{
    emit deviceChanged();
}

void ConnectionHandler::initLocalDevice()
{
#if QT_CONFIG(permissions)
    QBluetoothPermission permission{};
    permission.setCommunicationModes(QBluetoothPermission::Access);
    Qt::PermissionStatus status = qApp->checkPermission(permission);
    qDebug() << "Permission status:" << static_cast<int>(status);

    switch (status) {
    case Qt::PermissionStatus::Undetermined:
        qDebug() << "Requesting Bluetooth permission...";
        qApp->requestPermission(permission, this, &ConnectionHandler::initLocalDevice);
        return;
    case Qt::PermissionStatus::Denied:
        qDebug() << "Bluetooth permission denied!";
        return;
    case Qt::PermissionStatus::Granted:
        qDebug() << "Bluetooth permission granted";
        break;
    }
#endif

    // Verifica tutti i dispositivi disponibili
    QList<QBluetoothHostInfo> devices = QBluetoothLocalDevice::allDevices();
    qDebug() << "Available Bluetooth devices:" << devices.size();

    for (const auto& device : std::as_const(devices)) {
        qDebug() << "Device:" << device.name() << device.address().toString();
    }

    m_localDevice = new QBluetoothLocalDevice(this);
    qDebug() << "Local device valid:" << m_localDevice->isValid();
    qDebug() << "Host mode:" << m_localDevice->hostMode();
    qDebug() << "Address:" << m_localDevice->address().toString();

    if (m_localDevice->isValid()) {
        QBluetoothLocalDevice::HostMode currentMode = m_localDevice->hostMode();
        qDebug() << "Current host mode:" << static_cast<int>(currentMode);

        if (currentMode == QBluetoothLocalDevice::HostPoweredOff) {
            qDebug() << "Attempting to power on Bluetooth...";
            m_localDevice->powerOn();
        }
    } else {
        qDebug() << "Local device is not valid!";
    }

    connect(m_localDevice, &QBluetoothLocalDevice::hostModeStateChanged,
            this, &ConnectionHandler::hostModeChanged);
    m_hasPermission = true;
    emit deviceChanged();
}
