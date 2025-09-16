// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "devicehandler.h"
#include "deviceinfo.h"

#include <QtEndian>
#include <QRandomGenerator>
#include <cstring>

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

    if (m_control) {
        m_control->disconnectFromDevice();
        delete m_control;
        m_control = nullptr;
    }

    if (m_currentDevice) {
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
        m_control->connectToDevice();
        qDebug() << "[BLE] MTU negoziato:" << m_control->mtu();
    }
}

void DeviceHandler::serviceDiscovered(const QBluetoothUuid &gatt)
{
    qDebug() << "Service discovered" << gatt;
    if (gatt == m_customService) {
        setInfo("Service discovered. Waiting for service scan to be done...");
        setIcon(IconProgress);
        m_foundService = true;
    }
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

void DeviceHandler::serviceStateChanged(QLowEnergyService::ServiceState s)
{
    switch (s) {
    case QLowEnergyService::RemoteServiceDiscovering:
        setInfo(tr("Discovering services..."));
        setIcon(IconProgress);
        break;
    case QLowEnergyService::RemoteServiceDiscovered: {
        setInfo(tr("Service discovered."));
        setIcon(IconBluetooth);

        const QLowEnergyCharacteristic hrChar = m_service->characteristic(m_customCharacteristic);
        if (!hrChar.isValid()) {
            setError("Custom characteristic not found.");
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
        break;
    }
    emit aliveChanged();
}

bool DeviceHandler::alive() const
{
    return m_service && m_service->state() == QLowEnergyService::RemoteServiceDiscovered;
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
        delete m_batteryService;
        m_batteryService = nullptr;
    }
}

QVariantList DeviceHandler::userList() const
{
    QVariantList list;
    for (auto it = m_userList.constBegin(); it != m_userList.constEnd(); ++it) {
        const UserEntry &ue = it.value();
        QVariantMap entry;
        entry["username"] = ue.username;
        entry["password"] = ue.password; // testuale con placeholder
        entry["winlogin"] = ue.winlogin;
        entry["sendEnter"] = ue.sendEnter;
        entry["autoFinger"] = ue.autoFinger;
        entry["fingerprintIndex"] = ue.fingerprintIndex;
        entry["loginType"] = ue.loginType;
        list.append(entry);
    }
    return list;
}

static QByteArray padOrTruncate(const QByteArray &src, int len)
{
    QByteArray out = src.left(len);
    while (out.size() < len) out.append('\0');
    return out;
}

QByteArray DeviceHandler::buildUserPayload(quint8 cmd, quint8 index, const UserEntry &entry)
{
    QByteArray usernameBytes = padOrTruncate(entry.username.toUtf8(), MAX_LABEL_LEN);
    QByteArray passwordBytes = padOrTruncate(entry.rawPassword, MAX_PASSWORD_LEN);

    QByteArray data;
    data.append(char(cmd));
    data.append(char(index));
    data.append(usernameBytes);
    data.append(passwordBytes);
    data.append(char(entry.winlogin ? 1 : 0));
    data.append(char(entry.sendEnter ? 1 : 0));
    data.append(char(entry.autoFinger ? 1 : 0));
    data.append(char(entry.fingerprintIndex));
    data.append(char(entry.loginType));
    return data;
}

void DeviceHandler::addUser(const QVariantMap &user)
{
    int newIndex = m_userList.isEmpty() ? 0 : m_userList.lastKey() + 1;
    qDebug() << "BACKEND: Aggiungo utente all'indice" << newIndex;

    UserEntry entry;
    entry.username        = user.value("username").toString();
    entry.password        = user.value("password").toString(); // testuale
    entry.winlogin        = user.value("winlogin").toBool();
    entry.sendEnter       = user.value("sendEnter").toBool();
    entry.autoFinger      = user.value("autoFinger").toBool();
    entry.fingerprintIndex= user.value("fingerprintIndex").toInt();
    entry.loginType       = user.value("loginType").toInt();

    QString encErr;
    entry.rawPassword = PlaceholderEncoder::encode(entry.password, &encErr);
    if (!encErr.isEmpty()) {
        setError("Placeholder error: " + encErr);
        setIcon(IconError);
        return;
    }

    QByteArray payload = buildUserPayload(ADD_NEW_USER, quint8(newIndex), entry);
    writeCustomCharacteristic(payload);

    m_userList[newIndex] = entry;
    emit userListUpdated(userList());
}

void DeviceHandler::editUser(int index, const QVariantMap &user)
{
    qDebug() << "BACKEND: Modifico utente all'indice" << index;
    if (!m_userList.contains(index))
        return;

    UserEntry entry;
    entry.username         = user.value("username").toString();
    entry.password         = user.value("password").toString();
    entry.winlogin         = user.value("winlogin").toBool();
    entry.sendEnter        = user.value("sendEnter").toBool();
    entry.autoFinger       = user.value("autoFinger").toBool();
    entry.fingerprintIndex = user.value("fingerprintIndex").toInt();
    entry.loginType        = user.value("loginType").toInt();

    QString encErr;
    entry.rawPassword = PlaceholderEncoder::encode(entry.password, &encErr);
    if (!encErr.isEmpty()) {
        setError("Placeholder error: " + encErr);
        setIcon(IconError);
        return;
    }

    QByteArray payload = buildUserPayload(EDIT_USER, quint8(index), entry);
    writeCustomCharacteristic(payload);

    m_userList[index] = entry;
    emit userListUpdated(userList());
}

void DeviceHandler::getUserFromDevice(int index)
{
    QByteArray data;
    data.append(char(GET_USERS_LIST));
    data.append(char(index));
    writeCustomCharacteristic(data);
}

void DeviceHandler::removeUser(int index)
{
    qDebug() << "BACKEND: Rimuovo utente all'indice" << index;
    if (!m_userList.contains(index))
        return;

    QByteArray data;
    data.append(char(REMOVE_USER));
    data.append(char(index));
    writeCustomCharacteristic(data);
    m_userList.remove(index);
    getUserList();
}

void DeviceHandler::clearUserDB()
{
    qDebug() << "BACKEND: Rimuovo lista utenti completa";
    QByteArray data;
    data.append(char(CLEAR_USER_DB));
    data.append(char(0xFF));
    writeCustomCharacteristic(data);
    getUserList();
}

void DeviceHandler::enrollFingerprint()
{
    setInfo("Follow instructions on devices's display");
    setIcon(IconProgress);
    QByteArray data;
    data.append(char(ENROLL_FINGER));
    writeCustomCharacteristic(data);
}

void DeviceHandler::clearFingerprintDB()
{
    QByteArray data;
    data.append(char(CLEAR_LIBRARY));
    writeCustomCharacteristic(data);
}

void DeviceHandler::getUserList()
{
    m_userList.clear();
    getUserFromDevice(0);
}

UserEntry DeviceHandler::parseUserEntry(const QByteArray &data)
{
    UserEntry entry;

    if (data.size() < 2) {
        qWarning() << "Invalid data size for user entry parsing";
        return entry;
    }

    int offset = 0;
    uint8_t cmd = quint8(data[offset++]);
    if (cmd != GET_USERS_LIST) {
        qWarning() << "Invalid command byte for user entry:" << QString::number(cmd, 16);
        return entry;
    }

    int index = quint8(data[offset++]);
    Q_UNUSED(index);

    if (data.size() < offset + MAX_LABEL_LEN + MAX_PASSWORD_LEN + 5) {
        qWarning() << "Insufficient data for user entry fields";
        return entry;
    }

    QByteArray labelBytes = data.mid(offset, MAX_LABEL_LEN);
    offset += MAX_LABEL_LEN;

    // Username: togliere null finali
    size_t labelLen = strnlen(labelBytes.constData(), MAX_LABEL_LEN);
    entry.username = QString::fromUtf8(labelBytes.constData(), labelLen);
    entry.username = entry.username.trimmed();

    QByteArray passwordRaw = data.mid(offset, MAX_PASSWORD_LEN);
    offset += MAX_PASSWORD_LEN;

    // Rimuovo null finali SOLO per la decodifica testuale
    int rawLen = 0;
    for (int i=0; i<passwordRaw.size(); ++i) {
        if (passwordRaw[i] == '\0') break;
        rawLen++;
    }
    QByteArray shrink = passwordRaw.left(rawLen);
    entry.rawPassword = shrink;
    entry.password = PlaceholderEncoder::decode(shrink);

    entry.winlogin         = (data[offset++] == 1);
    entry.sendEnter        = (data[offset++] == 1);
    entry.autoFinger       = (data[offset++] == 1);
    entry.fingerprintIndex = quint8(data[offset++]);
    entry.loginType        = quint8(data[offset++]);

    return entry;
}

void DeviceHandler::batteryServiceStateChanged(QLowEnergyService::ServiceState s)
{
    switch (s) {
    case QLowEnergyService::RemoteServiceDiscovering:
        qDebug() << "Discovering battery service details...";
        break;
    case QLowEnergyService::RemoteServiceDiscovered: {
        qDebug() << "Battery service discovered.";
        const QLowEnergyCharacteristic batteryChar = m_batteryService->characteristic(m_batteryCharacteristic);
        if (!batteryChar.isValid()) {
            qDebug() << "Battery level characteristic not found.";
            break;
        }
        if (batteryChar.properties() & QLowEnergyCharacteristic::Read) {
            m_batteryService->readCharacteristic(batteryChar);
        }
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

void DeviceHandler::updateBatteryLevel(const QLowEnergyCharacteristic &c, const QByteArray &value)
{
    if (c.uuid() == m_batteryCharacteristic && value.length() > 0) {
        int newBatteryLevel = quint8(value.at(0));
        if (m_batteryLevel != newBatteryLevel) {
            m_batteryLevel = newBatteryLevel;
            emit batteryLevelChanged();
            qDebug() << "Battery level updated:" << m_batteryLevel << "%";
        }
    }
}

void DeviceHandler::updateCharacteristicValue(const QLowEnergyCharacteristic &c, const QByteArray &value)
{
    if (c.uuid() != m_customCharacteristic || value.size() < 2)
        return;

    const quint8 cmd = quint8(value[0]);
    const quint8 index = quint8(value[1]);
    const QByteArray remainder = value.mid(2);

    if (remainder.isEmpty() || remainder.at(0) == '\0') {
        QVariantList list = userList();
        qDebug() << "[BLE Notify] Lista utenti completata.";
        emit userListUpdated(list);
        return;
    }

    switch (cmd) {
    case NOT_AUTHORIZED: {
        bool ok = (remainder.at(0) != 0);
        clearMessages();
        if (ok) {
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
        qDebug().nospace() << "[BLE] Authenticated[" << (ok ? "true" : "false") << "]";
        break;
    }
    case GET_USERS_LIST: {
        UserEntry user = parseUserEntry(value);
        m_userList[index] = user;
        qDebug() << "User:" << user.username;
        qDebug() << "Winlogin:" << user.winlogin;
        qDebug() << "SendEnter:" << user.sendEnter;
        qDebug() << "Fingerprint:" << user.fingerprintIndex;
        qDebug() << "Fingerprint automatic:" << user.autoFinger;
        qDebug() << "Login type:" << user.loginType;
        getUserFromDevice(index + 1);
        break;
    }
    case LIST_EMPTY:
        qWarning() << "User list empty";
        setInfo("User list empty, please add new user");
        setIcon(IconSearch);
        break;
    case BLE_MESSAGE: {
        // index: 0=info, 1=error
        QString text = QString::fromUtf8(remainder.constData(),
                                         strnlen(remainder.constData(), remainder.size())).trimmed();
        clearMessages();
        qDebug() << "[" << (index ? "error" : "info") << "] Message:" << text;
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
    }
    case BATTERY_MV: {
        QString text = QString::fromUtf8(remainder.constData(),
                                         strnlen(remainder.constData(), remainder.size())).trimmed();
        qDebug().noquote().nospace() << "Battery " << text << "mV";
        break;
    }
    default:
        qDebug() << "[BLE] Comando sconosciuto:" << cmd;
        break;
    }
}
