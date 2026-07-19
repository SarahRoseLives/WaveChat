#include "ChatController.h"
#include "tnc/KissFrame.h"
#include <QDateTime>

// ============================================================================
// AX.25 address encoding (7 bytes per address)
// ============================================================================

QByteArray ChatController::encodeAx25Address(const QString& callsign, int ssid, bool isLast)
{
    QString baseCall = callsign.toUpper();
    int effectiveSsid = ssid;

    int dashIdx = baseCall.lastIndexOf('-');
    if (dashIdx >= 0 && dashIdx < baseCall.length() - 1) {
        bool ok = false;
        int parsed = baseCall.mid(dashIdx + 1).toInt(&ok);
        if (ok && parsed >= 0 && parsed <= 15) {
            effectiveSsid = parsed;
            baseCall = baseCall.left(dashIdx);
        }
    }

    QByteArray addr(7, '\0');
    QString cs = baseCall.leftJustified(6, ' ').left(6);

    for (int i = 0; i < 6; ++i) {
        addr[i] = static_cast<char>(static_cast<uint8_t>(cs[i].toLatin1()) << 1);
    }

    uint8_t lastByte = static_cast<uint8_t>((effectiveSsid & 0x0F) << 1) | 0x60;
    if (isLast)
        lastByte |= 0x01;

    addr[6] = static_cast<char>(lastByte);
    return addr;
}

QPair<QString, int> ChatController::decodeAx25Address(const QByteArray& data, int offset)
{
    QString callsign;
    int ssid = 0;

    if (offset + 7 > data.size())
        return {callsign, ssid};

    for (int i = 0; i < 6; ++i) {
        char c = static_cast<char>(static_cast<uint8_t>(data[offset + i]) >> 1);
        if (c >= 0x20 && c <= 0x7E)
            callsign += c;
    }

    uint8_t lastByte = static_cast<uint8_t>(data[offset + 6]);
    ssid = (lastByte >> 1) & 0x0F;
    callsign = callsign.trimmed();

    if (!callsign.isEmpty() && ssid > 0)
        callsign += QString("-%1").arg(ssid);

    return {callsign, ssid};
}

QPair<QString, QString> ChatController::parseAx25UiFrame(const QByteArray& ax25)
{
    if (ax25.size() < 16)
        return {};

    uint8_t ctrl = static_cast<uint8_t>(ax25[14]);
    uint8_t pid  = static_cast<uint8_t>(ax25[15]);

    if (ctrl != 0x03 && ctrl != 0x13)
        return {};

    if (pid != 0xF0 && pid != 0xCF && pid != 0xCD)
        return {};

    auto [srcCallsign, srcSsid] = decodeAx25Address(ax25, 7);

    QString info;
    if (ax25.size() > 16)
        info = QString::fromUtf8(ax25.mid(16));

    return {srcCallsign, info};
}

// ============================================================================
// Channel-aware text format:  "#channelName message text" or plain text → "main"
// ============================================================================

QPair<QString, QString> ChatController::parseChannelFromText(const QString& info)
{
    if (info.startsWith('#') && info.length() > 1) {
        int spaceIdx = info.indexOf(' ');
        if (spaceIdx > 1) {
            QString ch = info.mid(1, spaceIdx - 1);
            QString msg = info.mid(spaceIdx + 1);
            return {ch, msg};
        } else {
            // "#channel" with no message text
            return {info.mid(1), QString()};
        }
    }
    return {"main", info};
}

// ============================================================================
// ChatController
// ============================================================================

ChatController::ChatController(QObject* parent)
    : QObject(parent)
{
}

void ChatController::setTnc(ITnc* tnc)
{
    if (m_tnc)
        disconnect(m_tnc, nullptr, this, nullptr);

    m_tnc = tnc;

    if (m_tnc) {
        connect(m_tnc, &ITnc::frameReceived, this, &ChatController::onFrameReceived);
        connect(m_tnc, &ITnc::connected, this, &ChatController::connected);
        connect(m_tnc, &ITnc::disconnected, this, &ChatController::disconnected);
        connect(m_tnc, &ITnc::errorOccurred, this, &ChatController::errorOccurred);
    }
}

ITnc* ChatController::tnc() const { return m_tnc; }
void ChatController::setCallsign(const QString& callsign) { m_callsign = callsign; }
QString ChatController::callsign() const { return m_callsign; }
void ChatController::setCurrentChannel(const QString& channel) { m_currentChannel = channel; }
QString ChatController::currentChannel() const { return m_currentChannel; }

bool ChatController::isConnected() const
{
    return m_tnc && m_tnc->isConnected();
}

void ChatController::connectTnc()
{
    if (m_tnc)
        m_tnc->connectTnc();
}

void ChatController::disconnectTnc()
{
    if (m_tnc)
        m_tnc->disconnectTnc();
}

void ChatController::sendMessage(const QString& text)
{
    if (!m_tnc || !m_tnc->isConnected() || text.trimmed().isEmpty())
        return;

    QByteArray frame = buildFrame(text.trimmed());
    m_tnc->sendFrame(frame);

    Message localEcho;
    localEcho.callsign = m_callsign;
    localEcho.text = text.trimmed();
    localEcho.channel = m_currentChannel;
    localEcho.timestamp = QDateTime::currentDateTime();
    localEcho.type = Message::Text;
    emit messageReceived(localEcho);
}

void ChatController::onFrameReceived(const QByteArray& wireFrame)
{
    auto [command, data] = KissFrame::decode(wireFrame);
    if (command != KissFrame::CMD_DATA || data.isEmpty())
        return;

    auto [callsign, info] = parseAx25UiFrame(data);
    if (callsign.isEmpty())
        return;

    auto [channel, text] = parseChannelFromText(info);

    Message msg;
    msg.callsign = callsign;
    msg.text = text;
    msg.channel = channel;
    msg.timestamp = QDateTime::currentDateTime();
    msg.type = Message::Text;

    emit messageReceived(msg);
}

QByteArray ChatController::buildFrame(const QString& text) const
{
    // Prepend channel tag: "#channelName message text"
    QString payload = QString("#%1 %2").arg(m_currentChannel, text);

    QByteArray ax25;
    ax25.append(encodeAx25Address("CQ",      0, false));
    ax25.append(encodeAx25Address(m_callsign, 0, true));
    ax25.append(static_cast<char>(0x03));
    ax25.append(static_cast<char>(0xF0));
    ax25.append(payload.toUtf8());

    return KissFrame::encode(KissFrame::CMD_DATA, ax25);
}
