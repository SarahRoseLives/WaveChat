#pragma once
#include <QString>
#include <QDateTime>

struct Message {
    enum Type { Text, System, File, Image };

    QString callsign;
    QString text;
    QString channel;
    QDateTime timestamp;
    Type type = Text;

    static Message systemMessage(const QString& text)
    {
        Message m;
        m.text = text;
        m.type = System;
        m.timestamp = QDateTime::currentDateTime();
        return m;
    }
};
