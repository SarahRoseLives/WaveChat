#pragma once
#include <QByteArray>
#include <QPair>

struct KissFrame {
    static constexpr uint8_t FEND     = 0xC0;
    static constexpr uint8_t FESC     = 0xDB;
    static constexpr uint8_t TFEND    = 0xDC;
    static constexpr uint8_t TFESC    = 0xDD;
    static constexpr uint8_t CMD_DATA   = 0x00;
    static constexpr uint8_t CMD_TXDELAY = 0x01;
    static constexpr uint8_t CMD_P       = 0x02;
    static constexpr uint8_t CMD_SLOTTIME = 0x03;
    static constexpr uint8_t CMD_TXTAIL  = 0x04;
    static constexpr uint8_t CMD_FULLDUPLEX = 0x05;
    static constexpr uint8_t CMD_RETURN  = 0xFF;

    static QByteArray encode(uint8_t command, const QByteArray& data);
    static QPair<uint8_t, QByteArray> decode(const QByteArray& wireFrame);

private:
    static QByteArray escape(const QByteArray& data);
    static QByteArray unescape(const QByteArray& data);
};
