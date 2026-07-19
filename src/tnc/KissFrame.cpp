#include "KissFrame.h"

QByteArray KissFrame::encode(uint8_t command, const QByteArray& data)
{
    QByteArray payload;
    payload.append(static_cast<char>(command));
    payload.append(data);

    QByteArray escaped = escape(payload);
    QByteArray frame;
    frame.append(static_cast<char>(FEND));
    frame.append(escaped);
    frame.append(static_cast<char>(FEND));
    return frame;
}

QPair<uint8_t, QByteArray> KissFrame::decode(const QByteArray& wireFrame)
{
    if (wireFrame.size() < 2)
        return {0xFF, {}};
    if (static_cast<uint8_t>(wireFrame[0]) != FEND)
        return {0xFF, {}};
    if (static_cast<uint8_t>(wireFrame[wireFrame.size() - 1]) != FEND)
        return {0xFF, {}};

    QByteArray inner = wireFrame.mid(1, wireFrame.size() - 2);
    QByteArray deescaped = unescape(inner);

    if (deescaped.isEmpty())
        return {0xFF, {}};

    uint8_t command = static_cast<uint8_t>(deescaped[0]);
    QByteArray data = deescaped.mid(1);
    return {command, data};
}

QByteArray KissFrame::escape(const QByteArray& data)
{
    QByteArray out;
    for (int i = 0; i < data.size(); ++i) {
        uint8_t b = static_cast<uint8_t>(data[i]);
        if (b == FEND) {
            out.append(static_cast<char>(FESC));
            out.append(static_cast<char>(TFEND));
        } else if (b == FESC) {
            out.append(static_cast<char>(FESC));
            out.append(static_cast<char>(TFESC));
        } else {
            out.append(data[i]);
        }
    }
    return out;
}

QByteArray KissFrame::unescape(const QByteArray& data)
{
    QByteArray out;
    for (int i = 0; i < data.size(); ++i) {
        uint8_t b = static_cast<uint8_t>(data[i]);
        if (b == FESC) {
            if (i + 1 >= data.size())
                break;
            uint8_t next = static_cast<uint8_t>(data[++i]);
            if (next == TFEND)
                out.append(static_cast<char>(FEND));
            else if (next == TFESC)
                out.append(static_cast<char>(FESC));
            else {
                out.append(static_cast<char>(FESC));
                out.append(static_cast<char>(next));
            }
        } else {
            out.append(data[i]);
        }
    }
    return out;
}
