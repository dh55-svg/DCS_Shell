// =============================================================================
// ValueDelegate.h — 数据质量标记
// =============================================================================
#ifndef VALUEDELEGATE_H
#define VALUEDELEGATE_H
#include <QStyledItemDelegate>
class ValueDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:
    explicit ValueDelegate(QObject *parent = nullptr);
    void paint(QPainter *p, const QStyleOptionViewItem &opt, const QModelIndex &idx) const override;
};
#endif
