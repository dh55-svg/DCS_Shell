// =============================================================================
// DeviceStatusModel.cpp
// =============================================================================
#include "DeviceStatusModel.h"
#include <QColor>

int DeviceStatusModel::rowCount(const QModelIndex &) const { return m_devices.size(); }
int DeviceStatusModel::columnCount(const QModelIndex &) const { return ColCount; }
DeviceStatusModel::DeviceStatusModel(QObject *parent) : QAbstractTableModel(parent) {}

QVariant DeviceStatusModel::data(const QModelIndex &i, int role) const {
    if (!i.isValid()) return {};
    const auto &d = m_devices.at(i.row());
    if (role == Qt::DisplayRole) {
        switch (i.column()) {
        case ColId: return d.deviceId; case ColIp: return d.ip;
        case ColPort: return d.port; case ColStatus: return d.online ? "● 在线" : "○ 离线";
        }
    }
    if (role == Qt::ForegroundRole && i.column() == ColStatus)
        return d.online ? QColor("#00ff88") : QColor("#888888");
    return {};
}

QVariant DeviceStatusModel::headerData(int s, Qt::Orientation o, int role) const {
    if (role != Qt::DisplayRole || o != Qt::Horizontal) return {};
    switch (s) {
    case ColId: return "ID"; case ColIp: return "IP";
    case ColPort: return "端口"; case ColStatus: return "状态";
    }
    return {};
}

void DeviceStatusModel::refresh(const QVector<DeviceStatus> &list) {
    beginResetModel(); m_devices = list; endResetModel();
}
