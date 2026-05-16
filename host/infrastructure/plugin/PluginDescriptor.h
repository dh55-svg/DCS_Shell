#ifndef PLUGINDESCRIPTOR_H
#define PLUGINDESCRIPTOR_H
#include <QString>
#include <QJsonObject>

struct PluginDescriptor {
    QString filePath;
    QString iid;
    QString className;
    QString name;           ///< 插件名称（来自 companion JSON）
    QString version;        ///< 语义版本号（如 "1.0.0"）
    QString author;         ///< 作者/组织
    QString compatibility;  ///< 兼容性约束（如 ">=1.0.0"）
    int priority = 0;
    int qtVersion = 0;
    bool passed = false;
    QString failReason;
    QJsonObject rawMeta;
};
#endif
