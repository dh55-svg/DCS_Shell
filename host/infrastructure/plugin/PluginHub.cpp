#include "PluginHub.h"
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>

PluginHub::PluginHub(QObject* parent) : QObject(parent) {}
PluginHub::~PluginHub() { unloadAll(); }

int PluginHub::scanAll(const QString& pluginsDir) {
    m_discovered.clear();
    QDir dir(pluginsDir);
    if (!dir.exists()) return 0;

    QStringList filters;
#ifdef Q_OS_WIN
    filters << "*.dll";
#else
    filters << "*.so";
#endif

    for (const QFileInfo& fi : dir.entryInfoList(filters, QDir::Files)) {
        QPluginLoader loader(fi.absoluteFilePath());
        QJsonObject meta = loader.metaData();
        if (meta.isEmpty() || !meta.contains("IID")) continue;

        PluginDescriptor desc;
        desc.filePath = fi.absoluteFilePath();
        desc.iid = meta["IID"].toString();
        desc.className = meta["className"].toString();
        desc.qtVersion = meta["version"].toInt();
        desc.rawMeta = meta;

        // Check companion JSON for priority
        QString jsonPath = fi.absolutePath() + "/" + fi.completeBaseName() + ".json";
        if (QFile::exists(jsonPath)) {
            QFile f(jsonPath);
            if (f.open(QIODevice::ReadOnly)) {
                QJsonObject j = QJsonDocument::fromJson(f.readAll()).object();
                desc.name = j.value("name").toString();
                desc.version = j.value("version").toString();
                desc.author = j.value("author").toString();
                desc.compatibility = j.value("compatibility").toString();
                desc.priority = j.value("priority").toInt(0);
            }
        }
        desc.passed = true;
        m_discovered.append(desc);
    }
    return m_discovered.size();
}

bool PluginHub::unloadPlugin(const char* iid) {
    QString iidStr(iid);
    for (int i = 0; i < m_loaded.size(); ++i) {
        if (m_loaded[i].desc.iid == iidStr) {
            auto& item = m_loaded[i];
            emit pluginUnloaded(item.desc.filePath);
            item.loader->unload();
            delete item.loader;
            m_loaded.removeAt(i);
            return true;
        }
    }
    return false;
}

bool PluginHub::canSwitch(const char* iid, const QString& newPath) {
    QPluginLoader loader(newPath);
    QObject* inst = loader.instance();
    if (!inst) return false;
    QJsonObject meta = loader.metaData();
    bool ok = (meta["IID"].toString() == QString(iid));
    loader.unload();
    return ok;
}

bool PluginHub::switchPlugin(const char* iid, const QString& newPath) {
    QString iidStr(iid);
    QString oldName = "none";
    for (auto& item : m_loaded) {
        if (item.desc.iid == iidStr) oldName = item.desc.filePath;
    }
    unloadPlugin(iid);

    // ── 清理 m_discovered 中匹配此 iid 的旧条目 ──
    m_discovered.erase(
        std::remove_if(m_discovered.begin(), m_discovered.end(),
            [&iidStr](const PluginDescriptor& d) { return d.iid == iidStr; }),
        m_discovered.end());

    PluginDescriptor newDesc;
    newDesc.filePath = newPath;
    newDesc.iid = iidStr;
    newDesc.passed = true;
    newDesc.priority = 100;
    m_discovered.append(newDesc);

    emit pluginSwitched(iidStr, oldName, newPath);
    return true;
}

QStringList PluginHub::availablePlugins(const char* iid) const {
    QStringList result;
    for (auto& d : m_discovered)
        if (d.passed && d.iid == QString(iid))
            result.append(d.filePath);
    return result;
}

void PluginHub::unloadAll() {
    for (auto& item : m_loaded) {
        item.loader->unload();
        delete item.loader;
    }
    m_loaded.clear();
}

void PluginHub::log(const QString& msg) {
    qDebug() << "[PluginHub]" << msg;
}
