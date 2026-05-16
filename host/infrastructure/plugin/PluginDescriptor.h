#ifndef PLUGINDESCRIPTOR_H
#define PLUGINDESCRIPTOR_H
#include <QString>
#include <QJsonObject>

struct PluginDescriptor {
    QString filePath;
    QString iid;
    QString className;
    int priority = 0;
    int qtVersion = 0;
    bool passed = false;
    QString failReason;
    QJsonObject rawMeta;
};
#endif
