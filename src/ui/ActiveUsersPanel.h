#pragma once
#include <QWidget>
#include <QListWidget>
#include <QLabel>
#include <QMap>
#include <QDateTime>
#include <QString>

struct UserEntry {
    QListWidgetItem* item = nullptr;
    QWidget* widget = nullptr;
    QLabel* timeLabel = nullptr;
};

class ActiveUsersPanel : public QWidget {
    Q_OBJECT
public:
    explicit ActiveUsersPanel(QWidget* parent = nullptr);

    void addUser(const QString& callsign);
    void setLocalUser(const QString& callsign);
    void clear();
    int userCount() const;

private:
    QLabel* m_header;
    QListWidget* m_userList;
    QString m_localUser;
    QMap<QString, UserEntry> m_items;

    void updateHeader();
    void updateTimestamps();
    static QString formatTime(const QDateTime& dt);
    static QString avatarColor(const QString& callsign);
};
