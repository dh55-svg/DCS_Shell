// =============================================================================
// AlarmDelegate.cpp
// =============================================================================
#include "AlarmDelegate.h"
#include <QPainter>
#include <QApplication>
#include <QStyleOptionButton>
#include <QMouseEvent>
#include "../models/AlarmTableModel.h"

AlarmDelegate::AlarmDelegate(QObject *parent) : QStyledItemDelegate(parent) {}

void AlarmDelegate::paint(QPainter *p, const QStyleOptionViewItem &opt, const QModelIndex &idx) const {
    if (idx.column() == AlarmTableModel::ColAction) {
        QStyleOptionButton btn;
        btn.rect = opt.rect.adjusted(4, 3, -4, -3);
        btn.text = "确认";
        btn.state = QStyle::State_Enabled;
        QApplication::style()->drawControl(QStyle::CE_PushButton, &btn, p);
        return;
    }
    // 行背景色
    p->save();
    if (opt.state & QStyle::State_Selected) {
        p->fillRect(opt.rect, QColor("#0f3460"));
    }
    p->restore();
    QStyledItemDelegate::paint(p, opt, idx);
}

bool AlarmDelegate::editorEvent(QEvent *e, QAbstractItemModel *, const QStyleOptionViewItem &opt, const QModelIndex &idx) {
    if (idx.column() == AlarmTableModel::ColAction && e->type() == QEvent::MouseButtonRelease) {
        emit ackClicked(idx.row());
        return true;
    }
    return false;
}
