// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "devicehandler.h"
#include "deviceinfo.h"
#include <QtEndian>
#include <QRandomGenerator>

DeviceHandler::DeviceHandler(QObject *parent) :
    BluetoothBaseClass(parent)
{
    m_customService = QBluetoothUuid(static_cast<quint32>(0xFFF0));
    m_customCharacteristic = QBluetoothUuid(static_cast<quint32>(0xFFF1));
}

void DeviceHandler::setAddressType(AddressType type)
{
    switch (type) {
    case DeviceHandler::AddressType::PublicAddress:
        m_addressType = QLowEnergyController::PublicAddress;
        break;
    case DeviceHandler::AddressType::RandomAddress:
        m_addressType = QLowEnergyController::RandomAddress;
        break;
    }
}

DeviceHandler::AddressType DeviceHandler::addressType() const
{
    if (m_addressType == QLowEnergyController::RandomAddress)
        return DeviceHandler::AddressType::RandomAddress;

    return DeviceHandler::AddressType::PublicAddress;
}

void DeviceHandler::setDevice(DeviceInfo *device)
{
    clearMessages();
    m_currentDevice = device;

    // Disconnect and delete old connection
    if (m_control) {
        m_control->disconnectFromDevice();
        delete m_control;
        m_control = nullptr;
    }

    // Create new controller and connect it if device available
    if (m_currentDevice) {

        // Make connections
        m_control = QLowEnergyController::createCentral(m_currentDevice->getDevice(), this);
        m_control->setRemoteAddressType(m_addressType);
        connect(m_control, &QLowEnergyController::serviceDiscovered, this, &DeviceHandler::serviceDiscovered);
        connect(m_control, &QLowEnergyController::discoveryFinished, this, &DeviceHandler::serviceScanDone);
        connect(m_control, &QLowEnergyController::errorOccurred, this, [this](QLowEnergyController::Error error) {
                    Q_UNUSED(error);
                    setError("Cannot connect to remote device.");
                    setIcon(IconError);
                });
        connect(m_control, &QLowEnergyController::connected, this, [this]() {
            setInfo("Controller connected. Search services...");
            setIcon(IconProgress);
            m_control->discoverServices();
        });
        connect(m_control, &QLowEnergyController::disconnected, this, [this]() {
            setError("LowEnergy controller disconnected");
            setIcon(IconError);
        });

        // Connect
        m_control->connectToDevice();
    }
}


void DeviceHandler::serviceDiscovered(const QBluetoothUuid &gatt)
{
    qDebug() << "Service discovered" << gatt;

    if (gatt == m_customService) {
        qDebug() << "Waiting for service scan to be done...";
        setInfo("Service discovered. Waiting for service scan to be done...");
        setIcon(IconProgress);
        m_foundService = true;
    }

}

void DeviceHandler::serviceScanDone()
{
    setInfo("Service scan done.");
    setIcon(IconBluetooth);

    // Delete old service if available
    if (m_service) {
        delete m_service;
        m_service = nullptr;
    }

    if (m_foundService)
        m_service = m_control->createServiceObject(m_customService, this);

    if (m_service) {
        connect(m_service, &QLowEnergyService::stateChanged, this, &DeviceHandler::serviceStateChanged);
        connect(m_service, &QLowEnergyService::characteristicChanged, this, &DeviceHandler::updateCharacteristicValue);
        connect(m_service, &QLowEnergyService::descriptorWritten, this, &DeviceHandler::confirmedDescriptorWrite);
        m_service->discoverDetails();
    } else {
        setError("Heart Rate Service not found.");
        setIcon(IconError);
    }
}

// Service functions
void DeviceHandler::serviceStateChanged(QLowEnergyService::ServiceState s)
{
    switch (s) {
    case QLowEnergyService::RemoteServiceDiscovering:
        setInfo(tr("Discovering services..."));
        setIcon(IconProgress);
        break;
    case QLowEnergyService::RemoteServiceDiscovered:
    {
        setInfo(tr("Service discovered."));
        setIcon(IconBluetooth);

        const QLowEnergyCharacteristic hrChar = m_service->characteristic(m_customCharacteristic);
        if (!hrChar.isValid()) {
            setError("Custom characteristic not found.");
            qDebug() << "Custom characteristic not found.";
            setIcon(IconError);
            break;
        }

        m_notificationDesc = hrChar.descriptor(QBluetoothUuid::DescriptorType::ClientCharacteristicConfiguration);
        if (m_notificationDesc.isValid()) {
            m_service->writeDescriptor(m_notificationDesc, QByteArray::fromHex("0100"));
            qDebug() << "Custom characteristic notify subscribed";
        }

        break;
    }
    default:
        //nothing for now
        break;
    }

    emit aliveChanged();
}


void DeviceHandler::writeCustomCharacteristic(const QByteArray &data)
{
    if (!m_service) {
        qWarning() << "Cannot write: Service or characteristic invalid";
        return;
    }

    QLowEnergyCharacteristic ch = m_service->characteristic(m_customCharacteristic);
    m_service->writeCharacteristic(ch, data, QLowEnergyService::WriteWithResponse);
}


void DeviceHandler::confirmedDescriptorWrite(const QLowEnergyDescriptor &d, const QByteArray &value)
{
    if (d.isValid() && d == m_notificationDesc && value == QByteArray::fromHex("0000")) {
        //disabled notifications -> assume disconnect intent
        m_control->disconnectFromDevice();
        delete m_service;
        m_service = nullptr;
    }
}

void DeviceHandler::disconnectService()
{
    m_foundService = false;

    //disable notifications
    if (m_notificationDesc.isValid() && m_service
            && m_notificationDesc.value() == QByteArray::fromHex("0100")) {
        m_service->writeDescriptor(m_notificationDesc, QByteArray::fromHex("0000"));
    } else {
        if (m_control)
            m_control->disconnectFromDevice();

        delete m_service;
        m_service = nullptr;
    }
}

bool DeviceHandler::alive() const
{
    if (m_service)
        return m_service->state() == QLowEnergyService::RemoteServiceDiscovered;

    return false;
}

QVariantList DeviceHandler::userList() const
{
    QVariantList list;
    for (auto it = m_userList.constBegin(); it != m_userList.constEnd(); ++it) {
        QVariantMap entry;
        entry["index"] = it.key();
        entry["username"] = it.value().username;
        entry["password"] = it.value().password;
        list.append(entry);
    }
    return list;
}


void DeviceHandler::getUserList()
{
    m_userList.clear();  // reset lista
    QVariantList list;

    emit userListUpdated(list);
    qDebug() << "Start reading users list";
    requestNextUser(0x04);
}


void DeviceHandler::requestNextUser(quint8 cmd)
{
    m_readingUsers = true;
    QByteArray data;
    data.append(cmd);
    writeCustomCharacteristic(data);
}

void DeviceHandler::updateCharacteristicValue(const QLowEnergyCharacteristic &c, const QByteArray &value)
{
    if (c.uuid() != m_customCharacteristic || value.size() < 3)
        return;

    const quint8 cmd = static_cast<quint8>(value[0]);
    const quint8 index = static_cast<quint8>(value[1]);
    const QByteArray data = value.mid(2);

    if (data.isEmpty() || data.at(0) == '\0') {
        QVariantList list = userList();
        qDebug() << "[BLE Notify] Lista utenti completata.";
        emit userListUpdated(list);
        return;
    }

    QString text = QString::fromUtf8(data).trimmed();

    switch (cmd) {
    case 0x04: // username
        m_userList[index].username = text;
        qDebug() << "[BLE] Username[" << index << "]:" << text;
        break;
    case 0x05: // password
        m_userList[index].password = text;
        qDebug() << "[BLE] Password[" << index << "]:" << text;
        requestNextUser(0x04);
        break;
    default:
        qDebug() << "[BLE] Comando sconosciuto:" << cmd;
        break;
    }
}


