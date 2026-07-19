#include "MessageWidget.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFont>
#include <QHash>

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

    // Message text
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
    } else {
        m_textLabel = new QLabel(msg.text, this);
        m_textLabel->setObjectName("messageText");
        m_textLabel->setWordWrap(true);
        m_textLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    }

    contentLayout->addWidget(m_textLabel);
    outerLayout->addLayout(contentLayout, 1);
}

QString MessageWidget::avatarColor(const QString& callsign)
{
    if (callsign.isEmpty())
        return "#4f545c";
    uint hash = qHash(callsign.toUpper());
    return s_discordColors[hash % s_discordColors.size()];
}
