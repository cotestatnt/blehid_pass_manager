#include <QBluetoothPermission>
#include "bledevicemanager.h"
#include <QDebug>

BleDeviceModel::BleDeviceModel(QObject *parent)
    : QAbstractListModel(parent),
    m_discoveryAgent(new QBluetoothDeviceDiscoveryAgent(this)),
    m_controller(nullptr)
{
    connect(m_discoveryAgent, &QBluetoothDeviceDiscoveryAgent::deviceDiscovered,
            this, &BleDeviceModel::deviceDiscovered);
    connect(m_discoveryAgent, &QBluetoothDeviceDiscoveryAgent::finished,
            this, &BleDeviceModel::scanFinished);
}

int BleDeviceModel::rowCount(const QModelIndex &) const {
    return m_devices.count();
}

QVariant BleDeviceModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() >= m_devices.count())
        return {};

    const QBluetoothDeviceInfo &info = m_devices.at(index.row()).info;

    switch (role) {
    case NameRole:
        return info.name();
    case AddressRole:
        return info.address().toString();
    default:
        return {};
    }
}

QHash<int, QByteArray> BleDeviceModel::roleNames() const {
    return {
        {NameRole, "deviceName"},
        {AddressRole, "deviceAddress"}
    };
}

void BleDeviceModel::startScan() {
#if defined(Q_OS_ANDROID)
    QBluetoothPermission permission;
    permission.setCommunicationModes(QBluetoothPermission::Access);

    if (permission.status() != Qt::PermissionStatus::Granted) {
        qDebug() << "[BLE] Requesting Bluetooth permission...";
        qApp->requestPermission(permission, this, [this](const QPermission &perm) {
            if (perm.status() == Qt::PermissionStatus::Granted) {
                qDebug() << "[BLE] Bluetooth permission granted. Starting scan...";
                this->startScan();  // richiama sé stesso ora che il permesso è concesso
            } else {
                qWarning() << "[BLE] Bluetooth permission denied!";
            }
        });
        return;
    }
#endif

    // pulizia e avvio effettivo dello scan BLE
    beginResetModel();
    m_devices.clear();
    endResetModel();

    qDebug() << "[BLE] Starting device discovery...";
    m_discoveryAgent->start();
}

    // avvia lo scan normalmente
    m_devices.clear();
    beginResetModel();
    endResetModel();
    m_discoveryAgent->start();
}

void BleDeviceModel::deviceDiscovered(const QBluetoothDeviceInfo &info) {
    if (!(info.coreConfigurations() & QBluetoothDeviceInfo::LowEnergyCoreConfiguration))
        return;

    beginInsertRows(QModelIndex(), m_devices.size(), m_devices.size());
    m_devices.append({info});
    endInsertRows();

    qDebug() << "Discovered BLE device:" << info.name() << info.address().toString();
}

void BleDeviceModel::scanFinished() {
    qDebug() << "Device scan finished.";
}

void BleDeviceModel::connectToDevice(int index) {
    if (index < 0 || index >= m_devices.count()) return;

    const QBluetoothDeviceInfo &info = m_devices.at(index).info;
    m_controller = QLowEnergyController::createCentral(info, this);

    connect(m_controller, &QLowEnergyController::connected, this, [this]() {
        qDebug() << "Connected to device!";
        emit connected();
    });

    connect(m_controller, &QLowEnergyController::errorOccurred, this, [](QLowEnergyController::Error error) {
        qWarning() << "BLE connection error:" << error;
    });

    m_controller->connectToDevice();
}
