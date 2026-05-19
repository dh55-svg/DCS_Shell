#include <QApplication>
#include <QSet>
#include <QDir>
#include <QDebug>
#include "app/AppConfig.h"
#include "app/ApplicationBuilder.h"
#include "application/DataController.h"
#include "application/AlarmController.h"
#include "infrastructure/plugin/PluginHub.h"
#include "plugin_interface/IMqttGateway.h"
#include "presentation/MainWindow.h"

static QString findConfigDir() {
    QStringList candidates = { "config", "../../../config", "../../config", "../config" };
    for (const auto& c : candidates) {
        if (QFile::exists(c + "/app.json")) return c;
    }
    return "config";
}

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    // 1. PluginHub scan
    auto hub = std::make_shared<PluginHub>();
    QString pluginsDir = QApplication::applicationDirPath() + "/plugins";
    if (!QDir(pluginsDir).exists()) pluginsDir = "plugins";
    int found = hub->scanAll(pluginsDir);
    qDebug() << "[main] PluginHub scanned:" << found << "candidates";

    // 2. Load config
    QString configDir = findConfigDir();
    qDebug() << "[main] config dir:" << configDir;
    auto configRepo = std::make_shared<JsonConfigRepo>();
    AppConfig appCfg = AppConfig{}.fromJson(configDir + "/app.json", *configRepo);

    // 3. Build dependency graph
    auto ctx = ApplicationBuilder()
        .withPluginHub(hub)
        .withConfig(appCfg)
        .withLogger()
        .withAuditLogger()
        .withConfigRepo()
        .withFieldbus()
        .withDatabase()
        .withDomain()
        .withPipeline()
        .withMqtt()
        .build();

    // 4. Validate critical dependencies
    if (!ctx || !ctx->tagManager || !ctx->fieldbus || !ctx->dataPipeline) {
        qCritical() << "[FATAL] ApplicationBuilder::build() failed — missing critical dependency";
        return 1;
    }

    // 5. Load tags
    bool tagsOk = ctx->tagManager->loadFromJson(configDir + "/tags.json");
    qDebug() << "[main] tags:" << (tagsOk ? "OK" : "FAIL") << "count:" << ctx->tagManager->tagCount();

    QSet<int> serverAddrs;
    for (const auto& tag : ctx->tagManager->getAllTags())
        serverAddrs.insert(tag.modbusServerAddr);
    for (int serverAddr : serverAddrs) {
        DeviceConfig dev;
        dev.deviceId = serverAddr;
        dev.serverAddr = serverAddr;
        dev.regStart = 0;
        dev.regCount = 128;
        ctx->fieldbus->addDevice(dev);
    }

    QVector<TagInf> tagVec;
    for (const auto& tag : ctx->tagManager->getAllTags())
        tagVec.append(tag);
    ctx->dataPipeline->injectTagConfig(tagVec);

    // 5. Controllers + MainWindow
    DataController dataCtrl(*ctx->dataPipeline, *ctx->tagManager, *ctx->alarmEngine,
                            *ctx->fieldbus, ctx->logger.get());
    AlarmController alarmCtrl(*ctx->alarmEngine, ctx->logger.get());
    MainWindow window(dataCtrl, alarmCtrl, ctx->logger.get());
    window.show();

    return app.exec();
}
