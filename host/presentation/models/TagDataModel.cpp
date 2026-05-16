// =============================================================================
// TagDataModel.cpp
// =============================================================================
#include "TagDataModel.h"
#include <QColor>
int TagDataModel::rowCount(const QModelIndex &) const { return m_tags.size(); }
int TagDataModel::columnCount(const QModelIndex &) const { return ColCount; }
TagDataModel::TagDataModel(QObject *parent) : QAbstractTableModel(parent) {}

QVariant TagDataModel::data(const QModelIndex &i, int role) const {
    if (!i.isValid()) return {};
    const auto &t = m_tags.at(i.row());
    if (role == Qt::DisplayRole) {
        switch (i.column()) {
        case ColName: return t.tagName; case ColValue: return t.value;
        case ColUnit: return t.unit;
        case ColQuality: return t.quality == 0 ? "Good" : (t.quality == 1 ? "Uncertain" : "Bad");
        case ColTime: return QDateTime::fromMSecsSinceEpoch(t.timestamp).toString("hh:mm:ss");
        }
    }
    if (role == Qt::ForegroundRole && i.column() == ColQuality)
        return t.quality == 0 ? QColor("#00ff88") : (t.quality == 1 ? QColor("#ffcc00") : QColor("#ff4444"));
    return {};
}

QVariant TagDataModel::headerData(int s, Qt::Orientation o, int role) const {
    if (role != Qt::DisplayRole || o != Qt::Horizontal) return {};
    switch (s) {
    case ColName: return "位号"; case ColValue: return "值";
    case ColUnit: return "单位"; case ColQuality: return "质量"; case ColTime: return "时间";
    }
    return {};
}

void TagDataModel::refresh(const QVector<TagData> &list) {
    beginResetModel(); m_tags = list; endResetModel();
}
