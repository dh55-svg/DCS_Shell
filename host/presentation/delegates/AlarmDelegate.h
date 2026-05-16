// =============================================================================
// AlarmDelegate.h — 告警级别着色 + 确认按钮
// =============================================================================
#ifndef ALARMDELEGATE_H
#define ALARMDELEGATE_H
#include <QStyledItemDelegate>
class AlarmDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:
    explicit AlarmDelegate(QObject *parent = nullptr);
    void paint(QPainter *p, const QStyleOptionViewItem &opt, const QModelIndex &idx) const override;
    bool editorEvent(QEvent *e, QAbstractItemModel *m, const QStyleOptionViewItem &opt, const QModelIndex &idx) override;
signals:
    void ackClicked(int row);
};
#endif
