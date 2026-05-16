// =============================================================================
// DeviceStatusModel.h — 设备在线状态 Model
// =============================================================================
#ifndef DEVICESTATUSMODEL_H
#define DEVICESTATUSMODEL_H
#include <QAbstractTableModel>
#include <QVector>
#include <QString>

struct DeviceStatus {
    int deviceId = 0;
    QString ip;
    int port = 502;
    bool online = false;
};

class DeviceStatusModel : public QAbstractTableModel {
    Q_OBJECT
public:
    enum Col { ColId, ColIp, ColPort, ColStatus, ColCount };
    explicit DeviceStatusModel(QObject *parent = nullptr);
    int rowCount(const QModelIndex &p = QModelIndex()) const override;
    int columnCount(const QModelIndex &p = QModelIndex()) const override;
    QVariant data(const QModelIndex &i, int role = Qt::DisplayRole) const override;
    QVariant headerData(int s, Qt::Orientation o, int role = Qt::DisplayRole) const override;
    void refresh(const QVector<DeviceStatus> &list);
private:
    QVector<DeviceStatus> m_devices;
};
#endif
