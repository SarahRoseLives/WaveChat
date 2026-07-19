#pragma once
#include <QMainWindow>
#include <QSettings>
#include <QMap>
#include <QList>
#include "chat/ChatController.h"
#include "chat/FileTransfer.h"
#include "ui/ChannelList.h"
#include "ui/ChatView.h"
#include "ui/InputBar.h"
#include "ui/MessageWidget.h"
#include "ui/ActiveUsersPanel.h"

class QLabel;
class FileTransferDialog;

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

    // File transfer
    void onFileAttachRequested(const QString& filePath);
    void onFileOfferReceived(const FileTransferInfo& info);
    void onFileProgressUpdate(const QString& fileId, int done, int total);
    void onFileSendProgress(int done, int total);
    void onFileComplete(const FileTransferInfo& info);
    void onFileFailed(const QString& fileId, const QString& reason);

private:
    ChatController* m_controller;
    ChannelList* m_channelList;
    ChatView* m_chatView;
    InputBar* m_inputBar;
    ActiveUsersPanel* m_activeUsers;
    QLabel* m_channelHeader;
    QSettings m_settings;
    FileTransferManager* m_ftManager;

    QMap<QString, QList<Message>> m_channelMessages;

    // Active file transfer dialogs
    FileTransferDialog* m_sendDialog = nullptr;
    FileTransferDialog* m_recvDialog = nullptr;

    // File message widgets we need to update (fileId → MessageWidget*)
    QMap<QString, class MessageWidget*> m_fileWidgets;

    void setupUi();
    void setupMenuBar();
    void setupStatusBar();
    void loadSettings();
    void createTnc();
    void switchToChannel(const QString& channel);
    void ensureChannelExists(const QString& channel);
    void showFileMessage(const Message& msg);
};
