#pragma once
#include <QMainWindow>
#include <QSettings>
#include <QMap>
#include <QList>
#include "chat/ChatController.h"
#include "ui/ChannelList.h"
#include "ui/ChatView.h"
#include "ui/InputBar.h"
#include "ui/ActiveUsersPanel.h"

class QLabel;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

private slots:
    void onMessageReceived(const Message& msg);
    void onSendMessage(const QString& text);
    void onConnected();
    void onDisconnected();
    void onError(const QString& error);
    void onChannelSelected(const QString& channel);
    void openSettings();

private:
    ChatController* m_controller;
    ChannelList* m_channelList;
    ChatView* m_chatView;
    InputBar* m_inputBar;
    ActiveUsersPanel* m_activeUsers;
    QLabel* m_channelHeader;
    QSettings m_settings;

    QMap<QString, QList<Message>> m_channelMessages;

    void setupUi();
    void setupMenuBar();
    void setupStatusBar();
    void loadSettings();
    void createTnc();
    void switchToChannel(const QString& channel);
    void ensureChannelExists(const QString& channel);
};
