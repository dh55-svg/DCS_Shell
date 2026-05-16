// =============================================================================
// ValueDelegate.cpp — 质量列着色标记
// =============================================================================
#include "ValueDelegate.h"
#include <QPainter>
#include "../models/TagDataModel.h"

ValueDelegate::ValueDelegate(QObject *parent) : QStyledItemDelegate(parent) {}

void ValueDelegate::paint(QPainter *p, const QStyleOptionViewItem &opt, const QModelIndex &idx) const {
    if (idx.column() != TagDataModel::ColQuality) {
        QStyledItemDelegate::paint(p, opt, idx);
        return;
    }
    QStyledItemDelegate::paint(p, opt, idx);
    // 质量标记: Bad 画红色三角
    int q = idx.data(Qt::UserRole).toInt();
    if (q >= 2) {
        p->save();
        p->setRenderHint(QPainter::Antialiasing);
        p->setBrush(QColor("#ff4444"));
        p->setPen(Qt::NoPen);
        QPolygonF tri; tri << QPointF(opt.rect.right()-10, opt.rect.top()+2)
                           << QPointF(opt.rect.right()-2, opt.rect.top()+2)
                           << QPointF(opt.rect.right()-6, opt.rect.top()+10);
        p->drawPolygon(tri);
        p->restore();
    }
}
