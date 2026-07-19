#include "FileTransfer.h"
#include "util/ProtocolLogger.h"
#include <QFileInfo>
#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QStandardPaths>

// ============================================================================
// Helpers
// ============================================================================

QString FileTransferManager::makeFileId(const QString& filePath)
{
    QFileInfo fi(filePath);
    QString seed = fi.fileName() + QString::number(fi.size())
                   + QString::number(QDateTime::currentMSecsSinceEpoch());
    return QCryptographicHash::hash(seed.toUtf8(), QCryptographicHash::Sha1)
        .toHex().left(8);
}

int FileTransferManager::jitter(int baseMs)
{
    int v = (QRandomGenerator::global()->bounded(41) - 20) * baseMs / 100;
    return qMax(20, baseMs + v);
}

QByteArray FileTransferManager::buildChunk(const QString& fileId, int seq, int total,
                                             const QByteArray& raw)
{
    QString b64 = QString::fromLatin1(raw.toBase64());
    return ("!chunk " + fileId + " " + QString::number(seq) + "/"
            + QString::number(total) + " " + b64).toUtf8();
}

QList<int> FileTransferManager::parseMissingSeqs(const QString& list)
{
    QList<int> result;
    for (const QString& s : list.split(',', Qt::SkipEmptyParts)) {
        bool ok = false;
        int n = s.trimmed().toInt(&ok);
        if (ok) result.append(n);
    }
    return result;
}

QString FileTransferManager::estimateTime(int chunks) const
{
    double secs = chunks * 0.2 + 12.0;
    if (secs < 60)  return QString("%1s").arg((int)secs);
    if (secs < 3600) return QString("%1m %2s").arg((int)secs / 60).arg((int)secs % 60);
    return QString("%1h %2m").arg((int)secs / 3600).arg(((int)secs % 3600) / 60);
}

// ============================================================================
// Constructor
// ============================================================================

FileTransferManager::FileTransferManager(QObject* parent)
    : QObject(parent)
{
    m_burstTimer = new QTimer(this);
    m_burstTimer->setSingleShot(true);
    connect(m_burstTimer, &QTimer::timeout, this, &FileTransferManager::sendBurstChunk);

    m_offerTimer = new QTimer(this);
    m_offerTimer->setSingleShot(true);
    connect(m_offerTimer, &QTimer::timeout, this, &FileTransferManager::onOfferTimeout);

    m_missingTimer = new QTimer(this);
    m_missingTimer->setSingleShot(true);
    connect(m_missingTimer, &QTimer::timeout, this, &FileTransferManager::onMissingTimeout);
}

static const char* stateName(FileTransferManager* ft)
{
    switch (static_cast<int>(ft->sendState())) {
    case 0: return "Idle";
    case 1: return "Offering";
    case 2: return "Bursting";
    case 3: return "AwaitingMissing";
    case 4: return "GapFilling";
    case 5: return "SendDone";
    case 6: return "SendFailed";
    default: return "?";
    }
}

// ============================================================================
// Sender — Phase 0: Offer (wait for !accept)
// ============================================================================

void FileTransferManager::startSend(const QString& filePath)
{
    if (m_sendState != Idle && m_sendState != SendFailed && m_sendState != SendDone) {
        emit fileFailed(m_sendFileId, "A transfer is already in progress.");
        return;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        emit fileFailed({}, "Cannot open file: " + filePath);
        return;
    }

    m_sendData = file.readAll();
    file.close();

    QFileInfo fi(filePath);
    m_sendFileName = fi.fileName();
    m_sendTotalChunks = (m_sendData.size() + CHUNK_RAW_BYTES - 1) / CHUNK_RAW_BYTES;
    if (m_sendTotalChunks == 0) m_sendTotalChunks = 1;

    m_sendFileId = makeFileId(filePath);
    m_sendBurstSeq = 0;
    m_sendGapRounds = 0;
    m_sendOfferRetries = 0;
    m_sendState = Offering;

    ProtocolLogger::log("FT TX", QString("!burst %1 %2 size=%3 chunks=%4")
                        .arg(m_sendFileId, m_sendFileName)
                        .arg(m_sendData.size()).arg(m_sendTotalChunks));
    ProtocolLogger::log("FT STATE", stateName(this));

    QString burst = "!burst " + m_sendFileId + " " + m_sendFileName + " "
                    + QString::number(m_sendData.size()) + " "
                    + QString::number(m_sendTotalChunks);
    emit sendRawFrame(burst.toUtf8());

    m_offerTimer->start(OFFER_RETRY_MS);
}

void FileTransferManager::onOfferTimeout()
{
    if (m_sendState != Offering) return;

    if (++m_sendOfferRetries >= MAX_OFFER_RETRIES) {
        m_sendState = SendFailed;
        ProtocolLogger::log("FT STATE", QString("SendFailed (no accept, %1 retries)").arg(m_sendOfferRetries));
        emit fileFailed(m_sendFileId, "No receiver accepted the offer.");
        m_sendState = Idle;
        m_sendData.clear();
        return;
    }

    // Re-send !burst — retry the handshake
    ProtocolLogger::log("FT TX", QString("!burst (retry %1/%2)").arg(m_sendOfferRetries).arg(MAX_OFFER_RETRIES));
    QString burst = "!burst " + m_sendFileId + " " + m_sendFileName + " "
                    + QString::number(m_sendData.size()) + " "
                    + QString::number(m_sendTotalChunks);
    emit sendRawFrame(burst.toUtf8());

    m_offerTimer->start(OFFER_RETRY_MS);
}

// ============================================================================
// Sender — Phase 1: Burst (all chunks, then !endburst)
// ============================================================================

void FileTransferManager::sendBurstChunk()
{
    if (m_sendState != Bursting)
        return;

    if (m_sendBurstSeq >= m_sendTotalChunks) {
        onBurstEnd();
        return;
    }

    int offset = m_sendBurstSeq * CHUNK_RAW_BYTES;
    int size = qMin(CHUNK_RAW_BYTES, m_sendData.size() - offset);
    QByteArray raw = m_sendData.mid(offset, size);

    emit sendRawFrame(buildChunk(m_sendFileId, m_sendBurstSeq, m_sendTotalChunks, raw));
    emit fileSendProgress(m_sendBurstSeq + 1, m_sendTotalChunks);

    m_sendBurstSeq++;
    m_burstTimer->start(jitter(BURST_GAP_MS));
}

void FileTransferManager::onBurstEnd()
{
    ProtocolLogger::log("FT TX", "!endburst " + m_sendFileId);
    emit sendRawFrame(QString("!endburst %1").arg(m_sendFileId).toUtf8());
    m_sendState = AwaitingMissing;
    ProtocolLogger::log("FT STATE", stateName(this));
    m_missingTimer->start(jitter(MISSING_TIMEOUT_MS));
}

// ============================================================================
// Sender — Phase 2 & 3: AwaitingMissing / GapFilling
// ============================================================================

void FileTransferManager::onMissingTimeout()
{
    if (m_sendState != AwaitingMissing) return;

    if (++m_sendGapRounds >= MAX_GAP_ROUNDS) {
        m_sendState = SendFailed;
        ProtocolLogger::log("FT STATE", QString("SendFailed (missing timeout, %1 rounds)").arg(m_sendGapRounds));
        emit fileFailed(m_sendFileId, "No response from receiver.");
        m_sendState = Idle;
        m_sendData.clear();
        return;
    }

    ProtocolLogger::log("FT TX", QString("!endburst (ping, round %1/%2)").arg(m_sendGapRounds).arg(MAX_GAP_ROUNDS));
    emit sendRawFrame(QString("!endburst %1").arg(m_sendFileId).toUtf8());
    m_missingTimer->start(jitter(MISSING_TIMEOUT_MS));
}

void FileTransferManager::cancelSend()
{
    if (m_sendState == Idle || m_sendState == SendFailed || m_sendState == SendDone)
        return;
    m_burstTimer->stop();
    m_offerTimer->stop();
    m_missingTimer->stop();
    m_sendState = SendFailed;
    emit sendRawFrame(QString("!cancel %1").arg(m_sendFileId).toUtf8());
    ProtocolLogger::log("FT TX", "!cancel " + m_sendFileId);
    emit fileFailed(m_sendFileId, "Transfer cancelled.");
    m_sendState = Idle;
    m_sendData.clear();
}

// ============================================================================
// Receiver — Accept / Reject
// ============================================================================

void FileTransferManager::acceptReceive(const QString& fileId)
{
    auto it = m_receivers.find(fileId);
    if (it == m_receivers.end()) return;

    it->offerTimer->stop();
    it->info.state = FileTransferInfo::Transferring;
    emit fileProgressUpdate(fileId, 0, it->info.totalChunks);

    it->silenceTimer = new QTimer(this);
    it->silenceTimer->setSingleShot(true);
    QObject::connect(it->silenceTimer, &QTimer::timeout, this, [this]() {
        onReceiverSilence();
    });
    it->silenceTimer->start(RX_SILENCE_MS);

    // Send !accept and retry until we get the first chunk or exhaust retries
    auto sendAccept = [this, fileId]() {
        auto rx = m_receivers.find(fileId);
        if (rx == m_receivers.end()) return;
        emit sendRawFrame(QString("!accept %1").arg(fileId).toUtf8());
    };
    sendAccept();

    it->acceptRetryTimer = new QTimer(this);
    it->acceptRetries = 0;
    QObject::connect(it->acceptRetryTimer, &QTimer::timeout, this, [this, fileId]() {
        auto rx = m_receivers.find(fileId);
        if (rx == m_receivers.end()) return;
        if (rx->acceptRetries >= RxState::MAX_ACCEPT_RETRIES) {
            rx->acceptRetryTimer->stop();
            emit fileFailed(fileId, "Sender did not acknowledge accept.");
            if (rx->silenceTimer) { rx->silenceTimer->stop(); delete rx->silenceTimer; }
            m_receivers.remove(fileId);
            return;
        }
        rx->acceptRetries++;
        ProtocolLogger::log("FT TX", QString("!accept %1 (retry %2)").arg(fileId).arg(rx->acceptRetries));
        emit sendRawFrame(QString("!accept %1").arg(fileId).toUtf8());
    });
    it->acceptRetryTimer->start(RxState::ACCEPT_RETRY_MS);
    ProtocolLogger::log("FT TX", "!accept " + fileId + " (retry timer started)");
}

void FileTransferManager::rejectReceive(const QString& fileId)
{
    auto it = m_receivers.find(fileId);
    if (it == m_receivers.end()) return;

    if (it->offerTimer) { it->offerTimer->stop(); delete it->offerTimer; }
    if (it->silenceTimer) { it->silenceTimer->stop(); delete it->silenceTimer; }
    emit sendRawFrame(QString("!cancel %1").arg(fileId).toUtf8());
    m_receivers.erase(it);
}

// ============================================================================
// Receiver — Gap check
// ============================================================================

void FileTransferManager::onReceiverSilence()
{
    for (auto it = m_receivers.begin(); it != m_receivers.end(); ++it) {
        if (it->info.state != FileTransferInfo::Transferring)
            continue;

        if (it->gapRounds >= RX_MAX_GAP_ROUNDS) {
            emit fileFailed(it.key(), "Max gap-fill rounds reached.");
            if (it->silenceTimer) { it->silenceTimer->stop(); delete it->silenceTimer; }
            m_receivers.erase(it);
            return;
        }

        QStringList missingList;
        for (int i = 0; i < it->info.totalChunks; ++i) {
            if (!it->chunks.contains(i))
                missingList.append(QString::number(i));
        }

        if (missingList.isEmpty()) {
            ProtocolLogger::log("FT STATE", QString("Receiver: all %1 chunks present -> done").arg(it->info.totalChunks));
            QByteArray all;
            for (int i = 0; i < it->info.totalChunks; ++i)
                all.append(it->chunks[i]);

            QString dir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)
                          + "/WaveChat/downloads";
            QDir().mkpath(dir);
            QString savePath = dir + "/" + it->info.fileName;

            int suffix = 1;
            QString base = savePath;
            while (QFile::exists(savePath)) {
                QFileInfo fi(base);
                savePath = fi.absolutePath() + "/" + fi.completeBaseName()
                           + "_" + QString::number(suffix++)
                           + (fi.suffix().isEmpty() ? "" : "." + fi.suffix());
            }

            QFile out(savePath);
            if (out.open(QIODevice::WriteOnly)) {
                out.write(all);
                out.close();
            }

            it->info.state = FileTransferInfo::Complete;
            it->info.savePath = savePath;
            emit fileComplete(it->info);
            ProtocolLogger::log("FT TX", "!done " + it.key());
            emit sendRawFrame(QString("!done %1").arg(it.key()).toUtf8());

            if (it->silenceTimer) { it->silenceTimer->stop(); delete it->silenceTimer; }
            m_receivers.erase(it);
            return;
        }

        it->gapRounds++;
        ProtocolLogger::log("FT TX", QString("!missing %1 gaps=%2").arg(it.key()).arg(missingList.size()));
        emit sendRawFrame(QString("!missing %1 %2")
                              .arg(it.key())
                              .arg(missingList.join(',')).toUtf8());
        it->silenceTimer->start(RX_SILENCE_MS);
    }
}

// ============================================================================
// Incoming frame dispatcher
// ============================================================================

void FileTransferManager::processIncomingFrame(const QString& info, const QString& fromCallsign)
{
    Q_UNUSED(fromCallsign);

    if (!info.startsWith('!')) return;

    auto parts = info.split(' ');
    if (parts.isEmpty()) return;

    QString cmd = parts[0];

    // ========================================================================
    // Receiver side: !burst  → show accept dialog
    // ========================================================================
    if (cmd == "!burst" && parts.size() >= 5) {
        QString fileId   = parts[1];
        QString fileName = parts[2];
        qint64  fileSize = parts[3].toLongLong();
        int     chunks   = parts[4].toInt();

        if (m_receivers.contains(fileId)) return;

        RxState rx;
        rx.info.fileId       = fileId;
        rx.info.fileName     = fileName;
        rx.info.fileSize     = fileSize;
        rx.info.totalChunks  = chunks;
        rx.info.fromCallsign = fromCallsign;
        rx.info.state        = FileTransferInfo::Offering;

        rx.offerTimer = new QTimer(this);
        rx.offerTimer->setSingleShot(true);
        QObject::connect(rx.offerTimer, &QTimer::timeout, this, [this, fileId]() {
            auto it = m_receivers.find(fileId);
            if (it != m_receivers.end() && it->info.state == FileTransferInfo::Offering) {
                m_receivers.erase(it);
            }
        });
        rx.offerTimer->start(30000);

        m_receivers.insert(fileId, rx);
        ProtocolLogger::log("FT RX", QString("!burst %1 %2 size=%3 chunks=%4").arg(fileId, fileName).arg(fileSize).arg(chunks));
        emit fileOfferReceived(rx.info);
        return;
    }

    // ========================================================================
    // Receiver side: !chunk
    // ========================================================================
    if (cmd == "!chunk" && parts.size() >= 4) {
        QString fileId = parts[1];
        auto seqTotal  = parts[2].split('/');
        if (seqTotal.size() < 2) return;
        int seq   = seqTotal[0].toInt();
        int total = seqTotal[1].toInt();

        auto it = m_receivers.find(fileId);
        if (it == m_receivers.end()) return;

        QString b64 = info.section(' ', 3);
        QByteArray raw = QByteArray::fromBase64(b64.toLatin1());

        if (!it->chunks.contains(seq)) {
            it->chunks[seq] = raw;
            it->info.receivedChunks = it->chunks.size();
            it->info.totalChunks = total;
            it->info.state = FileTransferInfo::Transferring;
            emit fileProgressUpdate(fileId, it->info.receivedChunks, total);
        }

        if (it->silenceTimer)
            it->silenceTimer->start(RX_SILENCE_MS);

        // Stop accept retries — sender clearly got the accept
        if (it->acceptRetryTimer) {
            it->acceptRetryTimer->stop();
            delete it->acceptRetryTimer;
            it->acceptRetryTimer = nullptr;
        }

        return;
    }

    // ========================================================================
    // Receiver side: !endburst  → immediate gap check
    // ========================================================================
    if (cmd == "!endburst" && parts.size() >= 2) {
        QString fileId = parts[1];
        auto it = m_receivers.find(fileId);
        if (it == m_receivers.end()) return;

        ProtocolLogger::log("FT RX", "!endburst " + fileId + " -> gap check");
        if (it->silenceTimer) it->silenceTimer->stop();
        QTimer::singleShot(jitter(300), this, &FileTransferManager::onReceiverSilence);
        return;
    }

    // ========================================================================
    // Receiver side: !cancel
    // ========================================================================
    if (cmd == "!cancel" && parts.size() >= 2) {
        QString fileId = parts[1];
        auto it = m_receivers.find(fileId);
        if (it != m_receivers.end()) {
            it->info.state = FileTransferInfo::Cancelled;
            emit fileFailed(fileId, "Sender cancelled.");
            if (it->silenceTimer) { it->silenceTimer->stop(); delete it->silenceTimer; }
            if (it->offerTimer)   { it->offerTimer->stop(); delete it->offerTimer; }
            m_receivers.erase(it);
        }
        return;
    }

    // ========================================================================
    // Sender side: !accept  → receiver accepted, start burst
    // ========================================================================
    if (cmd == "!accept" && parts.size() >= 2) {
        QString fileId = parts[1];
        if (fileId != m_sendFileId || m_sendState != Offering) return;

        m_offerTimer->stop();
        m_sendState = Bursting;
        ProtocolLogger::log("FT RX", "!accept " + fileId);
        ProtocolLogger::log("FT STATE", stateName(this));
        emit fileBurstStarted();
        m_burstTimer->start(jitter(BURST_GAP_MS));
        return;
    }

    // ========================================================================
    // Sender side: !missing  → retransmit listed chunks
    // ========================================================================
    if (cmd == "!missing" && parts.size() >= 3) {
        QString fileId = parts[1];
        QString seqList = info.section(' ', 2);

        if (fileId != m_sendFileId || m_sendState != AwaitingMissing) return;

        m_missingTimer->stop();
        m_sendState = GapFilling;

        QList<int> missing = parseMissingSeqs(seqList);
        ProtocolLogger::log("FT RX", QString("!missing %1 gaps=%2").arg(fileId).arg(missing.size()));
        ProtocolLogger::log("FT STATE", QString("GapFilling (%1 chunks)").arg(missing.size()));
        if (missing.isEmpty()) {
            m_sendState = SendDone;
            emit fileSendComplete(m_sendFileId);
            m_sendState = Idle;
            m_sendData.clear();
            return;
        }

        // Signal gap-fill mode: sent=0 means "starting retransmit of N chunks"
        emit fileSendProgress(0, missing.size());

        int delay = jitter(BURST_GAP_MS);
        int gapChunk = 0;
        for (int seq : missing) {
            int offset = seq * CHUNK_RAW_BYTES;
            int size = qMin(CHUNK_RAW_BYTES, m_sendData.size() - offset);
            QByteArray raw = m_sendData.mid(offset, size);

            QTimer::singleShot(delay, this, [this, seq, raw, gapChunk]() {
                if (m_sendState != GapFilling) return;
                emit sendRawFrame(buildChunk(m_sendFileId, seq, m_sendTotalChunks, raw));
            });

            delay += jitter(BURST_GAP_MS);
            gapChunk++;
        }

        QTimer::singleShot(delay, this, [this]() {
            if (m_sendState != GapFilling) return;
            onBurstEnd();
        });

        return;
    }

    // ========================================================================
    // Sender side: !cancel → receiver declined or cancelled
    // ========================================================================
    if (cmd == "!cancel" && parts.size() >= 2) {
        if (parts[1] == m_sendFileId && (m_sendState == Offering || m_sendState == Bursting
                                          || m_sendState == AwaitingMissing
                                          || m_sendState == GapFilling)) {
            m_offerTimer->stop();
            m_burstTimer->stop();
            m_missingTimer->stop();
            m_sendState = SendFailed;
            ProtocolLogger::log("FT RX", "!cancel " + m_sendFileId);
            ProtocolLogger::log("FT STATE", "SendFailed (cancelled by receiver)");
            emit fileFailed(m_sendFileId, "Receiver cancelled the transfer.");
            m_sendState = Idle;
            m_sendData.clear();
        }
        return;
    }

    // ========================================================================
    // Sender side: !done  → transfer confirmed complete
    // ========================================================================
    if (cmd == "!done" && parts.size() >= 2) {
        if (parts[1] == m_sendFileId && m_sendState == AwaitingMissing) {
            m_missingTimer->stop();
            m_sendState = SendDone;
            ProtocolLogger::log("FT RX", "!done " + m_sendFileId);
            ProtocolLogger::log("FT STATE", "Complete");
            emit fileSendComplete(m_sendFileId);
            m_sendState = Idle;
            m_sendData.clear();
        }
        return;
    }
}
