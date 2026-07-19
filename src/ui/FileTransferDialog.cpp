#include "FileTransferDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QFont>
#include <QFileInfo>

FileTransferDialog::FileTransferDialog(Mode mode, QWidget* parent)
    : QDialog(parent)
    , m_mode(mode)
{
    setMinimumWidth(380);
    setMaximumWidth(420);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(20, 20, 20, 16);
    layout->setSpacing(12);

    // Title
    m_title = new QLabel(this);
    QFont titleFont;
    titleFont.setPointSize(13);
    titleFont.setBold(true);
    m_title->setFont(titleFont);
    m_title->setStyleSheet("color: #ffffff; background: transparent;");
    layout->addWidget(m_title);

    // Details
    m_details = new QLabel(this);
    m_details->setWordWrap(true);
    m_details->setStyleSheet("color: #b9bbbe; background: transparent; font-size: 12px;");
    layout->addWidget(m_details);

    // Progress bar (hidden for PreSend / ReceiveOffer)
    m_progress = new QProgressBar(this);
    m_progress->setObjectName("fileProgressBar");
    m_progress->setTextVisible(true);
    m_progress->setStyleSheet(
        "QProgressBar#fileProgressBar {"
        "  background-color: #40444b; border: none; border-radius: 4px;"
        "  height: 8px; text-align: center; color: #dcddde; font-size: 11px; }"
        "QProgressBar#fileProgressBar::chunk {"
        "  background-color: #5865f2; border-radius: 4px; }");
    m_progress->setVisible(false);
    layout->addWidget(m_progress);

    m_progressLabel = new QLabel(this);
    m_progressLabel->setStyleSheet("color: #8e9297; background: transparent; font-size: 11px;");
    m_progressLabel->setVisible(false);
    layout->addWidget(m_progressLabel);

    // Buttons
    auto* btnRow = new QHBoxLayout();
    btnRow->addStretch();

    m_cancelBtn = new QPushButton(this);
    m_cancelBtn->setMinimumWidth(80);
    connect(m_cancelBtn, &QPushButton::clicked, this, [this]() {
        emit cancelled();
        close();
    });
    btnRow->addWidget(m_cancelBtn);

    m_primaryBtn = new QPushButton(this);
    m_primaryBtn->setObjectName("primaryButton");
    m_primaryBtn->setMinimumWidth(100);
    connect(m_primaryBtn, &QPushButton::clicked, this, [this]() {
        emit accepted();
        close();
    });
    btnRow->addWidget(m_primaryBtn);

    layout->addLayout(btnRow);

    // Configure for mode
    switch (mode) {
    case PreSend:
        setWindowTitle("Send File");
        m_title->setText("\xF0\x9F\x93\x81 Send File");
        m_primaryBtn->setText("Send");
        m_cancelBtn->setText("Cancel");
        break;
    case SendProgress:
        setWindowTitle("Sending File");
        m_title->setText("\xF0\x9F\x93\xA4 Sending...");
        m_primaryBtn->setText("Cancel Send");
        m_cancelBtn->hide();
        m_progress->setVisible(true);
        m_progressLabel->setVisible(true);
        break;
    case ReceiveOffer:
        setWindowTitle("Incoming File");
        m_title->setText("\xF0\x9F\x93\xA5 Incoming File");
        m_primaryBtn->setText("Accept");
        m_cancelBtn->setText("Decline");
        break;
    case ReceiveProgress:
        setWindowTitle("Receiving File");
        m_title->setText("\xF0\x9F\x93\xA5 Receiving...");
        m_primaryBtn->hide();
        m_cancelBtn->setText("Cancel");
        m_progress->setVisible(true);
        m_progressLabel->setVisible(true);
        break;
    }
}

void FileTransferDialog::setFileInfo(const QString& name, qint64 size, int totalChunks, const QString& timeEstimate)
{
    QFileInfo fi(name);
    QString sizeStr;
    if (size < 1024)
        sizeStr = QString("%1 B").arg(size);
    else if (size < 1024 * 1024)
        sizeStr = QString("%1 KB").arg(size / 1024);
    else
        sizeStr = QString("%1.%2 MB").arg(size / (1024 * 1024)).arg((size % (1024 * 1024)) / 102400);

    QString detail;
    if (m_mode == PreSend || m_mode == SendProgress) {
        detail = QString("%1  \xE2\x80\xA2  %2  \xE2\x80\xA2  %3 chunks\n\n"
                         "\xE2\x8F\xB1  Estimated time: %4 at 1200 baud\n\n"
                         "\xE2\x9A\xA0  Verify you're on a clear frequency.\n"
                         "Large transfers can clog shared channels.")
                     .arg(fi.fileName(), sizeStr, QString::number(totalChunks), timeEstimate);
    } else {
        detail = QString("%1  \xE2\x80\xA2  %2  \xE2\x80\xA2  %3 chunks")
                     .arg(fi.fileName(), sizeStr, QString::number(totalChunks));
    }
    m_details->setText(detail);

    if (m_progress->isVisible())
        m_progress->setMaximum(totalChunks);
}

void FileTransferDialog::setProgress(int done, int total)
{
    if (done == 0 && total > 0 && total < 100) {
        // Gap-fill mode — retransmitting missing chunks
        m_progress->setMaximum(total);
        m_progress->setValue(0);
        m_title->setText("\xF0\x9F\x94\x84 Retransmitting...");
        m_progressLabel->setText(QString("Resending %1 missing chunks").arg(total));
    } else {
        m_progress->setMaximum(total);
        m_progress->setValue(done);
        m_progressLabel->setText(QString("%1 / %2 chunks").arg(done).arg(total));
    }
}

void FileTransferDialog::setComplete(const QString& savePath)
{
    m_title->setText("\xE2\x9C\x85 Transfer Complete");
    m_progress->setValue(m_progress->maximum());
    m_progressLabel->setText(QString("Saved to: %1").arg(savePath));
    m_primaryBtn->setText("Open Folder");
    m_primaryBtn->show();
    m_cancelBtn->setText("Close");
    m_cancelBtn->show();
}
