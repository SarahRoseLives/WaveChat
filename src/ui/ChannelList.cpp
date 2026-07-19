#include "ChannelList.h"
#include <QFont>

ChannelList::ChannelList(QWidget* parent)
    : QListWidget(parent)
{
    setObjectName("channelList");
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setMinimumWidth(180);
    setMaximumWidth(260);

    QFont font;
    font.setPointSize(11);
    setFont(font);

    connect(this, &QListWidget::itemClicked, this, [this](QListWidgetItem* item) {
        emit channelClicked(item->text());
    });
}

void ChannelList::setChannelName(const QString& name)
{
    m_channelName = name;
    clear();

    auto* item = new QListWidgetItem(QString("# %1").arg(name), this);
    QFont boldFont;
    boldFont.setPointSize(11);
    boldFont.setBold(true);
    item->setFont(boldFont);
    item->setSelected(true);

    if (!m_frequency.isEmpty()) {
        auto* freqItem = new QListWidgetItem(QString("  %1").arg(m_frequency), this);
        QFont smallFont;
        smallFont.setPointSize(9);
        freqItem->setFont(smallFont);
        freqItem->setFlags(freqItem->flags() & ~Qt::ItemIsSelectable);
    }
}

void ChannelList::setChannelFrequency(const QString& freq)
{
    m_frequency = freq;
    if (!m_channelName.isEmpty())
        setChannelName(m_channelName);
}

void ChannelList::setConnectionStatus(bool connected)
{
    // Visual indicator — add or remove a status item
    for (int i = count() - 1; i >= 0; --i) {
        if (item(i)->text().startsWith("STATUS:")) {
            delete takeItem(i);
        }
    }

    auto* statusItem = new QListWidgetItem(
        connected ? "STATUS: Connected" : "STATUS: Disconnected", this);
    QFont smallFont;
    smallFont.setPointSize(8);
    statusItem->setFont(smallFont);
    statusItem->setFlags(statusItem->flags() & ~Qt::ItemIsSelectable);

    if (connected)
        statusItem->setForeground(QColor("#57f287"));
    else
        statusItem->setForeground(QColor("#ed4245"));
}
