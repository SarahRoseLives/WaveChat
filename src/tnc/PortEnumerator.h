#pragma once
#include <QString>
#include <QList>

struct PortInfo {
    QString portName;       // e.g., "COM7"
    QString systemLocation; // e.g., "\\\\.\\COM7"
    QString description;    // e.g., "Standard Serial over Bluetooth link"
    QString friendlyName;   // e.g., "VR-N76" — resolved BT device name, or empty
    QString displayText;    // e.g., "COM7 — VR-N76" — ready for UI
    int vid = 0;
    int pid = 0;
};

class PortEnumerator {
public:
    static QList<PortInfo> enumerate();
};
