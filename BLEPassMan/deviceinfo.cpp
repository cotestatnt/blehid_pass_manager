// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "deviceinfo.h"
#include <QBluetoothAddress>
#include <QBluetoothUuid>

using namespace Qt::StringLiterals;

DeviceInfo::DeviceInfo(const QBluetoothDeviceInfo &info):
    m_device(info)
{
}

QBluetoothDeviceInfo DeviceInfo::getDevice() const
{
    return m_device;
}

QString DeviceInfo::getName() const
{
    return m_device.name();
}

QString DeviceInfo::getAddress() const
{
#ifdef Q_OS_DARWIN
    // workaround for Core Bluetooth:
    return m_device.deviceUuid().toString();
#else
    return m_device.address().toString();
#endif
}

void DeviceInfo::setDevice(const QBluetoothDeviceInfo &device)
{
    m_device = device;
    emit deviceChanged();
}
