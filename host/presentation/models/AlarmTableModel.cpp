// =============================================================================
// AlarmTableModel.cpp
// =============================================================================
#include "AlarmTableModel.h"
#include <QColor>
#include <QFont>

AlarmTableModel::AlarmTableModel(QObject *parent) : QAbstractTableModel(parent) {}

int AlarmTableModel::rowCount(const QModelIndex &) const { return m_events.size(); }
int AlarmTableModel::columnCount(const QModelIndex &) const { return ColCount; }

QVariant AlarmTableModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() >= m_events.size()) return {};
    const auto &e = m_events.at(index.row());

    if (role == Qt::DisplayRole) {
        switch (index.column()) {
        case ColTime:        return QDateTime::fromMSecsSinceEpoch(e.triggerTime).toString("MM-dd hh:mm:ss");
        case ColLevel: {
            switch (e.priority) {
            case AlarmPriority::Critical: return QStringLiteral("紧急");
            case AlarmPriority::Major:    return QStringLiteral("高");
            case AlarmPriority::Minor:    return QStringLiteral("中");
            default:                      return QStringLiteral("低");
            }
        }
        case ColTagName:     return e.tagName;
        case ColDescription: return e.description;
        case ColStatus:      return e.acknowledged ? "已确认" : "未确认";
        case ColAction:      return "确认";
        }
    }
    if (role == Qt::ForegroundRole) {
        switch (e.priority) {
        case AlarmPriority::Critical: return QColor("#ff4444");
        case AlarmPriority::Major:    return QColor("#ff8800");
        case AlarmPriority::Minor:    return QColor("#ffcc00");
        default:                      return QColor("#44aaff");
        }
    }
    if (role == Qt::FontRole && !e.acknowledged) {
        QFont f; f.setBold(true); return f;
    }
    return {};
}

QVariant AlarmTableModel::headerData(int section, Qt::Orientation o, int role) const {
    if (role != Qt::DisplayRole || o != Qt::Horizontal) return {};
    switch (section) {
    case ColTime: return "时间"; case ColLevel: return "级别";
    case ColTagName: return "位号"; case ColDescription: return "描述";
    case ColStatus: return "状态"; case ColAction: return "操作";
    }
    return {};
}

void AlarmTableModel::appendEvent(const AlarmEvent &e) {
    int row = m_events.size();
    beginInsertRows(QModelIndex(), row, row);
    m_events.append(e);
    endInsertRows();
}

void AlarmTableModel::refresh(const QVector<AlarmEvent> &events) {
    beginResetModel();
    m_events = events;
    endResetModel();
}

AlarmEvent AlarmTableModel::eventAt(int row) const {
    return m_events.value(row);
}
