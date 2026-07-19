#pragma once
#include <QWidget>
#include <QListWidget>

class ChannelList : public QListWidget {
    Q_OBJECT
public:
    explicit ChannelList(QWidget* parent = nullptr);

    void setChannelName(const QString& name);
    void setConnectionStatus(bool connected);

signals:
    void channelClicked(const QString& name);

private:
    QString m_channelName;
};
