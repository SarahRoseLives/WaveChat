#pragma once
#include <QObject>
#include <QString>
#include <QMap>
#include <QSet>
#include <QTimer>
#include <QFile>
#include <QByteArray>
#include <QRandomGenerator>
#include "chat/Message.h"

// ---------------------------------------------------------------------------
// FileTransferManager — batched burst + gap-fill with accept handshake
//
// Sender:     Idle → Offering → Bursting → AwaitingMissing → GapFilling → Done
// Receiver:   Idle → Offered → Transferring → Done
// ---------------------------------------------------------------------------
class FileTransferManager : public QObject {
    Q_OBJECT
public:
    explicit FileTransferManager(QObject* parent = nullptr);

    void startSend(const QString& filePath);
    void cancelSend();
    void acceptReceive(const QString& fileId);
    void rejectReceive(const QString& fileId);
    void processIncomingFrame(const QString& info, const QString& fromCallsign);
    int sendState() const { return static_cast<int>(m_sendState); }

signals:
    void sendRawFrame(const QByteArray& payload);
    void fileOfferReceived(const FileTransferInfo& info);
    void fileProgressUpdate(const QString& fileId, int received, int total);
    void fileSendProgress(int sent, int total);
    void fileBurstStarted();                              // sender: burst began
    void fileComplete(const FileTransferInfo& info);
    void fileSendComplete(const QString& fileId);         // only after !done from receiver
    void fileFailed(const QString& fileId, const QString& reason);

private slots:
    void sendBurstChunk();
    void onBurstEnd();
    void onMissingTimeout();
    void onReceiverSilence();
    void onOfferTimeout();

private:
    // -- Sender --
    enum SendState { Idle, Offering, Bursting, AwaitingMissing, GapFilling, SendDone, SendFailed };
    SendState m_sendState = Idle;
    QString   m_sendFileId;
    QByteArray m_sendData;
    QString   m_sendFileName;
    int       m_sendTotalChunks = 0;
    int       m_sendBurstSeq = 0;
    int       m_sendGapRounds = 0;
    int       m_sendOfferRetries = 0;
    static constexpr int OFFER_RETRY_MS = 5000;
    static constexpr int MAX_OFFER_RETRIES = 6; // 30s total
    static constexpr int BURST_GAP_MS = 1500;
    static constexpr int MISSING_TIMEOUT_MS = 12000;
    static constexpr int MAX_GAP_ROUNDS = 3;

    QTimer* m_burstTimer = nullptr;
    QTimer* m_offerTimer = nullptr;
    QTimer* m_missingTimer = nullptr;

    // -- Receiver --
    struct RxState {
        FileTransferInfo info;
        QMap<int, QByteArray> chunks;
        QTimer* silenceTimer = nullptr;
        QTimer* offerTimer = nullptr;
        QTimer* acceptRetryTimer = nullptr;
        int gapRounds = 0;
        int acceptRetries = 0;
        static constexpr int ACCEPT_RETRY_MS = 3000;
        static constexpr int MAX_ACCEPT_RETRIES = 10;
    };
    QMap<QString, RxState> m_receivers;
    static constexpr int RX_SILENCE_MS = 8000;
    static constexpr int RX_MAX_GAP_ROUNDS = 3;

    // -- Helpers --
    static QString makeFileId(const QString& filePath);
    static int jitter(int baseMs);
    static QByteArray buildChunk(const QString& fileId, int seq, int total, const QByteArray& raw);
    static QList<int> parseMissingSeqs(const QString& list);
    QString estimateTime(int chunks) const;

public:
    static constexpr int CHUNK_RAW_BYTES = 90;
    static constexpr int CHUNK_B64_BYTES = 120;
    static constexpr int DEFAULT_BAUD = 1200;
};
