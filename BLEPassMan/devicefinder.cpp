// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "devicefinder.h"
#include "devicehandler.h"
#include "deviceinfo.h"
#include <QBluetoothDeviceInfo>

#if QT_CONFIG(permissions)
#include <QCoreApplication>
#include <QPermissions>
#endif

DeviceFinder::DeviceFinder(DeviceHandler *handler, QObject *parent):
    BluetoothBaseClass(parent),
    m_deviceHandler(handler)
{
    m_deviceDiscoveryAgent = new QBluetoothDeviceDiscoveryAgent(this);
    m_deviceDiscoveryAgent->setLowEnergyDiscoveryTimeout(15000);
    connect(m_deviceDiscoveryAgent, &QBluetoothDeviceDiscoveryAgent::deviceDiscovered, this, &DeviceFinder::addDevice);
    connect(m_deviceDiscoveryAgent, &QBluetoothDeviceDiscoveryAgent::errorOccurred, this, &DeviceFinder::scanError);
    connect(m_deviceDiscoveryAgent, &QBluetoothDeviceDiscoveryAgent::finished, this, &DeviceFinder::scanFinished);
    connect(m_deviceDiscoveryAgent, &QBluetoothDeviceDiscoveryAgent::canceled, this, &DeviceFinder::scanFinished);
    resetMessages();
}

DeviceFinder::~DeviceFinder()
{
    qDeleteAll(m_devices);
    m_devices.clear();
}

void DeviceFinder::startSearch()
{
#if QT_CONFIG(permissions)
    //! [permissions]
    QBluetoothPermission permission{};
    permission.setCommunicationModes(QBluetoothPermission::Access);
    switch (qApp->checkPermission(permission)) {
    case Qt::PermissionStatus::Undetermined:
        qApp->requestPermission(permission, this, &DeviceFinder::startSearch);
        return;
    case Qt::PermissionStatus::Denied:
        setError(tr("Bluetooth permissions not granted!"));
        setIcon(IconError);
        return;
    case Qt::PermissionStatus::Granted:
        break; // proceed to search
    }
    //! [permissions]
#endif // QT_CONFIG(permissions)
    clearMessages();
    m_deviceHandler->setDevice(nullptr);
    qDeleteAll(m_devices);
    m_devices.clear();

    emit devicesChanged();
    m_deviceDiscoveryAgent->start(QBluetoothDeviceDiscoveryAgent::LowEnergyMethod);

    emit scanningChanged();
    setInfo(tr("Scanning for devices..."));
    setIcon(IconProgress);
}

void DeviceFinder::addDevice(const QBluetoothDeviceInfo &device)
{
    // If device is LowEnergy-device, add it to the list
    if (device.coreConfigurations() & QBluetoothDeviceInfo::LowEnergyCoreConfiguration) {
        auto devInfo = new DeviceInfo(device);
        auto it = std::find_if(m_devices.begin(), m_devices.end(),
                               [devInfo](DeviceInfo *dev) {
                                   return devInfo->getAddress() == dev->getAddress();
                               });
        if (it == m_devices.end()) {
            m_devices.append(devInfo);
        } else {
            auto oldDev = *it;
            *it = devInfo;
            delete oldDev;
        }
        setInfo(tr("Low Energy device found. Scanning more..."));
        setIcon(IconProgress);
        emit devicesChanged();
    }
}


void DeviceFinder::scanError(QBluetoothDeviceDiscoveryAgent::Error error)
{
    if (error == QBluetoothDeviceDiscoveryAgent::PoweredOffError)
        setError(tr("The Bluetooth adaptor is powered off."));
    else if (error == QBluetoothDeviceDiscoveryAgent::InputOutputError)
        setError(tr("Writing or reading from the device resulted in an error."));
    else
        setError(tr("An unknown error has occurred."));
    setIcon(IconError);
}

void DeviceFinder::scanFinished()
{
    if (m_devices.isEmpty()) {
        setError(tr("No Low Energy devices found."));
        setIcon(IconError);
    } else {
        setInfo(tr("Scanning done."));
        setIcon(IconBluetooth);
    }
    emit scanningChanged();
    emit devicesChanged();
}

void DeviceFinder::resetMessages()
{
    setError("");
    setInfo(tr("Start search to find devices"));
    setIcon(IconSearch);
}

void DeviceFinder::connectToService(const QString &address)
{
    m_deviceDiscoveryAgent->stop();

    DeviceInfo *currentDevice = nullptr;
    for (QObject *entry : std::as_const(m_devices)) {
        auto device = qobject_cast<DeviceInfo *>(entry);
        if (device && device->getAddress() == address) {
            currentDevice = device;
            break;
        }
    }

    if (currentDevice)
        m_deviceHandler->setDevice(currentDevice);

    resetMessages();
}

bool DeviceFinder::scanning() const
{
    return m_deviceDiscoveryAgent->isActive();
}

QVariant DeviceFinder::devices()
{
    return QVariant::fromValue(m_devices);
}
