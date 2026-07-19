#include "ActiveUsersPanel.h"
#include <QVBoxLayout>
#include <QFont>
#include <QHash>

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
    layout->addWidget(m_userList, 1);
}

void ActiveUsersPanel::setLocalUser(const QString& callsign)
{
    m_localUser = callsign;
}

void ActiveUsersPanel::addUser(const QString& callsign)
{
    if (callsign.isEmpty())
        return;

    if (m_items.contains(callsign)) {
        // Already present — could update "last heard" timestamp in the future
        return;
    }

    bool isLocal = (callsign == m_localUser);
    QListWidgetItem* item = createUserItem(callsign, isLocal);

    // Insert local user at top, others alphabetically after local
    if (isLocal) {
        m_userList->insertItem(0, item);
    } else {
        // Insert in alphabetical order after any local user
        int insertAt = 0;
        if (m_items.contains(m_localUser))
            insertAt = 1;

        // Find correct alphabetical position
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

    m_items.insert(callsign, item);
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

QListWidgetItem* ActiveUsersPanel::createUserItem(const QString& callsign, bool isLocal)
{
    QString color = avatarColor(callsign);

    QString label;
    if (isLocal) {
        label = QString("  %1 (you)").arg(callsign);
    } else {
        label = QString("  %1").arg(callsign);
    }

    auto* item = new QListWidgetItem(label);
    item->setData(Qt::UserRole, callsign);
    item->setForeground(QColor(color));

    QFont font;
    font.setPointSize(11);
    item->setFont(font);

    item->setToolTip(QString("Heard on frequency: %1").arg(callsign));

    return item;
}

QString ActiveUsersPanel::avatarColor(const QString& callsign)
{
    if (callsign.isEmpty())
        return "#4f545c";
    uint hash = qHash(callsign.toUpper());
    return s_userColors[hash % s_userColors.size()];
}
