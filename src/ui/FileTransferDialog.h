#pragma once
#include <QDialog>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include "chat/Message.h"

// ---------------------------------------------------------------------------
// FileTransferDialog
//
// Three modes:
//   1. PreSend  — estimate time, warn user, confirm/cancel
//   2. SendProgress  — progress bar while sending
//   3. ReceiveOffer  — accept/decline incoming file
//   4. ReceiveProgress — progress bar while receiving
// ---------------------------------------------------------------------------
class FileTransferDialog : public QDialog {
    Q_OBJECT
public:
    enum Mode { PreSend, SendProgress, ReceiveOffer, ReceiveProgress };

    FileTransferDialog(Mode mode, QWidget* parent = nullptr);

    void setFileInfo(const QString& name, qint64 size, int totalChunks, const QString& timeEstimate);
    void setProgress(int done, int total);
    void setComplete(const QString& savePath);

signals:
    void accepted();
    void cancelled();

private:
    Mode m_mode;
    QLabel* m_title;
    QLabel* m_details;
    QProgressBar* m_progress;
    QLabel* m_progressLabel;
    QPushButton* m_primaryBtn;
    QPushButton* m_cancelBtn;
};
