// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "devicehandler.h"
#include "deviceinfo.h"


#include <QtEndian>
#include <QRandomGenerator>

DeviceHandler::DeviceHandler(QObject *parent) :
    BluetoothBaseClass(parent),
    m_foundBatteryService(false),
    m_batteryLevel(-1),
    m_control(nullptr),
    m_service(nullptr),
    m_batteryService(nullptr),
    m_currentDevice(nullptr)
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

    // Rileva Battery Service
    if (gatt == m_batteryService_uuid) {
        setInfo("Battery service found...");
        m_foundBatteryService = true;
        qDebug() << "Battery service discovered";
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

    // Gestisci Battery Service
    if (m_foundBatteryService) {
        m_batteryService = m_control->createServiceObject(m_batteryService_uuid, this);
        if (m_batteryService) {
            connect(m_batteryService, &QLowEnergyService::stateChanged, this, &DeviceHandler::serviceStateChanged);
            connect(m_batteryService, &QLowEnergyService::characteristicChanged, this, &DeviceHandler::updateBatteryLevel);
            m_batteryService->discoverDetails();
            qDebug() << "Battery service object created and discovery started";
        }
    }

    if (!m_foundService && !m_foundBatteryService) {
        setError("No relevant services found.");
        return;
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

bool DeviceHandler::alive() const
{
    if (m_service)
        return m_service->state() == QLowEnergyService::RemoteServiceDiscovered;

    return false;
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
    m_foundBatteryService = false;
    m_batteryLevel = -1;
    emit batteryLevelChanged();

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

    if (m_batteryNotificationDesc.isValid() && m_batteryService
        && m_batteryNotificationDesc.value() == QByteArray::fromHex("0100")) {
        m_batteryService->writeDescriptor(m_batteryNotificationDesc, QByteArray::fromHex("0000"));
    } else {
        if (m_control)
            m_control->disconnectFromDevice();

        delete m_service;
        m_service = nullptr;

        // NUOVO
        delete m_batteryService;
        m_batteryService = nullptr;
    }
}


QVariantList DeviceHandler::userList() const
{
    QVariantList list;
    for (auto it = m_userList.constBegin(); it != m_userList.constEnd(); ++it) {
        QVariantMap entry;
        entry["username"] = it.value().username;
        entry["password"] = it.value().password;
        entry["winlogin"] = it.value().winlogin;
        entry["autoFinger"] = it.value().autoFinger;
        entry["fingerprintIndex"] = it.value().fingerprintIndex;
        entry["loginType"] = it.value().loginType;
        list.append(entry);
    }
    return list;
}



void DeviceHandler::addUser(const QVariantMap &user)
{
    int newIndex = m_userList.isEmpty() ? 0 : m_userList.lastKey() + 1;
    qDebug() << "BACKEND: Aggiungo utente all'indice" << newIndex;

    UserEntry entry;
    entry.username = user["username"].toString();
    entry.password = user["password"].toString();
    entry.winlogin = user["winlogin"].toBool();
    entry.autoFinger = user["autoFinger"].toBool();
    entry.fingerprintIndex = user["fingerprintIndex"].toInt();
    entry.loginType = user["loginType"].toInt();

    qDebug() << "User:" << entry.username;
    // qDebug() << "Pass:" << entry.password;
    qDebug() << "Winlogin:" << entry.winlogin;
    qDebug() << "Fingerprint:" << entry.fingerprintIndex;
    qDebug() << "Fingerprint automatic:" << entry.autoFinger;
    qDebug() << "Login type:" << entry.loginType;

    // Converti username e password in array di lunghezza fissa (32 byte)
    QByteArray usernameBytes = entry.username.toUtf8();
    QByteArray passwordBytes = entry.password.toUtf8();

    // Tronca o riempi con null bytes per raggiungere esattamente 32 byte
    usernameBytes = usernameBytes.left(32);  // Tronca se più lungo di 32
    passwordBytes = passwordBytes.left(32);  // Tronca se più lungo di 32
    // Riempi con 0x00 fino a raggiungere 32 byte
    while (usernameBytes.size() < 32) usernameBytes.append('\0');
    while (passwordBytes.size() < 32) passwordBytes.append('\0');

    // Pacchetto username
    QByteArray data;
    data.append(static_cast<char>(ADD_NEW_USER));
    data.append(static_cast<char>(newIndex));
    data.append(usernameBytes);  // 32 byte fissi
    data.append(passwordBytes);  // 32 byte fissi
    data.append(entry.winlogin);
    data.append(entry.autoFinger);
    data.append(entry.fingerprintIndex);
    data.append(entry.loginType);
    writeCustomCharacteristic(data);
    m_userList[newIndex] = entry;
    emit userListUpdated(userList());
}

void DeviceHandler::editUser(int index, const QVariantMap &user)
{
    qDebug() << "BACKEND: Modifco utente all'indice" << index;
    if (!m_userList.contains(index))
        return;

    UserEntry entry;
    entry.username = user["username"].toString();
    entry.password = user["password"].toString();
    entry.winlogin = user["winlogin"].toBool();
    entry.autoFinger = user["autoFinger"].toBool();
    entry.fingerprintIndex = user["fingerprintIndex"].toInt();
    entry.loginType = user["loginType"].toInt();

    qDebug() << "User:" << entry.username;
    // qDebug() << "Pass:" << entry.password;
    qDebug() << "Winlogin:" << entry.winlogin;
    qDebug() << "Fingerprint:" << entry.fingerprintIndex;
    qDebug() << "Fingerprint automatic:" << entry.autoFinger;
    qDebug() << "Login type:" << entry.loginType;

    // Converti username e password in array di lunghezza fissa (32 byte)
    QByteArray usernameBytes = entry.username.toUtf8();
    QByteArray passwordBytes = entry.password.toUtf8();

    // Tronca o riempi con null bytes per raggiungere esattamente 32 byte
    usernameBytes = usernameBytes.left(32);  // Tronca se più lungo di 32
    passwordBytes = passwordBytes.left(32);  // Tronca se più lungo di 32
    // Riempi con 0x00 fino a raggiungere 32 byte
    while (usernameBytes.size() < 32) usernameBytes.append('\0');
    while (passwordBytes.size() < 32) passwordBytes.append('\0');

    // Pacchetto username
    QByteArray data;
    data.append(static_cast<char>(EDIT_USER));
    data.append(static_cast<char>(index));
    data.append(usernameBytes);  // 32 byte fissi
    data.append(passwordBytes);  // 32 byte fissi
    data.append(entry.winlogin);
    data.append(entry.autoFinger);
    data.append(entry.fingerprintIndex);
    data.append(entry.loginType);

    writeCustomCharacteristic(data);
    m_userList[index] = entry;
    emit userListUpdated(userList());
}

void DeviceHandler::getUserFromDevice(int index) {

    qDebug() << "Get complete user list from BLE device";
    QByteArray data;
    data.append(static_cast<char>(GET_USERS_LIST)); // CMD
    data.append(static_cast<char>(index));          // Indice
    writeCustomCharacteristic(data);    
}


void DeviceHandler::removeUser(int index)
{
    qDebug() << "BACKEND: Rimuovo utente all'indice" << index;
    if (!m_userList.contains(index))
        return;

    QByteArray data;
    data.append(static_cast<char>(REMOVE_USER));           // CMD Remove
    data.append(static_cast<char>(index));          // Indice
    writeCustomCharacteristic(data);
    m_userList.remove(index);

    // Chiedi la lista aggiornata al device
    getUserList();
}

void DeviceHandler::clearUserDB() {
    qDebug() << "BACKEND: Rimuovo lista utenti completa";

    QByteArray data;
    data.append(static_cast<char>(CLEAR_USER_DB)); // CMD Remove
    data.append(static_cast<char>(0xFF));          // Indice
    writeCustomCharacteristic(data);

    // Chiedi la lista aggiornata al device
    getUserList();
}

void DeviceHandler::enrollFingerprint() {
    setInfo("Follow instructions on devices's display");
    setIcon(IconProgress);
    qDebug() << "Enroll new fingerprint";
    QByteArray data;
    data.append(static_cast<char>(ENROLL_FINGER));
    writeCustomCharacteristic(data);
}

void DeviceHandler::clearFingerprintDB() {
    qDebug() << "Clear fingerprint DB";
    QByteArray data;
    data.append(static_cast<char>(CLEAR_LIBRARY));
    writeCustomCharacteristic(data);
}

void DeviceHandler::getUserList() {
    m_userList.clear();
    getUserFromDevice(0);
}

UserEntry DeviceHandler::parseUserEntry(const QByteArray &data) {
    UserEntry entry = {0};

    if (data.size() < 2) {
        qWarning() << "Invalid data size for user entry parsing";
        return entry;
    }

    int offset = 0;

    // Skip command byte (0xA1)
    uint8_t cmd = static_cast<uint8_t>(data[offset++]);
    if (cmd != GET_USERS_LIST) {
        qWarning() << "Invalid command byte for user entry:" << QString::number(cmd, 16);
        return entry;
    }

    // Parse index
    int index = static_cast<uint8_t>(data[offset++]);
    Q_UNUSED(index);

    // Parse label (MAX_LABEL_LEN bytes)
    constexpr int MAX_LABEL_LEN = 32; // Adjust based on your #define
    if (data.size() < offset + MAX_LABEL_LEN) {
        qWarning() << "Insufficient data for label parsing";
        return entry;
    }

    QByteArray labelBytes = data.mid(offset, MAX_LABEL_LEN);
    entry.username = QString::fromUtf8(labelBytes.data(), strnlen(labelBytes.data(), labelBytes.size())).trimmed(); // Remove null terminators
    offset += MAX_LABEL_LEN;

    // Parse password (MAX_PASSWORD_LEN bytes)
    constexpr int MAX_PASSWORD_LEN = 32; // Adjust based on your #define
    if (data.size() < offset + MAX_PASSWORD_LEN) {
        qWarning() << "Insufficient data for password parsing";
        return entry;
    }

    QByteArray passwordBytes = data.mid(offset, MAX_PASSWORD_LEN);
    entry.password = QString::fromUtf8(passwordBytes.data(), strnlen(passwordBytes.data(), passwordBytes.size())).trimmed();
    offset += MAX_PASSWORD_LEN;

    // Parse boolean flags
    if (data.size() < offset + 3) {
        qWarning() << "Insufficient data for flags parsing";
        return entry;
    }

    entry.winlogin = (data[offset++] == 1);
    entry.autoFinger = (data[offset++] == 1);
    entry.fingerprintIndex = static_cast<uint8_t>(data[offset++]);
    entry.loginType = static_cast<uint8_t>(data[offset++]);
    return entry;
}


// Gestione stato Battery Service
void DeviceHandler::batteryServiceStateChanged(QLowEnergyService::ServiceState s)
{
    switch (s) {
    case QLowEnergyService::RemoteServiceDiscovering:
        qDebug() << "Discovering battery service details...";
        break;
    case QLowEnergyService::RemoteServiceDiscovered:
    {
        qDebug() << "Battery service discovered.";

        const QLowEnergyCharacteristic batteryChar = m_batteryService->characteristic(m_batteryCharacteristic);
        if (!batteryChar.isValid()) {
            qDebug() << "Battery level characteristic not found.";
            break;
        }

        // Leggi il valore iniziale della batteria
        if (batteryChar.properties() & QLowEnergyCharacteristic::Read) {
            m_batteryService->readCharacteristic(batteryChar);
        }

        // Abilita le notifiche se supportate
        if (batteryChar.properties() & QLowEnergyCharacteristic::Notify) {
            m_batteryNotificationDesc = batteryChar.descriptor(QBluetoothUuid::DescriptorType::ClientCharacteristicConfiguration);
            if (m_batteryNotificationDesc.isValid()) {
                m_batteryService->writeDescriptor(m_batteryNotificationDesc, QByteArray::fromHex("0100"));
                qDebug() << "Battery level notifications enabled";
            }
        }

        break;
    }
    default:
        break;
    }
}

// Aggiorna livello batteria
void DeviceHandler::updateBatteryLevel(const QLowEnergyCharacteristic &c, const QByteArray &value)
{
    if (c.uuid() == m_batteryCharacteristic) {
        if (value.length() > 0) {
            int newBatteryLevel = static_cast<uint8_t>(value.at(0));
            if (m_batteryLevel != newBatteryLevel) {
                m_batteryLevel = newBatteryLevel;
                emit batteryLevelChanged();
                qDebug() << "Battery level updated:" << m_batteryLevel << "%";
            }
        }
    }
}


// PARSE CHARACTERISTICS NOTIFIED FROM DEVICE
void DeviceHandler::updateCharacteristicValue(const QLowEnergyCharacteristic &c, const QByteArray &value)
{
    qDebug() << c.uuid();

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
        case NOT_AUTHORIZED: // Not authenticate
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

        case GET_USERS_LIST: {            
            UserEntry user = parseUserEntry(value);
            m_userList[index].username =  user.username;
            m_userList[index].password = user.password;
            m_userList[index].winlogin = user.winlogin;
            m_userList[index].autoFinger = user.autoFinger;
            m_userList[index].fingerprintIndex = user.fingerprintIndex;
            m_userList[index].loginType = user.loginType;

            qDebug() << "User:" << user.username;
            // qDebug() << "Pass:" << user.password;
            qDebug() << "Winlogin:" << user.winlogin;
            qDebug() << "Fingerprint:" << user.fingerprintIndex;
            qDebug() << "Fingerprint automatic:" << user.autoFinger;
            qDebug() << "Login type:" << user.loginType;
            getUserFromDevice(index + 1);
            break;
        }

        case LIST_EMPTY: // List empty
            qWarning() << "User list empty";
            setInfo("User list empty, please add new user");
            setIcon(IconSearch);
            break;

        case BLE_MESSAGE: // generic message
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


