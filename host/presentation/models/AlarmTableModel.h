// =============================================================================
// AlarmTableModel.h — 告警列表 Model (QAbstractTableModel)
// =============================================================================
#ifndef ALARMTABLEMODEL_H
#define ALARMTABLEMODEL_H
#include <QAbstractTableModel>
#include <QVector>
#include <QDateTime>
#include "../../domain/alarm/AlarmEvent.h"

class AlarmTableModel : public QAbstractTableModel {
    Q_OBJECT
public:
    enum Col { ColTime, ColLevel, ColTagName, ColDescription, ColStatus, ColAction, ColCount };

    explicit AlarmTableModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation o, int role = Qt::DisplayRole) const override;

    void appendEvent(const AlarmEvent &e);
    void refresh(const QVector<AlarmEvent> &events);
    AlarmEvent eventAt(int row) const;

signals:
    void acknowledgeRequested(int row);

private:
    QVector<AlarmEvent> m_events;
};

#endif
