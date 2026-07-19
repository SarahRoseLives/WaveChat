#pragma once
#include <QScrollArea>
#include <QVBoxLayout>
#include <QWidget>
#include "chat/Message.h"

class ChatView : public QScrollArea {
    Q_OBJECT
public:
    explicit ChatView(QWidget* parent = nullptr);

    void appendMessage(const Message& msg);
    void clearMessages();

private:
    QWidget* m_contents;
    QVBoxLayout* m_layout;
    bool m_autoScroll = true;
};
