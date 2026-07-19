#include "InputBar.h"
#include "EmojiPicker.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QKeyEvent>
#include <QTextCursor>
#include <QApplication>
#include <QScreen>
#include <QFileDialog>

InputBar::InputBar(QWidget* parent)
    : QWidget(parent)
{
    setObjectName("inputBar");

    auto* outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(0, 0, 0, 0);
    outerLayout->setSpacing(0);

    auto* separator = new QWidget(this);
    separator->setFixedHeight(1);
    separator->setStyleSheet("background-color: #2f3136;");
    outerLayout->addWidget(separator);

    auto* rowLayout = new QHBoxLayout();
    rowLayout->setContentsMargins(16, 12, 16, 16);
    rowLayout->setSpacing(6);

    m_attachButton = new QPushButton("\xF0\x9F\x93\x8E", this);
    m_attachButton->setObjectName("attachButton");
    m_attachButton->setFixedSize(36, 36);
    m_attachButton->setToolTip("Attach a file");
    m_attachButton->setCursor(Qt::PointingHandCursor);

    m_emojiButton = new QPushButton("\xF0\x9F\x98\x8A", this);
    m_emojiButton->setObjectName("emojiButton");
    m_emojiButton->setFixedSize(36, 36);
    m_emojiButton->setToolTip("Emoji picker");
    m_emojiButton->setCursor(Qt::PointingHandCursor);

    m_edit = new QTextEdit(this);
    m_edit->setObjectName("messageEdit");
    m_edit->setFixedHeight(44);
    m_edit->setPlaceholderText("Message #channel");
    m_edit->setAcceptRichText(false);
    m_edit->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_edit->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_edit->setTabChangesFocus(false);
    m_edit->installEventFilter(this);

    m_sendButton = new QPushButton("Send", this);
    m_sendButton->setObjectName("sendButton");
    m_sendButton->setFixedHeight(44);
    m_sendButton->setMinimumWidth(72);
    m_sendButton->setEnabled(false);

    connect(m_attachButton, &QPushButton::clicked, this, &InputBar::onAttachClicked);
    connect(m_emojiButton, &QPushButton::clicked, this, &InputBar::onEmojiClicked);
    connect(m_sendButton, &QPushButton::clicked, this, &InputBar::onSendClicked);

    connect(m_edit, &QTextEdit::textChanged, this, [this]() {
        bool hasText = !m_edit->toPlainText().trimmed().isEmpty();
        m_sendButton->setEnabled(m_edit->isEnabled() && hasText);
    });

    rowLayout->addWidget(m_attachButton, 0, Qt::AlignBottom);
    rowLayout->addWidget(m_emojiButton, 0, Qt::AlignBottom);
    rowLayout->addWidget(m_edit, 1);
    rowLayout->addWidget(m_sendButton, 0, Qt::AlignBottom);

    outerLayout->addLayout(rowLayout);
}

void InputBar::setEnabled(bool enabled)
{
    m_edit->setEnabled(enabled);
    bool hasText = !m_edit->toPlainText().trimmed().isEmpty();
    m_sendButton->setEnabled(enabled && hasText);
    m_emojiButton->setEnabled(enabled);
    m_attachButton->setEnabled(enabled);
    if (!enabled)
        m_edit->setPlaceholderText("Connect to a TNC to start chatting...");
}

void InputBar::setPlaceholder(const QString& text)
{
    m_edit->setPlaceholderText(text);
}

void InputBar::clear()
{
    m_edit->clear();
}

void InputBar::onSendClicked()
{
    QString text = m_edit->toPlainText().trimmed();
    if (!text.isEmpty()) {
        emit messageSubmitted(text);
        clear();
    }
}

void InputBar::onAttachClicked()
{
    QString path = QFileDialog::getOpenFileName(this, "Select file to send");
    if (!path.isEmpty())
        emit fileAttachRequested(path);
}

void InputBar::onEmojiClicked()
{
    if (!m_emojiPicker) {
        m_emojiPicker = new EmojiPicker(nullptr);
        connect(m_emojiPicker, &EmojiPicker::emojiSelected,
                this, &InputBar::onEmojiSelected);
    }

    QPoint btnPos = m_emojiButton->mapToGlobal(QPoint(0, 0));
    int x = btnPos.x();
    int y = btnPos.y() - m_emojiPicker->height() - 4;
    if (y < 0) y = btnPos.y() + m_emojiButton->height() + 4;

    m_emojiPicker->move(x, y);
    m_emojiPicker->show();
}

void InputBar::onEmojiSelected(const QString& emoji)
{
    QTextCursor cursor = m_edit->textCursor();
    cursor.insertText(emoji);
    m_edit->setFocus();
}

bool InputBar::eventFilter(QObject* obj, QEvent* event)
{
    if (obj == m_edit && event->type() == QEvent::KeyPress) {
        auto* keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_Return
            || keyEvent->key() == Qt::Key_Enter) {
            if (keyEvent->modifiers() & Qt::ShiftModifier) {
                return false;
            } else {
                onSendClicked();
                return true;
            }
        }
    }
    return QWidget::eventFilter(obj, event);
}
