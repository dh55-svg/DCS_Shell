#ifndef NULLOPERATIONREPO_H
#define NULLOPERATIONREPO_H
#include "plugin_interface/IOperationRepo.h"

class NullOperationRepo : public IOperationRepo {
public:
    void log(const QString&, const QString&, const QString&, const QString&) override {}
    QVector<QJsonObject> query(qint64, qint64, int) override { return {}; }
};
#endif
