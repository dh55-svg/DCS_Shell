#ifndef IOPERATIONREPO_H
#define IOPERATIONREPO_H
#include <QObject>
#include <QVector>
#include <QString>
#include <QJsonObject>

class IOperationRepo {
public:
    virtual ~IOperationRepo() = default;
    virtual void log(const QString& user, const QString& action,
                     const QString& target, const QString& detail) = 0;
    virtual QVector<QJsonObject> query(qint64 start, qint64 end, int limit) = 0;
};

#define IOperationRepo_iid "com.dcsshell.IOperationRepo"
Q_DECLARE_INTERFACE(IOperationRepo, IOperationRepo_iid)
#endif
