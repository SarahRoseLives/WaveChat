#pragma once
#include <QObject>
#include <QPair>
#include "chat/Message.h"
#include "tnc/ITnc.h"

class ChatController : public QObject {
    Q_OBJECT
public:
    explicit ChatController(QObject* parent = nullptr);

    void setTnc(ITnc* tnc);
    ITnc* tnc() const;

    void setCallsign(const QString& callsign);
    QString callsign() const;

    void setCurrentChannel(const QString& channel);
    QString currentChannel() const;

    bool isConnected() const;

public slots:
    void connectTnc();
    void disconnectTnc();
    void sendMessage(const QString& text);
    void sendRawMessage(const QString& text);

signals:
    void messageReceived(const Message& message);
    void rawFrameReceived(const QByteArray& infoField, const QString& fromCallsign);
    void connected();
    void disconnected();
    void errorOccurred(const QString& error);

private slots:
    void onFrameReceived(const QByteArray& wireFrame);

private:
    ITnc* m_tnc = nullptr;
    QString m_callsign;
    QString m_currentChannel = "main";

    QByteArray buildFrame(const QString& text) const;

    static QByteArray encodeAx25Address(const QString& callsign, int ssid, bool isLast);
    static QPair<QString, int> decodeAx25Address(const QByteArray& data, int offset);
    static QPair<QString, QString> parseAx25UiFrame(const QByteArray& ax25);
    static QPair<QString, QString> parseChannelFromText(const QString& info);
};
