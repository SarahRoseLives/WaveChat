#include "MainWindow.h"
#include "SettingsDialog.h"
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

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_controller(new ChatController(this))
    , m_settings("WaveChat", "WaveChat")
{
    setupUi();
    setupMenuBar();
    setupStatusBar();

    connect(m_controller, &ChatController::messageReceived,
            this, &MainWindow::onMessageReceived);
    connect(m_controller, &ChatController::connected,
            this, &MainWindow::onConnected);
    connect(m_controller, &ChatController::disconnected,
            this, &MainWindow::onDisconnected);
    connect(m_controller, &ChatController::errorOccurred,
            this, &MainWindow::onError);

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

    if (msg.type == Message::Text) {
        m_channelList->touchChannel(channel);
    }

    if (channel == m_controller->currentChannel()) {
        m_chatView->appendMessage(msg);
    } else {
        // Mark unread — only for incoming text messages, not system messages
        if (msg.type == Message::Text) {
            m_channelList->setChannelUnread(channel, true);
        }
    }

    if (msg.type == Message::Text && !msg.callsign.isEmpty()) {
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
    // Clear unread for the old channel
    QString oldChannel = m_controller->currentChannel();
    if (!oldChannel.isEmpty()) {
        m_channelList->setChannelUnread(oldChannel, false);
    }

    m_controller->setCurrentChannel(channel);

    m_chatView->clearMessages();
    m_channelHeader->setText(QString("# %1").arg(channel));

    m_inputBar->setPlaceholder(
        QString("Message #%1").arg(channel));

    m_channelList->setActiveChannel(channel);

    // Clear unread for this channel (we're viewing it now)
    m_channelList->setChannelUnread(channel, false);

    // Replay stored messages for this channel
    if (m_channelMessages.contains(channel)) {
        for (const auto& msg : m_channelMessages[channel]) {
            m_chatView->appendMessage(msg);
        }
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
    statusBar()->showMessage(QString("Connected — %1")
                                 .arg(m_controller->callsign()));
    m_channelList->setConnectionStatus(true);

    m_activeUsers->clear();
    m_activeUsers->setLocalUser(m_controller->callsign());
    m_activeUsers->addUser(m_controller->callsign());

    // Default to "main" channel
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
    QSerialPort::FlowControl fc = static_cast<QSerialPort::FlowControl>(
        qBound(0, fcIdx, 2));
    kiss->setFlowControl(fc);

    int txDelay = m_settings.value("tnc/txdelay", 300).toInt();
    kiss->setTxDelay(txDelay);

    m_controller->setTnc(kiss);
}
