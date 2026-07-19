#pragma once
#include <QString>
#include <QDateTime>
#include <QProgressBar>
#include <QWidget>

// ---------------------------------------------------------------------------
// File transfer sub-struct — lives on Message when type == File
// ---------------------------------------------------------------------------
struct FileTransferInfo {
    QString  fileId;
    QString  fileName;
    QString  fromCallsign; // sender
    qint64   fileSize = 0;
    int      totalChunks = 0;
    int      receivedChunks = 0;
    enum State { Offering, Transferring, Complete, Failed, Cancelled } state = Offering;
    QString  savePath;      // set after complete
    QProgressBar* progressBar = nullptr; // widget pointer — null for stored/offering
};

// ---------------------------------------------------------------------------
// Message
// ---------------------------------------------------------------------------
struct Message {
    enum Type { Text, System, File };

    QString   callsign;
    QString   text;
    QString   channel;
    QDateTime timestamp;
    Type      type = Text;

    // Only valid when type == File
    FileTransferInfo file;

    static Message systemMessage(const QString& text)
    {
        Message m;
        m.text = text;
        m.type = System;
        m.timestamp = QDateTime::currentDateTime();
        return m;
    }
};
