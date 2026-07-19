#pragma once
#include <QWidget>
#include <QListWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QStringList>
#include <QSet>
#include <QMap>
#include <QDateTime>

struct ChannelEntry {
    QListWidgetItem* item = nullptr;
    QWidget* widget = nullptr;
    QLabel* timeLabel = nullptr;
    QLabel* nameLabel = nullptr;
};

class ChannelList : public QWidget {
    Q_OBJECT
public:
    explicit ChannelList(QWidget* parent = nullptr);

    void addChannel(const QString& name);
    void setActiveChannel(const QString& name);
    void setChannelUnread(const QString& name, bool unread);
    void touchChannel(const QString& name);
    bool hasUnread(const QString& name) const;
    void setConnectionStatus(bool connected);
    QStringList channels() const;
    QString activeChannel() const;

signals:
    void channelSelected(const QString& name);

private slots:
    void onCreateChannel();
    void onChannelClicked(QListWidgetItem* item);

private:
    QListWidget* m_list;
    QLineEdit* m_newChannelEdit;
    QPushButton* m_createButton;
    QString m_activeChannel;
    QSet<QString> m_unreadChannels;
    QMap<QString, ChannelEntry> m_entries;
    QListWidgetItem* m_statusItem = nullptr;

    QListWidgetItem* findChannelItem(const QString& name) const;
    void createChannelItem(const QString& name);
    void updateTimestamps();
    static QString formatTime(const QDateTime& dt);
};
