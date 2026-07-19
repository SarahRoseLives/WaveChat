#pragma once
#include "ITnc.h"
#include <QSerialPort>
#include <QByteArray>

class KissTnc : public ITnc {
    Q_OBJECT
public:
    explicit KissTnc(QObject* parent = nullptr);
    ~KissTnc() override;
    bool isConnected() const override;

    void setPortName(const QString& name);
    void setBaudRate(int baudRate);
    void setFlowControl(QSerialPort::FlowControl fc);
    void setTxDelay(int ms);

public slots:
    void connectTnc() override;
    void disconnectTnc() override;
    void sendFrame(const QByteArray& frame) override;

private slots:
    void onReadyRead();
    void onErrorOccurred();

private:
    QSerialPort* m_serial = nullptr;
    QString m_portName;
    int m_baudRate = 9600;
    QSerialPort::FlowControl m_flowControl = QSerialPort::HardwareControl;
    int m_txDelay = 300;
    QByteArray m_buffer;

    void sendKissInit();
};
