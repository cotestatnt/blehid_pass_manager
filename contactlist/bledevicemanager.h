#pragma once

#include <QObject>
#include <QBluetoothDeviceDiscoveryAgent>
#include <QBluetoothDeviceInfo>
#include <QLowEnergyController>
#include <QAbstractListModel>

struct BleDevice {
    QBluetoothDeviceInfo info;
};

class BleDeviceModel : public QAbstractListModel {
    Q_OBJECT
public:
    enum Roles {
        NameRole = Qt::UserRole + 1,
        AddressRole
    };

    explicit BleDeviceModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE void startScan();
    Q_INVOKABLE void connectToDevice(int index);

signals:
    void connected();

private slots:
    void deviceDiscovered(const QBluetoothDeviceInfo &info);
    void scanFinished();

private:
    QList<BleDevice> m_devices;
    QBluetoothDeviceDiscoveryAgent *m_discoveryAgent;
    QLowEnergyController *m_controller;
};
