#include "KissTnc.h"
#include "KissFrame.h"
#include "util/ProtocolLogger.h"
#include <QTimer>

KissTnc::KissTnc(QObject* parent)
    : ITnc(parent)
    , m_serial(new QSerialPort(this))
{
    connect(m_serial, &QSerialPort::readyRead, this, &KissTnc::onReadyRead);
    connect(m_serial, &QSerialPort::errorOccurred, this, &KissTnc::onErrorOccurred);
}

KissTnc::~KissTnc()
{
    disconnectTnc();
}

bool KissTnc::isConnected() const
{
    return m_serial && m_serial->isOpen();
}

void KissTnc::setPortName(const QString& name) { m_portName = name; }
void KissTnc::setBaudRate(int baudRate) { m_baudRate = baudRate; }
void KissTnc::setFlowControl(QSerialPort::FlowControl fc) { m_flowControl = fc; }
void KissTnc::setTxDelay(int ms) { m_txDelay = qBound(0, ms, 1000); }

void KissTnc::connectTnc()
{
    if (m_serial->isOpen())
        disconnectTnc();

    m_serial->setPortName(m_portName);
    m_serial->setBaudRate(m_baudRate);
    m_serial->setDataBits(QSerialPort::Data8);
    m_serial->setStopBits(QSerialPort::OneStop);
    m_serial->setParity(QSerialPort::NoParity);
    m_serial->setFlowControl(m_flowControl);

    if (!m_serial->open(QIODevice::ReadWrite)) {
        emit errorOccurred(QString("Failed to open %1: %2")
                               .arg(m_portName, m_serial->errorString()));
        return;
    }

    m_buffer.clear();

    // Send KISS initialization parameters after a brief settle delay
    QTimer::singleShot(200, this, [this]() {
        if (m_serial->isOpen())
            sendKissInit();
        emit connected();
    });
}

void KissTnc::disconnectTnc()
{
    if (m_serial->isOpen())
        m_serial->close();
    m_buffer.clear();
    emit disconnected();
}

void KissTnc::sendFrame(const QByteArray& frame)
{
    if (!m_serial->isOpen())
        return;
    m_serial->write(frame);
    m_serial->flush();
    ProtocolLogger::log("TX KISS", QString("bytes=%1").arg(frame.size()) + " " + frame.toHex(' '));
}

void KissTnc::sendKissInit()
{
    // TX delay: how long (in 10ms units) the TNC waits between PTT and data
    if (m_txDelay > 0) {
        uint8_t delay = static_cast<uint8_t>(qMin(m_txDelay / 10, 255));
        QByteArray cmd = KissFrame::encode(KissFrame::CMD_TXDELAY, QByteArray(1, static_cast<char>(delay)));
        m_serial->write(cmd);
    }

    // Persistence: 63 = moderate persistence for shared channels
    {
        QByteArray cmd = KissFrame::encode(KissFrame::CMD_P, QByteArray(1, static_cast<char>(63)));
        m_serial->write(cmd);
    }

    // Slot time: 10 (= 100ms), standard for 1200 baud
    {
        QByteArray cmd = KissFrame::encode(KissFrame::CMD_SLOTTIME, QByteArray(1, static_cast<char>(10)));
        m_serial->write(cmd);
    }

    // TX tail: 5 (= 50ms) to keep TX keyed briefly after last byte
    {
        QByteArray cmd = KissFrame::encode(KissFrame::CMD_TXTAIL, QByteArray(1, static_cast<char>(5)));
        m_serial->write(cmd);
    }

    // Full duplex: 0 = half duplex (normal for single-radio)
    {
        QByteArray cmd = KissFrame::encode(KissFrame::CMD_FULLDUPLEX, QByteArray(1, static_cast<char>(0)));
        m_serial->write(cmd);
    }

    m_serial->flush();
}

void KissTnc::onReadyRead()
{
    m_buffer.append(m_serial->readAll());

    while (true) {
        int start = m_buffer.indexOf(static_cast<char>(KissFrame::FEND));
        if (start < 0)
            break;

        int end = m_buffer.indexOf(static_cast<char>(KissFrame::FEND), start + 1);
        if (end < 0)
            break;

        QByteArray frame = m_buffer.mid(start, end - start + 1);
        m_buffer.remove(0, end + 1);

        if (frame.size() > 2) {
            ProtocolLogger::log("RX KISS", QString("bytes=%1").arg(frame.size()) + " " + frame.toHex(' '));
            emit frameReceived(frame);
        }
    }
}

void KissTnc::onErrorOccurred()
{
    emit errorOccurred(m_serial->errorString());
}
