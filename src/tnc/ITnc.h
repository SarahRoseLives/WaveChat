#pragma once
#include <QObject>

class ITnc : public QObject {
    Q_OBJECT
public:
    explicit ITnc(QObject* parent = nullptr)
        : QObject(parent)
    {
    }
    virtual ~ITnc() = default;

    virtual bool isConnected() const = 0;

public slots:
    virtual void connectTnc() = 0;
    virtual void disconnectTnc() = 0;
    virtual void sendFrame(const QByteArray& frame) = 0;

signals:
    void frameReceived(const QByteArray& frame);
    void connected();
    void disconnected();
    void errorOccurred(const QString& error);
};
