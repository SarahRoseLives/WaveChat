#include "PortEnumerator.h"
#include <QSerialPortInfo>
#include <QSet>

#ifdef Q_OS_WIN
#include <windows.h>
#include <map>
#include <string>

static std::wstring readRegStringW(HKEY key, const wchar_t* valueName)
{
    wchar_t buf[512] = {};
    DWORD len = sizeof(buf);
    DWORD type = 0;
    if (RegQueryValueExW(key, valueName, nullptr, &type,
                         reinterpret_cast<LPBYTE>(buf), &len) == ERROR_SUCCESS) {
        if (type == REG_SZ || type == REG_EXPAND_SZ)
            return std::wstring(buf);
    }
    return {};
}

static std::string wstrToUtf8(const std::wstring& ws)
{
    if (ws.empty()) return {};
    int len = WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (len <= 0) return {};
    std::string result(len - 1, '\0');
    WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), -1, result.data(), len, nullptr, nullptr);
    return result;
}

static QString wstrToQ(const std::wstring& ws)
{
    if (ws.empty()) return {};
    return QString::fromWCharArray(ws.c_str(), static_cast<int>(ws.size()));
}

#endif // Q_OS_WIN

QList<PortInfo> PortEnumerator::enumerate()
{
    QList<PortInfo> result;

#ifdef Q_OS_WIN
    std::map<std::string, QString> addr_to_name;
    std::map<std::string, QString> addr_to_port;

    // === Pass 1 & 2: Enumerate Bluetooth TNC devices ===
    HKEY bthenum = nullptr;
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                      L"SYSTEM\\CurrentControlSet\\Enum\\BTHENUM",
                      0, KEY_READ, &bthenum) == ERROR_SUCCESS) {

        wchar_t subkeyName[512];
        for (DWORD i = 0; ; ++i) {
            DWORD len = sizeof(subkeyName) / sizeof(wchar_t);
            if (RegEnumKeyExW(bthenum, i, subkeyName, &len, nullptr, nullptr, nullptr, nullptr)
                != ERROR_SUCCESS)
                break;

            std::wstring subkey(subkeyName, len);

            // --- Pass 1: Dev_ keys → device friendly names ---
            if (subkey.rfind(L"Dev_", 0) == 0 && subkey.length() >= 4 + 12) {
                std::wstring addrW = subkey.substr(4, 12);
                std::string addr = wstrToUtf8(addrW);

                HKEY devKey = nullptr;
                if (RegOpenKeyExW(bthenum, subkeyName, 0, KEY_READ, &devKey) == ERROR_SUCCESS) {
                    // Enumerate instances under Dev_ key
                    wchar_t instName[512];
                    for (DWORD j = 0; ; ++j) {
                        DWORD ilen = sizeof(instName) / sizeof(wchar_t);
                        if (RegEnumKeyExW(devKey, j, instName, &ilen,
                                          nullptr, nullptr, nullptr, nullptr) != ERROR_SUCCESS)
                            break;

                        HKEY instKey = nullptr;
                        if (RegOpenKeyExW(devKey, instName, 0, KEY_READ, &instKey) == ERROR_SUCCESS) {
                            std::wstring friendly = readRegStringW(instKey, L"FriendlyName");
                            if (!friendly.empty()) {
                                addr_to_name[addr] = wstrToQ(friendly);
                            }
                            RegCloseKey(instKey);
                        }
                    }
                    RegCloseKey(devKey);
                }
            }

            // --- Pass 2: RFCOMM service keys → COM port mapping ---
            if (subkey.find(L"00001101") != std::wstring::npos) {
                HKEY devClass = nullptr;
                if (RegOpenKeyExW(bthenum, subkeyName, 0, KEY_READ, &devClass) == ERROR_SUCCESS) {

                    wchar_t instName[512];
                    for (DWORD j = 0; ; ++j) {
                        DWORD ilen = sizeof(instName) / sizeof(wchar_t);
                        if (RegEnumKeyExW(devClass, j, instName, &ilen,
                                          nullptr, nullptr, nullptr, nullptr) != ERROR_SUCCESS)
                            break;

                        HKEY instKey = nullptr;
                        if (RegOpenKeyExW(devClass, instName, 0, KEY_READ, &instKey) == ERROR_SUCCESS) {
                            std::wstring friendly = readRegStringW(instKey, L"FriendlyName");
                            std::wstring instance(instName, ilen);

                            // Extract BT address: 12 chars before last '_'
                            auto us = instance.rfind(L'_');
                            if (us != std::wstring::npos && us >= 12) {
                                std::wstring addrW = instance.substr(us - 12, 12);
                                std::string addr = wstrToUtf8(addrW);

                                // Skip zero/null addresses
                                if (addr != "000000000000" && !friendly.empty()) {
                                    // Parse COM port from FriendlyName: e.g. "...(COM6)"
                                    auto lp = friendly.rfind(L"(COM");
                                    auto rp = friendly.rfind(L')');
                                    if (lp != std::wstring::npos && rp != std::wstring::npos
                                        && rp > lp) {
                                        QString comPort = wstrToQ(friendly.substr(lp + 1, rp - lp - 1));
                                        if (!comPort.isEmpty()) {
                                            addr_to_port[addr] = comPort;
                                        }
                                    }
                                }
                            }
                            RegCloseKey(instKey);
                        }
                    }
                    RegCloseKey(devClass);
                }
            }
        }
        RegCloseKey(bthenum);
    }

    // === Merge: produce PortInfo for each BT TNC found ===
    QSet<QString> btPortsSeen;
    for (const auto& [addr, port] : addr_to_port) {
        PortInfo info;
        info.portName = port;
        info.systemLocation = QString("\\\\.\\%1").arg(port);
        info.description = "Bluetooth Serial";

        auto nameIt = addr_to_name.find(addr);
        if (nameIt != addr_to_name.end() && !nameIt->second.isEmpty()) {
            info.friendlyName = nameIt->second;
        } else {
            info.friendlyName = QString::fromStdString("BT:" + addr).left(20);
        }

        info.displayText = QString("%1 \u2014 %2").arg(info.portName, info.friendlyName);
        btPortsSeen.insert(port);
        result.append(info);
    }
#endif // Q_OS_WIN

    // === Non-Bluetooth ports via QSerialPortInfo ===
    const auto ports = QSerialPortInfo::availablePorts();
    for (const auto& p : ports) {
#ifdef Q_OS_WIN
        if (btPortsSeen.contains(p.portName()))
            continue; // Already handled via BT enumeration above
#endif

        PortInfo info;
        info.portName = p.portName();
        info.systemLocation = p.systemLocation();
        info.description = p.description();
        info.vid = p.vendorIdentifier();
        info.pid = p.productIdentifier();

        if (!info.description.isEmpty() && info.description != info.portName) {
            info.displayText = QString("%1 \u2014 %2").arg(info.portName, info.description);
        } else {
            info.displayText = info.portName;
        }

        result.append(info);
    }

    return result;
}
