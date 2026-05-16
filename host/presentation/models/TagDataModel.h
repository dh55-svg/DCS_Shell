// =============================================================================
// TagDataModel.h — 实时位号数据 Model
// =============================================================================
#ifndef TAGDATAMODEL_H
#define TAGDATAMODEL_H
#include <QAbstractTableModel>
#include <QVector>
#include <QDateTime>

struct TagData {
    QString tagName;
    double value = 0;
    QString unit;
    int quality = 0;
    qint64 timestamp = 0;
};

class TagDataModel : public QAbstractTableModel {
    Q_OBJECT
public:
    enum Col { ColName, ColValue, ColUnit, ColQuality, ColTime, ColCount };
    explicit TagDataModel(QObject *parent = nullptr);
    int rowCount(const QModelIndex &p = QModelIndex()) const override;
    int columnCount(const QModelIndex &p = QModelIndex()) const override;
    QVariant data(const QModelIndex &i, int role = Qt::DisplayRole) const override;
    QVariant headerData(int s, Qt::Orientation o, int role = Qt::DisplayRole) const override;
    void refresh(const QVector<TagData> &list);
private:
    QVector<TagData> m_tags;
};
#endif
