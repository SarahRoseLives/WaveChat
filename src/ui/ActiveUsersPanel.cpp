#include "ActiveUsersPanel.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFont>
#include <QHash>
#include <QTimer>

static const QStringList s_userColors = {
    "#5865f2", "#57f287", "#fee75c", "#eb459e",
    "#ed4245", "#faa81a", "#9b59b6", "#1abc9c"
};

ActiveUsersPanel::ActiveUsersPanel(QWidget* parent)
    : QWidget(parent)
{
    setObjectName("activeUsersPanel");
    setMinimumWidth(180);
    setMaximumWidth(240);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_header = new QLabel("ON FREQUENCY — 0", this);
    m_header->setObjectName("activeUsersHeader");
    QFont headerFont;
    headerFont.setPointSize(10);
    headerFont.setBold(true);
    m_header->setFont(headerFont);
    layout->addWidget(m_header);

    m_userList = new QListWidget(this);
    m_userList->setObjectName("activeUsersList");
    m_userList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_userList->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_userList->setFrameShape(QFrame::NoFrame);
    m_userList->setSpacing(1);
    layout->addWidget(m_userList, 1);

    // Refresh "X ago" labels every 30 seconds
    auto* timer = new QTimer(this);
    timer->setInterval(30000);
    connect(timer, &QTimer::timeout, this, &ActiveUsersPanel::updateTimestamps);
    timer->start();
}

void ActiveUsersPanel::setLocalUser(const QString& callsign)
{
    m_localUser = callsign;
}

void ActiveUsersPanel::addUser(const QString& callsign)
{
    if (callsign.isEmpty())
        return;

    // Already exists — just refresh the timestamp
    auto it = m_items.find(callsign);
    if (it != m_items.end()) {
        QDateTime now = QDateTime::currentDateTime();
        it->timeLabel->setText(formatTime(now));
        it->timeLabel->setProperty("lastHeard", now);
        return;
    }

    bool isLocal = (callsign == m_localUser);
    QString color = avatarColor(callsign);

    // --- Build item widget ---
    auto* widget = new QWidget();
    widget->setStyleSheet("background: transparent;");
    auto* wLayout = new QVBoxLayout(widget);
    wLayout->setContentsMargins(8, 2, 8, 2);
    wLayout->setSpacing(0);

    // Row: colored dot + callsign
    auto* topRow = new QHBoxLayout();
    topRow->setContentsMargins(0, 0, 0, 0);
    topRow->setSpacing(6);

    auto* dot = new QLabel(widget);
    dot->setFixedSize(8, 8);
    dot->setStyleSheet(QString(
        "background-color: %1; border-radius: 4px;"
    ).arg(color));

    QString label = isLocal ? QString("%1 (you)").arg(callsign) : callsign;
    auto* nameLabel = new QLabel(label, widget);
    nameLabel->setStyleSheet(QString(
        "color: %1; font-size: 12px; font-weight: bold; background: transparent;"
    ).arg(color));

    topRow->addWidget(dot);
    topRow->addWidget(nameLabel, 1);
    wLayout->addLayout(topRow);

    // Timestamp
    auto* timeLabel = new QLabel(formatTime(QDateTime::currentDateTime()), widget);
    timeLabel->setObjectName("userTimestamp");
    timeLabel->setStyleSheet("color: #72767d; font-size: 10px; background: transparent; padding-left: 14px;");
    timeLabel->setProperty("lastHeard", QDateTime::currentDateTime());
    wLayout->addWidget(timeLabel);

    // Calculate size
    QFontMetrics nameFm(nameLabel->font());
    QFontMetrics timeFm(timeLabel->font());
    int h = nameFm.lineSpacing() + timeFm.lineSpacing() + 4;
    widget->setMinimumHeight(h);

    auto* item = new QListWidgetItem();
    item->setSizeHint(QSize(0, h));
    item->setData(Qt::UserRole, callsign);

    // Insert in order: local first, then alphabetical
    if (isLocal) {
        m_userList->insertItem(0, item);
    } else {
        int insertAt = 0;
        if (m_items.contains(m_localUser))
            insertAt = 1;

        for (int i = insertAt; i < m_userList->count(); ++i) {
            QListWidgetItem* existing = m_userList->item(i);
            if (existing && existing->data(Qt::UserRole).toString() > callsign) {
                insertAt = i;
                break;
            }
            insertAt = i + 1;
        }
        m_userList->insertItem(insertAt, item);
    }

    m_userList->setItemWidget(item, widget);

    UserEntry entry;
    entry.item = item;
    entry.widget = widget;
    entry.timeLabel = timeLabel;
    m_items.insert(callsign, entry);

    updateHeader();
}

void ActiveUsersPanel::clear()
{
    m_userList->clear();
    m_items.clear();
    updateHeader();
}

int ActiveUsersPanel::userCount() const
{
    return m_items.size();
}

void ActiveUsersPanel::updateHeader()
{
    m_header->setText(QString("ON FREQUENCY — %1").arg(m_items.size()));
}

void ActiveUsersPanel::updateTimestamps()
{
    QDateTime now = QDateTime::currentDateTime();
    for (auto it = m_items.begin(); it != m_items.end(); ++it) {
        QDateTime lastHeard = it->timeLabel->property("lastHeard").toDateTime();
        if (lastHeard.isValid()) {
            it->timeLabel->setText(formatTime(lastHeard));
        }
    }
}

QString ActiveUsersPanel::formatTime(const QDateTime& dt)
{
    if (!dt.isValid())
        return QString();

    qint64 secs = dt.secsTo(QDateTime::currentDateTime());
    if (secs < 0)  secs = 0;
    if (secs < 60) return "just now";
    if (secs < 120) return "1m ago";
    if (secs < 3600) return QString("%1m ago").arg(secs / 60);
    if (secs < 7200) return "1h ago";
    if (secs < 86400) return QString("%1h ago").arg(secs / 3600);
    return dt.toString("MMM d");
}

QString ActiveUsersPanel::avatarColor(const QString& callsign)
{
    if (callsign.isEmpty())
        return "#4f545c";
    uint hash = qHash(callsign.toUpper());
    return s_userColors[hash % s_userColors.size()];
}
