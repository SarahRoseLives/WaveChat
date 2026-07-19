#include "MainWindow.h"
#include "SettingsDialog.h"
#include "FileTransferDialog.h"
#include "tnc/KissTnc.h"
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QStatusBar>
#include <QSplitter>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QApplication>
#include <QTimer>
#include <QCloseEvent>
#include <QFileInfo>
#include <QDesktopServices>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_controller(new ChatController(this))
    , m_settings("WaveChat", "WaveChat")
    , m_ftManager(new FileTransferManager(this))
{
    setupUi();
    setupMenuBar();
    setupStatusBar();

    connect(m_controller, &ChatController::messageReceived,
            this, &MainWindow::onMessageReceived);
    connect(m_controller, &ChatController::rawFrameReceived,
            this, [this](const QByteArray& info, const QString& from) {
        m_ftManager->processIncomingFrame(QString::fromUtf8(info), from);
        if (!from.isEmpty())
            m_activeUsers->addUser(from);
    });
    connect(m_controller, &ChatController::connected,
            this, &MainWindow::onConnected);
    connect(m_controller, &ChatController::disconnected,
            this, &MainWindow::onDisconnected);
    connect(m_controller, &ChatController::errorOccurred,
            this, &MainWindow::onError);

    // File transfer signals
    connect(m_ftManager, &FileTransferManager::sendRawFrame,
            this, [this](const QByteArray& data) {
        m_controller->sendRawMessage(QString::fromUtf8(data));
    });
    connect(m_ftManager, &FileTransferManager::fileOfferReceived,
            this, &MainWindow::onFileOfferReceived);
    connect(m_ftManager, &FileTransferManager::fileProgressUpdate,
            this, &MainWindow::onFileProgressUpdate);
    connect(m_ftManager, &FileTransferManager::fileSendProgress,
            this, &MainWindow::onFileSendProgress);
    connect(m_ftManager, &FileTransferManager::fileComplete,
            this, &MainWindow::onFileComplete);
    connect(m_ftManager, &FileTransferManager::fileSendComplete,
            this, [this](const QString& fileId) {
        if (m_sendDialog)
            m_sendDialog->setComplete("");

        auto it = m_fileWidgets.find(fileId);
        if (it != m_fileWidgets.end() && it.value())
            it.value()->updateFileComplete(0);
    });
    connect(m_ftManager, &FileTransferManager::fileFailed,
            this, &MainWindow::onFileFailed);

    loadSettings();
}

MainWindow::~MainWindow()
{
    m_controller->disconnectTnc();
}

void MainWindow::setupUi()
{
    setWindowTitle("WaveChat");
    resize(960, 640);
    setMinimumSize(720, 400);

    auto* centralWidget = new QWidget(this);
    centralWidget->setObjectName("centralWidget");
    setCentralWidget(centralWidget);

    auto* mainLayout = new QHBoxLayout(centralWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    auto* splitter = new QSplitter(Qt::Horizontal, centralWidget);
    splitter->setHandleWidth(1);

    m_channelList = new ChannelList(splitter);
    splitter->addWidget(m_channelList);

    auto* rightPanel = new QWidget(splitter);
    auto* rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(0);

    m_channelHeader = new QLabel("# channel", rightPanel);
    m_channelHeader->setObjectName("channelHeader");
    rightLayout->addWidget(m_channelHeader);

    m_chatView = new ChatView(rightPanel);
    rightLayout->addWidget(m_chatView, 1);

    m_inputBar = new InputBar(rightPanel);
    rightLayout->addWidget(m_inputBar);

    splitter->addWidget(rightPanel);

    m_activeUsers = new ActiveUsersPanel(splitter);
    splitter->addWidget(m_activeUsers);

    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    splitter->setStretchFactor(2, 0);
    splitter->setSizes({200, 760, 180});

    mainLayout->addWidget(splitter);

    connect(m_inputBar, &InputBar::messageSubmitted,
            this, &MainWindow::onSendMessage);
    connect(m_inputBar, &InputBar::fileAttachRequested,
            this, &MainWindow::onFileAttachRequested);
    connect(m_channelList, &ChannelList::channelSelected,
            this, &MainWindow::onChannelSelected);
}

void MainWindow::setupMenuBar()
{
    auto* fileMenu = menuBar()->addMenu("&File");

    auto* settingsAction = fileMenu->addAction("&Settings...");
    settingsAction->setShortcut(QKeySequence("Ctrl+,"));
    connect(settingsAction, &QAction::triggered, this, &MainWindow::openSettings);

    fileMenu->addSeparator();

    auto* quitAction = fileMenu->addAction("&Quit");
    quitAction->setShortcut(QKeySequence("Ctrl+Q"));
    connect(quitAction, &QAction::triggered, qApp, &QApplication::quit);

    auto* viewMenu = menuBar()->addMenu("&View");

    auto* connectAction = viewMenu->addAction("&Connect TNC");
    connectAction->setShortcut(QKeySequence("Ctrl+Shift+C"));
    connect(connectAction, &QAction::triggered, this, [this]() {
        m_controller->connectTnc();
    });

    auto* disconnectAction = viewMenu->addAction("&Disconnect TNC");
    disconnectAction->setShortcut(QKeySequence("Ctrl+Shift+D"));
    connect(disconnectAction, &QAction::triggered, this, [this]() {
        m_controller->disconnectTnc();
    });
}

void MainWindow::setupStatusBar()
{
    statusBar()->showMessage("Disconnected — configure via File > Settings (Ctrl+,)");
}

// ============================================================================
// Channel routing
// ============================================================================

void MainWindow::onMessageReceived(const Message& msg)
{
    QString channel = msg.channel.isEmpty() ? "main" : msg.channel;

    ensureChannelExists(channel);
    m_channelMessages[channel].append(msg);

    if (msg.type == Message::Text || msg.type == Message::File) {
        m_channelList->touchChannel(channel);
    }

    if (channel == m_controller->currentChannel()) {
        m_chatView->appendMessage(msg);
    } else {
        if (msg.type == Message::Text) {
            m_channelList->setChannelUnread(channel, true);
        }
    }

    if ((msg.type == Message::Text || msg.type == Message::File) && !msg.callsign.isEmpty()) {
        m_activeUsers->addUser(msg.callsign);
    }
}

void MainWindow::onSendMessage(const QString& text)
{
    m_controller->sendMessage(text);
}

void MainWindow::onChannelSelected(const QString& channel)
{
    if (channel == m_controller->currentChannel())
        return;
    switchToChannel(channel);
}

void MainWindow::switchToChannel(const QString& channel)
{
    QString oldChannel = m_controller->currentChannel();
    if (!oldChannel.isEmpty())
        m_channelList->setChannelUnread(oldChannel, false);

    m_controller->setCurrentChannel(channel);
    m_chatView->clearMessages();
    m_channelHeader->setText(QString("# %1").arg(channel));
    m_inputBar->setPlaceholder(QString("Message #%1").arg(channel));
    m_channelList->setActiveChannel(channel);
    m_channelList->setChannelUnread(channel, false);

    if (m_channelMessages.contains(channel)) {
        for (const auto& msg : m_channelMessages[channel])
            m_chatView->appendMessage(msg);
    }
}

void MainWindow::ensureChannelExists(const QString& channel)
{
    if (!m_channelMessages.contains(channel)) {
        m_channelMessages[channel] = {};
        m_channelList->addChannel(channel);
    }
}

// ============================================================================
// Connection lifecycle
// ============================================================================

void MainWindow::onConnected()
{
    m_inputBar->setEnabled(true);
    statusBar()->showMessage(QString("Connected — %1").arg(m_controller->callsign()));
    m_channelList->setConnectionStatus(true);
    m_activeUsers->clear();
    m_activeUsers->setLocalUser(m_controller->callsign());
    m_activeUsers->addUser(m_controller->callsign());

    if (!m_channelMessages.contains("main"))
        m_channelMessages["main"] = {};
    m_channelList->addChannel("main");
    m_channelList->touchChannel("main");
    switchToChannel("main");
}

void MainWindow::onDisconnected()
{
    m_inputBar->setEnabled(false);
    m_inputBar->setPlaceholder("Connect to a TNC to start chatting...");
    statusBar()->showMessage("Disconnected");
    m_channelList->setConnectionStatus(false);
    m_activeUsers->clear();
}

void MainWindow::onError(const QString& error)
{
    statusBar()->showMessage(QString("Error: %1").arg(error));
    m_chatView->appendMessage(Message::systemMessage("Error: " + error));
}

// ============================================================================
// File transfer
// ============================================================================

void MainWindow::onFileAttachRequested(const QString& filePath)
{
    QFileInfo fi(filePath);
    qint64 size = fi.size();

    // Sanity check — won't work over packet radio
    const qint64 maxSize = 500 * 1024; // 500 KB
    if (size > maxSize) {
        QMessageBox::warning(this, "File too large",
            QString("%1 is %2 KB.\n\n"
                    "Files over 500 KB are impractical over packet radio.\n"
                    "At 1200 baud this would take over an hour.")
                .arg(fi.fileName()).arg(size / 1024));
        return;
    }

    int totalChunks = (size + FileTransferManager::CHUNK_RAW_BYTES - 1)
                      / FileTransferManager::CHUNK_RAW_BYTES;
    if (totalChunks == 0) totalChunks = 1;

    // Time estimate
    double secs = totalChunks * 3.7;
    QString timeStr;
    if (secs < 60)
        timeStr = QString("%1s").arg((int)secs);
    else if (secs < 3600)
        timeStr = QString("%1m %2s").arg((int)secs / 60).arg((int)secs % 60);
    else
        timeStr = QString("%1h %2m").arg((int)secs / 3600).arg(((int)secs % 3600) / 60);

    auto* dlg = new FileTransferDialog(FileTransferDialog::PreSend, this);
    dlg->setFileInfo(filePath, size, totalChunks, timeStr);

    connect(dlg, &FileTransferDialog::accepted, this, [this, filePath, dlg]() {
        dlg->deleteLater();

        // Show send progress immediately — starts as "waiting for receiver"
        m_sendDialog = new FileTransferDialog(FileTransferDialog::SendProgress, this);
        QFileInfo fi(filePath);
        int chunks = (fi.size() + FileTransferManager::CHUNK_RAW_BYTES - 1)
                     / FileTransferManager::CHUNK_RAW_BYTES;
        if (chunks == 0) chunks = 1;
        double secs = chunks * 0.2 + 12.0;
        QString ts = secs < 60 ? QString("%1s").arg((int)secs)
                     : QString("%1m %2s").arg((int)secs / 60).arg((int)secs % 60);
        m_sendDialog->setFileInfo(filePath, fi.size(), chunks, ts);

        connect(m_sendDialog, &FileTransferDialog::cancelled, this, [this]() {
            m_ftManager->cancelSend();
        });

        m_sendDialog->show();
        m_ftManager->startSend(filePath);

        // Show inline chat message for sender
        FileTransferInfo fiSend;
        fiSend.fileId = m_ftManager->sendFileId();
        fiSend.fileName = fi.fileName();
        fiSend.fileSize = fi.size();
        fiSend.totalChunks = chunks;
        fiSend.state = FileTransferInfo::Offering;
        Message msg;
        msg.callsign = m_controller->callsign();
        msg.type = Message::File;
        msg.file = fiSend;
        msg.timestamp = QDateTime::currentDateTime();

        QString channel = m_controller->currentChannel();
        ensureChannelExists(channel);
        m_channelMessages[channel].append(msg);
        m_channelList->touchChannel(channel);

        if (channel == m_controller->currentChannel()) {
            m_fileWidgets[fiSend.fileId] = m_chatView->appendMessageEx(msg);
        }
    });

    connect(dlg, &FileTransferDialog::cancelled, dlg, &QObject::deleteLater);
    dlg->show();
}

void MainWindow::onFileOfferReceived(const FileTransferInfo& info)
{
    auto* dlg = new FileTransferDialog(FileTransferDialog::ReceiveOffer, this);
    dlg->setFileInfo(info.fileName, info.fileSize, info.totalChunks, "");

    connect(dlg, &FileTransferDialog::accepted, this, [this, info, dlg]() {
        dlg->deleteLater();
        m_ftManager->acceptReceive(info.fileId);

        // Show receive progress
        m_recvDialog = new FileTransferDialog(FileTransferDialog::ReceiveProgress, this);
        m_recvDialog->setFileInfo(info.fileName, info.fileSize, info.totalChunks, "");
        m_recvDialog->show();
    });

    connect(dlg, &FileTransferDialog::cancelled, this, [this, info, dlg]() {
        dlg->deleteLater();
        m_ftManager->rejectReceive(info.fileId);
    });

    dlg->show();

    // Show in chat with progress tracking
    FileTransferInfo fiCopy = info;
    fiCopy.state = FileTransferInfo::Offering;
    Message msg;
    msg.callsign = info.fromCallsign;
    msg.type = Message::File;
    msg.file = fiCopy;
    msg.timestamp = QDateTime::currentDateTime();

    QString channel = "main";
    ensureChannelExists(channel);
    m_channelMessages[channel].append(msg);
    m_channelList->touchChannel(channel);

    MessageWidget* w = nullptr;
    if (channel == m_controller->currentChannel()) {
        w = m_chatView->appendMessageEx(msg);
    }
    if (!msg.callsign.isEmpty())
        m_activeUsers->addUser(msg.callsign);

    m_fileWidgets[info.fileId] = w;
}

void MainWindow::onFileProgressUpdate(const QString& fileId, int done, int total)
{
    if (m_recvDialog)
        m_recvDialog->setProgress(done, total);

    auto it = m_fileWidgets.find(fileId);
    if (it != m_fileWidgets.end() && it.value())
        it.value()->updateFileProgress(done, total);
}

void MainWindow::onFileSendProgress(int done, int total)
{
    if (m_sendDialog)
        m_sendDialog->setProgress(done, total);

    // Update sender's inline chat widget
    auto it = m_fileWidgets.find(m_ftManager->sendFileId());
    if (it != m_fileWidgets.end() && it.value())
        it.value()->updateFileProgress(done, total);
}

void MainWindow::onFileComplete(const FileTransferInfo& info)
{
    if (m_recvDialog) {
        m_recvDialog->setComplete(info.savePath);
        connect(m_recvDialog, &FileTransferDialog::accepted, this, [info]() {
            QDesktopServices::openUrl(QUrl::fromLocalFile(
                QFileInfo(info.savePath).absolutePath()));
        });
    }

    auto it = m_fileWidgets.find(info.fileId);
    if (it != m_fileWidgets.end() && it.value()) {
        it.value()->updateFileComplete(info.fileSize);
        if (!info.savePath.isEmpty())
            it.value()->showImage(info.savePath);
    }
}

void MainWindow::onFileFailed(const QString& fileId, const QString& reason)
{
    statusBar()->showMessage(QString("File transfer: %1").arg(reason), 5000);
    m_chatView->appendMessage(Message::systemMessage("File transfer: " + reason));

    auto it = m_fileWidgets.find(fileId);
    if (it != m_fileWidgets.end() && it.value())
        it.value()->updateFileFailed();
}

// ============================================================================
// Settings
// ============================================================================

void MainWindow::openSettings()
{
    SettingsDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        if (m_controller->isConnected())
            m_controller->disconnectTnc();

        m_chatView->clearMessages();
        m_channelMessages.clear();

        m_controller->setCallsign(dialog.callsign());

        m_settings.setValue("tnc/port", dialog.serialPort());
        m_settings.setValue("tnc/baudrate", dialog.baudRate());
        m_settings.setValue("tnc/flowcontrol", dialog.flowControlIndex());
        m_settings.setValue("tnc/txdelay", dialog.txDelay());

        createTnc();

        m_chatView->appendMessage(Message::systemMessage(
            QString("Channel settings saved. Welcome, %1!")
                .arg(m_controller->callsign())));

        if (!dialog.callsign().isEmpty()) {
            QTimer::singleShot(300, this, [this]() {
                m_controller->connectTnc();
            });
        }
    }
}

void MainWindow::loadSettings()
{
    QString callsign = m_settings.value("station/callsign", "").toString();
    m_controller->setCallsign(callsign);

    createTnc();

    if (callsign.isEmpty()) {
        m_channelHeader->setText("# welcome");
        m_chatView->appendMessage(Message::systemMessage(
            "Welcome to WaveChat! Set your callsign and TNC in "
            "File > Settings (Ctrl+,) to get started."));
    } else {
        QTimer::singleShot(500, this, [this]() {
            m_controller->connectTnc();
        });
    }
}

void MainWindow::createTnc()
{
    ITnc* oldTnc = m_controller->tnc();
    m_controller->setTnc(nullptr);
    delete oldTnc;

    auto* kiss = new KissTnc(this);
    kiss->setPortName(m_settings.value("tnc/port", "").toString());
    kiss->setBaudRate(m_settings.value("tnc/baudrate", 9600).toInt());

    int fcIdx = m_settings.value("tnc/flowcontrol", 1).toInt();
    QSerialPort::FlowControl fc = static_cast<QSerialPort::FlowControl>(qBound(0, fcIdx, 2));
    kiss->setFlowControl(fc);

    int txDelay = m_settings.value("tnc/txdelay", 300).toInt();
    kiss->setTxDelay(txDelay);

    m_controller->setTnc(kiss);
}
