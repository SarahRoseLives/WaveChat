#include "ProtocolLogger.h"
#include <QCoreApplication>
#include <QFile>
#include <QTextStream>
#include <QMutex>
#include <QMutexLocker>
#include <QDateTime>
#include <QDir>
#include <QStandardPaths>
#include <QProcess>

ProtocolLogger::ProtocolLogger()
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)
                  + "/WaveChat/logs";
    QDir().mkpath(dir);

    qint64 pid = QCoreApplication::applicationPid();
    QString date = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
    QString path = dir + "/WaveChat_" + QString::number(pid) + "_" + date + ".log";

    m_file = new QFile(path);
    m_file->open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text);
    m_stream = new QTextStream(m_file);
    m_mutex = new QMutex();
}

ProtocolLogger::~ProtocolLogger()
{
    if (m_stream) {
        m_stream->flush();
        delete m_stream;
    }
    if (m_file) {
        m_file->close();
        delete m_file;
    }
    delete m_mutex;
}

ProtocolLogger* ProtocolLogger::instance()
{
    static ProtocolLogger inst;
    return &inst;
}

void ProtocolLogger::log(const QString& category, const QString& text)
{
    instance()->writeLine(category, text);
}

void ProtocolLogger::writeLine(const QString& category, const QString& text)
{
    QMutexLocker lock(m_mutex);

    QString ts = QDateTime::currentDateTime().toString("hh:mm:ss.zzz");
    QString cat = category.leftJustified(9, ' ', true);

    *m_stream << "[" << ts << "] " << cat << text << "\n";
    m_stream->flush();
}
