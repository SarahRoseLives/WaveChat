#include "ChannelList.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QFont>
#include <QKeyEvent>
#include <QRegularExpression>
#include <QTimer>

ChannelList::ChannelList(QWidget* parent)
    : QWidget(parent)
{
    setObjectName("channelListPanel");
    setMinimumWidth(180);
    setMaximumWidth(260);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    auto* header = new QLabel("CHANNELS", this);
    header->setObjectName("channelListHeader");
    QFont headerFont;
    headerFont.setPointSize(9);
    headerFont.setBold(true);
    header->setFont(headerFont);
    layout->addWidget(header);

    m_list = new QListWidget(this);
    m_list->setObjectName("channelList");
    m_list->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_list->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_list->setFrameShape(QFrame::NoFrame);
    m_list->setSpacing(1);

    QFont listFont;
    listFont.setPointSize(11);
    m_list->setFont(listFont);

    connect(m_list, &QListWidget::itemClicked, this, &ChannelList::onChannelClicked);
    layout->addWidget(m_list, 1);

    auto* newRow = new QWidget(this);
    newRow->setObjectName("newChannelRow");
    auto* newLayout = new QHBoxLayout(newRow);
    newLayout->setContentsMargins(8, 6, 8, 8);
    newLayout->setSpacing(4);

    m_newChannelEdit = new QLineEdit(newRow);
    m_newChannelEdit->setPlaceholderText("New channel...");
    m_newChannelEdit->setMaxLength(24);
    m_newChannelEdit->installEventFilter(this);
    connect(m_newChannelEdit, &QLineEdit::returnPressed, this, &ChannelList::onCreateChannel);

    m_createButton = new QPushButton("+", newRow);
    m_createButton->setFixedSize(28, 28);
    m_createButton->setToolTip("Create channel");
    connect(m_createButton, &QPushButton::clicked, this, &ChannelList::onCreateChannel);

    newLayout->addWidget(m_newChannelEdit, 1);
    newLayout->addWidget(m_createButton);
    layout->addWidget(newRow);

    auto* timer = new QTimer(this);
    timer->setInterval(30000);
    connect(timer, &QTimer::timeout, this, &ChannelList::updateTimestamps);
    timer->start();
}

// ---- item creation ----

void ChannelList::createChannelItem(const QString& name)
{
    auto* widget = new QWidget();
    widget->setStyleSheet("background: transparent;");
    auto* wLayout = new QVBoxLayout(widget);
    wLayout->setContentsMargins(8, 2, 8, 2);
    wLayout->setSpacing(0);

    auto* nameLabel = new QLabel(QString("# %1").arg(name), widget);
    nameLabel->setObjectName("channelNameLabel");
    QFont nameFont;
    nameFont.setPointSize(11);
    nameLabel->setFont(nameFont);
    wLayout->addWidget(nameLabel);

    auto* timeLabel = new QLabel("just now", widget);
    timeLabel->setObjectName("channelTimeLabel");
    timeLabel->setStyleSheet("color: #72767d; font-size: 10px; background: transparent;");
    wLayout->addWidget(timeLabel);

    // Calculate size from actual font metrics
    QFontMetrics nameFm(nameLabel->font());
    QFontMetrics timeFm(timeLabel->font());
    int h = nameFm.lineSpacing() + timeFm.lineSpacing() + 4; // margins
    widget->setMinimumHeight(h);

    auto* item = new QListWidgetItem();
    item->setData(Qt::UserRole, name);
    item->setSizeHint(QSize(0, h));

    m_list->addItem(item);
    m_list->setItemWidget(item, widget);

    ChannelEntry entry;
    entry.item = item;
    entry.widget = widget;
    entry.nameLabel = nameLabel;
    entry.timeLabel = timeLabel;
    m_entries.insert(name, entry);

    // Apply unread if needed
    if (m_unreadChannels.contains(name))
        setChannelUnread(name, true);

    if (name == m_activeChannel) {
        item->setSelected(true);
        m_list->setCurrentItem(item);
    }
}

// ---- helpers ----

QListWidgetItem* ChannelList::findChannelItem(const QString& name) const
{
    for (int i = 0; i < m_list->count(); ++i) {
        QListWidgetItem* item = m_list->item(i);
        if (item->data(Qt::UserRole).toString() == name)
            return item;
    }
    return nullptr;
}

void ChannelList::updateTimestamps()
{
    for (auto it = m_entries.begin(); it != m_entries.end(); ++it) {
        QVariant v = it->timeLabel->property("lastHeard");
        if (v.isValid()) {
            it->timeLabel->setText(formatTime(v.toDateTime()));
        }
    }
}

QString ChannelList::formatTime(const QDateTime& dt)
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

// ---- public API ----

void ChannelList::addChannel(const QString& name)
{
    if (m_entries.contains(name))
        return;

    createChannelItem(name);
}

void ChannelList::setActiveChannel(const QString& name)
{
    m_activeChannel = name;
    for (int i = 0; i < m_list->count(); ++i) {
        QListWidgetItem* item = m_list->item(i);
        item->setSelected(item->data(Qt::UserRole).toString() == name);
    }
}

void ChannelList::setChannelUnread(const QString& name, bool unread)
{
    if (unread)
        m_unreadChannels.insert(name);
    else
        m_unreadChannels.remove(name);

    auto it = m_entries.find(name);
    if (it != m_entries.end() && it->nameLabel) {
        QFont f = it->nameLabel->font();
        f.setBold(unread);
        it->nameLabel->setFont(f);
        it->nameLabel->setStyleSheet(
            QString("color: %1; font-size: %2px; background: transparent;")
                .arg(unread ? "#ffffff" : "#8e9297")
                .arg(f.pointSize()));
    }
}

void ChannelList::touchChannel(const QString& channel)
{
    auto it = m_entries.find(channel);
    if (it != m_entries.end() && it->timeLabel) {
        QDateTime now = QDateTime::currentDateTime();
        it->timeLabel->setText(formatTime(now));
        it->timeLabel->setProperty("lastHeard", now);
    }
}

bool ChannelList::hasUnread(const QString& name) const
{
    return m_unreadChannels.contains(name);
}

void ChannelList::setConnectionStatus(bool connected)
{
    if (m_statusItem) {
        delete m_statusItem;
        m_statusItem = nullptr;
    }

    m_statusItem = new QListWidgetItem(
        connected ? "  Connected" : "  Disconnected", m_list);
    QFont smallFont;
    smallFont.setPointSize(8);
    m_statusItem->setFont(smallFont);
    m_statusItem->setFlags(m_statusItem->flags() & ~Qt::ItemIsSelectable);

    if (connected)
        m_statusItem->setForeground(QColor("#57f287"));
    else
        m_statusItem->setForeground(QColor("#ed4245"));
}

QStringList ChannelList::channels() const
{
    QStringList result;
    for (auto it = m_entries.begin(); it != m_entries.end(); ++it)
        result.append(it.key());
    return result;
}

QString ChannelList::activeChannel() const { return m_activeChannel; }

// ---- slots ----

void ChannelList::onCreateChannel()
{
    QString name = m_newChannelEdit->text().trimmed();
    if (name.isEmpty()) return;

    name.replace(' ', '-');
    name.remove(QRegularExpression("[^a-zA-Z0-9_\\-]"));
    if (name.isEmpty()) return;

    m_newChannelEdit->clear();

    if (m_entries.contains(name)) return;

    addChannel(name);
    setActiveChannel(name);
    touchChannel(name);
    emit channelSelected(name);
}

void ChannelList::onChannelClicked(QListWidgetItem* item)
{
    QString name = item->data(Qt::UserRole).toString();
    if (name.isEmpty() || name == m_activeChannel)
        return;

    m_activeChannel = name;
    setActiveChannel(name);
    emit channelSelected(name);
}
