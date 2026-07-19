#include "ChatView.h"
#include "MessageWidget.h"
#include <QScrollBar>
#include <QTimer>

ChatView::ChatView(QWidget* parent)
    : QScrollArea(parent)
{
    setObjectName("chatView");
    setWidgetResizable(true);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setFrameShape(QFrame::NoFrame);

    m_contents = new QWidget(this);
    m_contents->setObjectName("chatViewContents");
    m_layout = new QVBoxLayout(m_contents);
    m_layout->setContentsMargins(0, 8, 0, 8);
    m_layout->setSpacing(0);
    m_layout->addStretch();

    setWidget(m_contents);

    // Auto-scroll to bottom when content range grows
    connect(verticalScrollBar(), &QScrollBar::rangeChanged, this, [this](int, int) {
        if (m_autoScroll)
            verticalScrollBar()->setValue(verticalScrollBar()->maximum());
    });

    // Detect user scrolling away from bottom → pause auto-scroll
    // Detect user scrolling back to bottom → resume auto-scroll
    connect(verticalScrollBar(), &QScrollBar::valueChanged, this, [this](int value) {
        int max = verticalScrollBar()->maximum();
        if (max <= 0) return;
        if (value < max - 20)
            m_autoScroll = false;
        else
            m_autoScroll = true;
    });
}

void ChatView::appendMessage(const Message& msg)
{
    if (m_layout->count() > 0) {
        QLayoutItem* item = m_layout->takeAt(m_layout->count() - 1);
        delete item;
    }

    m_layout->addWidget(new MessageWidget(msg, m_contents));
    m_layout->addStretch();
}

void ChatView::clearMessages()
{
    while (m_layout->count() > 1) {
        QLayoutItem* item = m_layout->takeAt(0);
        if (item->widget())
            delete item->widget();
        delete item;
    }
}
