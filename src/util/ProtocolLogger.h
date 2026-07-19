#pragma once
#include <QString>

class ProtocolLogger {
public:
    static ProtocolLogger* instance();
    static void log(const QString& category, const QString& text);

private:
    ProtocolLogger();
    ~ProtocolLogger();
    void writeLine(const QString& category, const QString& text);

    class QFile* m_file = nullptr;
    class QTextStream* m_stream = nullptr;
    class QMutex* m_mutex = nullptr;
};
