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


        // // Dopo aver connesso il controller
        // m_control->requestMtu(247); // oppure 512 se vuoi provare MTU massimo

        // Connect
        m_control->connectToDevice();

        qDebug() << "[BLE] MTU negoziato:" << m_control->mtu();
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
        setError("Service not found.");
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
            emit serviceReady();
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


void DeviceHandler::addUser(const QString &username, const QString &password)
{
    int newIndex = m_userList.isEmpty() ? 0 : m_userList.lastKey() + 1;
    qDebug() << "BACKEND: Aggiungo utente all'indice" << newIndex;

    // Pacchetto username
    QByteArray userData;
    userData.append(static_cast<char>(0x01));           // CMD
    userData.append(static_cast<char>(newIndex));       // Indice
    userData.append(static_cast<char>(0x04));           // Sub-CMD: username
    userData.append(username.toUtf8());                 // Username
    writeCustomCharacteristic(userData);

    // Pacchetto password
    QByteArray passData;
    passData.append(static_cast<char>(0x01));           // CMD
    passData.append(static_cast<char>(newIndex));       // Indice
    passData.append(static_cast<char>(0x05));           // Sub-CMD: password
    passData.append(password.toUtf8());                 // Password
    writeCustomCharacteristic(passData);

    // (Opzionale) Aggiorna modello locale
    m_userList[newIndex] = {username, password};
    emit userListUpdated(userList());
}

void DeviceHandler::editUser(int index, const QString &username, const QString &password)
{
    qDebug() << "BACKEND: Modifco utente all'indice" << index;
    if (!m_userList.contains(index))
        return;

    // Pacchetto username
    QByteArray userData;
    userData.append(static_cast<char>(0x01));           // CMD
    userData.append(static_cast<char>(index));          // Indice
    userData.append(static_cast<char>(0x04));           // Sub-CMD: username
    userData.append(username.toUtf8());
    writeCustomCharacteristic(userData);

    // Pacchetto password
    QByteArray passData;
    passData.append(static_cast<char>(0x01));           // CMD
    passData.append(static_cast<char>(index));          // Indice
    passData.append(static_cast<char>(0x05));           // Sub-CMD: password
    passData.append(password.toUtf8());
    writeCustomCharacteristic(passData);

    m_userList[index] = {username, password};
    emit userListUpdated(userList());
}



void DeviceHandler::removeUser(int index)
{
    qDebug() << "BACKEND: Rimuovo utente all'indice" << index;
    if (!m_userList.contains(index))
        return;

    QByteArray data;
    data.append(static_cast<char>(0x03));           // CMD Remove
    data.append(static_cast<char>(index));          // Indice
    writeCustomCharacteristic(data);
    m_userList.remove(index);

    // Chiedi la lista aggiornata al device
    getUserList();
}

void DeviceHandler::requestUser(quint8 idx) {
    QByteArray data;
    data.append(0x04);
    data.append(idx);
    writeCustomCharacteristic(data);
}

void DeviceHandler::requestPassword(quint8 idx) {
    QByteArray data;
    data.append(0x05);
    data.append(idx);
    writeCustomCharacteristic(data);
}

void DeviceHandler::enrollFingerprint() {
    setInfo("Follow instructions on devices's display");
    setIcon(IconProgress);

    qDebug() << "Enroll new fingerprint";
    QByteArray data;
    data.append(0x07);
    writeCustomCharacteristic(data);
}

void DeviceHandler::getUserList()
{
    m_userList.clear();  // reset lista

    // emit userListUpdated(list);
    qDebug() << "Start reading users list";    
    requestUser(0);
}


// PARSE CHARACTERISTICS NOTIFIED FROM DEVICE
void DeviceHandler::updateCharacteristicValue(const QLowEnergyCharacteristic &c, const QByteArray &value)
{
    if (c.uuid() != m_customCharacteristic || value.size() < 3)
        return;

    const quint8 cmd = static_cast<quint8>(value[0]);
    const quint8 index = static_cast<quint8>(value[1]);
    const QByteArray data = value.mid(2);

    // qDebug() << value.toHex(' ').toUpper();

    if (data.isEmpty() || data.at(0) == '\0') {
        QVariantList list = userList();
        qDebug() << "[BLE Notify] Lista utenti completata.";
        emit userListUpdated(list);
        return;
    }

    QString text = QString::fromUtf8(data.data(), strnlen(data.data(), data.size())).trimmed();


    switch (cmd) {
        case 0x04: // username
            m_userList[index].username = text;
            qDebug() << "[BLE] Username[" << index << "]:" << text;
            requestPassword(index);
            break;
        case 0x05: // password
            m_userList[index].password = text;
            qDebug() << "[BLE] Password[" << index << "]:" << "***********";
            requestUser(index + 1);
            break;
        case 0x99: // Not authenticate
            qDebug().nospace() << "[BLE] Authenticated[" << (data.at(0) ? "true" : "false") << "]";
            clearMessages();
            if (data.at(0)) {
                setInfo("User Authenticated");
                setIcon(IconSearch);
                m_soundEffect.setSource(QUrl("qrc:/images/info.wav"));
                m_soundEffect.play();
            } else {
                setError("Not authenticated. Put fingerprint on sensor");
                setIcon(IconError);
                m_soundEffect.setSource(QUrl("qrc:/images/pop.wav"));
                m_soundEffect.play();
            }
            break;
        case 0xFF: // List empty

            qWarning() << "User list empty";
            setInfo("User list empty, please add new user");
            setIcon(IconSearch);
            break;
        case 0xAA: // generic message
            clearMessages();
            qDebug() << "[" << (index ? "error" : "info") <<  "] Message: " << text;
            if (index) {
                setError("[BLE] " + text);
                setIcon(IconError);
                m_soundEffect.setSource(QUrl("qrc:/images/pop.wav"));
                m_soundEffect.play();
            } else {
                setInfo("[BLE] " + text);
                setIcon(IconSearch);
                m_soundEffect.setSource(QUrl("qrc:/images/info.wav"));
                m_soundEffect.play();
            }
            break;
        default:
            qDebug() << "[BLE] Comando sconosciuto:" << cmd;
            break;
    }
}


