#include "MessageWidget.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QProgressBar>
#include <QFont>
#include <QHash>
#include <QFileInfo>
#include <QPixmap>

static const QStringList s_discordColors = {
    "#5865f2", "#57f287", "#fee75c", "#eb459e",
    "#ed4245", "#faa81a", "#9b59b6", "#1abc9c"
};

MessageWidget::MessageWidget(const Message& msg, QWidget* parent)
    : QWidget(parent)
{
    setObjectName("messageWidget");

    auto* outerLayout = new QHBoxLayout(this);
    outerLayout->setContentsMargins(16, 6, 16, 6);
    outerLayout->setSpacing(12);

    // --- Avatar ---
    m_avatar = new QLabel(this);
    m_avatar->setObjectName("avatarLabel");
    m_avatar->setFixedSize(40, 40);
    m_avatar->setAlignment(Qt::AlignCenter);
    m_avatar->setText(msg.callsign.isEmpty() ? "!" : msg.callsign.left(1).toUpper());

    QString bgColor = avatarColor(msg.callsign);
    m_avatar->setStyleSheet(QString(
        "QLabel#avatarLabel {"
        "  background-color: %1;"
        "  color: #ffffff;"
        "  border-radius: 20px;"
        "  font-weight: bold;"
        "  font-size: 16px;"
        "}"
    ).arg(bgColor));

    outerLayout->addWidget(m_avatar, 0, Qt::AlignTop);

    // --- Content column ---
    auto* contentLayout = new QVBoxLayout();
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(2);

    // Row: callsign + timestamp
    auto* headerRow = new QHBoxLayout();
    headerRow->setContentsMargins(0, 0, 0, 0);
    headerRow->setSpacing(8);

    m_callsignLabel = new QLabel(msg.type == Message::System ? "" : msg.callsign, this);
    m_callsignLabel->setObjectName("callsignLabel");

    m_timestampLabel = new QLabel(msg.timestamp.toString("hh:mm"), this);
    m_timestampLabel->setObjectName("timestampLabel");

    headerRow->addWidget(m_callsignLabel);
    headerRow->addWidget(m_timestampLabel);
    headerRow->addStretch();

    contentLayout->addLayout(headerRow);

    // Message body
    if (msg.type == Message::System) {
        m_avatar->setText("#");
        m_avatar->setStyleSheet(QString(
            "QLabel#avatarLabel {"
            "  background-color: #4f545c;"
            "  color: #ffffff;"
            "  border-radius: 20px;"
            "  font-weight: bold;"
            "  font-size: 16px;"
            "}"
        ));

        m_textLabel = new QLabel(msg.text, this);
        m_textLabel->setObjectName("systemMessageText");
        m_textLabel->setWordWrap(true);
        m_textLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
        contentLayout->addWidget(m_textLabel);
    } else if (msg.type == Message::File) {
        m_avatar->setText("\xF0\x9F\x93\x81");

        // File name
        auto* nameLabel = new QLabel(msg.file.fileName, this);
        nameLabel->setStyleSheet("color: #dcddde; font-size: 13px; font-weight: bold; background: transparent;");
        contentLayout->addWidget(nameLabel);

        // Image placeholder — filled on completion
        if (isImageFile(msg.file.fileName)) {
            m_imageLabel = new QLabel(this);
            m_imageLabel->setStyleSheet("background: transparent; border: 1px solid #40444b; border-radius: 4px;");
            m_imageLabel->setMaximumWidth(260);
            m_imageLabel->setAlignment(Qt::AlignCenter);
            m_imageLabel->hide();
            contentLayout->addWidget(m_imageLabel);
        }

        if (msg.file.state == FileTransferInfo::Transferring
            || msg.file.state == FileTransferInfo::Offering) {
            // Progress bar
            m_progressBar = new QProgressBar(this);
            m_progressBar->setObjectName("fileProgressBar");
            m_progressBar->setMaximum(msg.file.totalChunks);
            m_progressBar->setValue(msg.file.receivedChunks);
            m_progressBar->setTextVisible(true);
            m_progressBar->setStyleSheet(
                "QProgressBar#fileProgressBar {"
                "  background-color: #40444b; border: none; border-radius: 4px;"
                "  height: 6px; text-align: center; color: #dcddde; font-size: 10px; }"
                "QProgressBar#fileProgressBar::chunk {"
                "  background-color: #5865f2; border-radius: 4px; }");
            m_progressBar->setMaximumWidth(300);
            contentLayout->addWidget(m_progressBar);

            QString status = msg.file.state == FileTransferInfo::Offering
                ? "Waiting for acceptance..."
                : QString("%1 / %2 chunks").arg(msg.file.receivedChunks).arg(msg.file.totalChunks);
            m_textLabel = new QLabel(status, this);
            m_textLabel->setStyleSheet("color: #72767d; font-size: 11px; background: transparent;");
            contentLayout->addWidget(m_textLabel);
        } else if (msg.file.state == FileTransferInfo::Complete) {
            m_textLabel = new QLabel(QString("\xE2\x9C\x85 Complete \xE2\x80\xA2 %1 KB")
                                         .arg(msg.file.fileSize / 1024), this);
            m_textLabel->setStyleSheet("color: #57f287; font-size: 12px; background: transparent;");
            contentLayout->addWidget(m_textLabel);
        } else if (msg.file.state == FileTransferInfo::Failed) {
            m_textLabel = new QLabel("\xE2\x9D\x8C Transfer failed", this);
            m_textLabel->setStyleSheet("color: #ed4245; font-size: 12px; background: transparent;");
            contentLayout->addWidget(m_textLabel);
        } else {
            m_textLabel = new QLabel("\xE2\x9D\x8C Cancelled", this);
            m_textLabel->setStyleSheet("color: #ed4245; font-size: 12px; background: transparent;");
            contentLayout->addWidget(m_textLabel);
        }
    } else {
        m_textLabel = new QLabel(msg.text, this);
        m_textLabel->setObjectName("messageText");
        m_textLabel->setWordWrap(true);
        m_textLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
        contentLayout->addWidget(m_textLabel);
    }

    outerLayout->addLayout(contentLayout, 1);
}

QString MessageWidget::avatarColor(const QString& callsign)
{
    if (callsign.isEmpty())
        return "#4f545c";
    uint hash = qHash(callsign.toUpper());
    return s_discordColors[hash % s_discordColors.size()];
}

bool MessageWidget::isImageFile(const QString& name)
{
    QString ext = QFileInfo(name).suffix().toLower();
    return ext == "png" || ext == "jpg" || ext == "jpeg"
        || ext == "gif" || ext == "bmp" || ext == "webp";
}

void MessageWidget::updateFileProgress(int received, int total)
{
    if (m_progressBar) {
        m_progressBar->setMaximum(total);
        m_progressBar->setValue(received);
    }
    if (m_textLabel) {
        m_textLabel->setText(QString("%1 / %2 chunks").arg(received).arg(total));
    }
}

void MessageWidget::updateFileComplete(qint64 fileSize)
{
    if (m_progressBar) {
        m_progressBar->setValue(m_progressBar->maximum());
    }
    if (m_textLabel) {
        if (fileSize > 0)
            m_textLabel->setText(QString("\xE2\x9C\x85 Complete \xE2\x80\xA2 %1 KB")
                                     .arg(fileSize / 1024));
        else
            m_textLabel->setText("\xE2\x9C\x85 Complete");
        m_textLabel->setStyleSheet("color: #57f287; font-size: 12px; background: transparent;");
    }
}

void MessageWidget::updateFileFailed()
{
    if (m_textLabel) {
        m_textLabel->setText("\xE2\x9D\x8C Transfer failed");
        m_textLabel->setStyleSheet("color: #ed4245; font-size: 12px; background: transparent;");
    }
}

void MessageWidget::showImage(const QString& path)
{
    if (!m_imageLabel || path.isEmpty())
        return;

    QPixmap pix(path);
    if (pix.isNull())
        return;

    QPixmap scaled = pix.scaledToWidth(260, Qt::SmoothTransformation);
    m_imageLabel->setPixmap(scaled);
    m_imageLabel->setFixedSize(scaled.size());
    m_imageLabel->show();
    m_imageLabel->setCursor(Qt::PointingHandCursor);
    m_imageLabel->setToolTip("Click to open");
}
