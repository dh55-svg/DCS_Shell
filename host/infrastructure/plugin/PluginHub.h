#ifndef PLUGINHUB_H
#define PLUGINHUB_H
#include <QObject>
#include <QVector>
#include <QPluginLoader>
#include "PluginDescriptor.h"

class PluginHub : public QObject {
    Q_OBJECT
public:
    explicit PluginHub(QObject* parent = nullptr);
    ~PluginHub() override;

    int scanAll(const QString& pluginsDir);

    template<typename T> T* resolve(const char* iid);
    template<typename T> QVector<PluginDescriptor> discover(const char* iid) const;

    bool unloadPlugin(const char* iid);
    bool switchPlugin(const char* iid, const QString& newPath);
    bool canSwitch(const char* iid, const QString& newPath);
    QStringList availablePlugins(const char* iid) const;
    int loadedCount() const { return m_loaded.size(); }
    void unloadAll();

signals:
    void pluginLoaded(const QString& name);
    void pluginUnloaded(const QString& name);
    void pluginSwitched(const QString& iid, const QString& from, const QString& to);
    void pluginError(const QString& iid, const QString& reason);

private:
    void log(const QString& msg);
    QVector<PluginDescriptor> m_discovered;

    struct LoadedItem {
        PluginDescriptor desc;
        QObject* instance = nullptr;
        QPluginLoader* loader = nullptr;
    };
    QVector<LoadedItem> m_loaded;
};

// ─── Template implementations ───
#include <algorithm>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QDebug>

template<typename T>
T* PluginHub::resolve(const char* iid) {
    if (!iid) return nullptr;

    QVector<PluginDescriptor*> candidates;
    for (auto& d : m_discovered)
        if (d.passed && d.iid == QString(iid))
            candidates.append(&d);

    if (candidates.isEmpty()) {
        log(QString("No matching plugin for %1").arg(iid));
        return nullptr;
    }

    std::sort(candidates.begin(), candidates.end(),
        [](auto* a, auto* b) { return a->priority > b->priority; });

    for (auto* cand : candidates) {
        auto* loader = new QPluginLoader(cand->filePath);
        QObject* inst = loader->instance();
        if (!inst) {
            log(QString("Failed to load %1: %2").arg(cand->filePath, loader->errorString()));
            delete loader;
            continue;
        }
        T* plugin = dynamic_cast<T*>(inst);
        if (!plugin) {
            log(QString("qobject_cast failed for %1").arg(cand->filePath));
            loader->unload();
            delete loader;
            continue;
        }
        LoadedItem item;
        item.desc = *cand;
        item.instance = inst;
        item.loader = loader;
        m_loaded.append(item);
        log(QString("Loaded: %1 (priority=%2)").arg(cand->filePath).arg(cand->priority));
        emit pluginLoaded(cand->filePath);
        return plugin;
    }
    return nullptr;
}

template<typename T>
QVector<PluginDescriptor> PluginHub::discover(const char* iid) const {
    QVector<PluginDescriptor> results;
    if (!iid) return results;
    for (auto& d : m_discovered)
        if (d.passed && d.iid == QString(iid))
            results.append(d);
    std::sort(results.begin(), results.end(),
        [](auto& a, auto& b) { return a.priority > b.priority; });
    return results;
}
#endif
