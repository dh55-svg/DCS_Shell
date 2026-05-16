// =============================================================================
// StatusDelegate.cpp — 绘制在线/离线/报警圆形指示灯
// =============================================================================
#include "StatusDelegate.h"
#include <QPainter>
#include "../models/DeviceStatusModel.h"

StatusDelegate::StatusDelegate(QObject *parent) : QStyledItemDelegate(parent) {}

void StatusDelegate::paint(QPainter *p, const QStyleOptionViewItem &opt, const QModelIndex &idx) const {
    if (idx.column() != DeviceStatusModel::ColStatus) {
        QStyledItemDelegate::paint(p, opt, idx);
        return;
    }
    p->save();
    bool online = idx.data(Qt::DisplayRole).toString().startsWith("●");
    QColor color = online ? QColor("#00ff88") : QColor("#666666");
    p->setRenderHint(QPainter::Antialiasing);
    p->setBrush(color);
    p->setPen(Qt::NoPen);
    int cx = opt.rect.center().x();
    int cy = opt.rect.center().y();
    int r = qMin(opt.rect.width(), opt.rect.height()) / 2 - 4;
    p->drawEllipse(QPoint(cx, cy), r, r);
    p->restore();
}
