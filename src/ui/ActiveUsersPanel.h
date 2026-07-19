#pragma once
#include <QWidget>
#include <QListWidget>
#include <QLabel>
#include <QMap>
#include <QDateTime>
#include <QString>

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
    QMap<QString, QListWidgetItem*> m_items;

    void updateHeader();
    QListWidgetItem* createUserItem(const QString& callsign, bool isLocal);
    static QString avatarColor(const QString& callsign);
};
