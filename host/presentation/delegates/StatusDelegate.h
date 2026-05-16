// =============================================================================
// StatusDelegate.h — 设备在线状态圆形指示灯
// =============================================================================
#ifndef STATUSDELEGATE_H
#define STATUSDELEGATE_H
#include <QStyledItemDelegate>
class StatusDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:
    explicit StatusDelegate(QObject *parent = nullptr);
    void paint(QPainter *p, const QStyleOptionViewItem &opt, const QModelIndex &idx) const override;
};
#endif
