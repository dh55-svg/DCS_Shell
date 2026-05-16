#ifndef SPDLOGADAPTER_H
#define SPDLOGADAPTER_H
#include "ILogger.h"
#include <QString>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QMutex>

class FileLogger : public ILogger {
public:
    void setLogDir(const QString& dir) {
        QDir().mkpath(dir);
        m_logFile.setFileName(dir + "/dcs_" +
            QDateTime::currentDateTime().toString("yyyyMMdd") + ".log");
    }

    void info(const QString& msg) override { write("INFO", msg); }
    void warn(const QString& msg) override { write("WARN", msg); }
    void error(const QString& msg) override { write("ERROR", msg); }
    void debug(const QString& msg) override { write("DEBUG", msg); }

private:
    void write(const QString& level, const QString& msg) {
        QMutexLocker lock(&m_mutex);
        if (!m_logFile.isOpen())
            m_logFile.open(QIODevice::Append | QIODevice::Text);
        if (m_logFile.isOpen()) {
            QTextStream stream(&m_logFile);
            stream << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz")
                   << " [" << level << "] " << msg << "\n";
            stream.flush();
        }
    }
    QFile m_logFile;
    QMutex m_mutex;
};
#endif
