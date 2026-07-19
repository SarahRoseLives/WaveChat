#pragma once
#include <QWidget>
#include <QLabel>
#include <QProgressBar>
#include "chat/Message.h"

class MessageWidget : public QWidget {
    Q_OBJECT
public:
    explicit MessageWidget(const Message& msg, QWidget* parent = nullptr);

private:
    QLabel* m_avatar;
    QLabel* m_callsignLabel;
    QLabel* m_timestampLabel;
    QLabel* m_textLabel;
    QProgressBar* m_progressBar = nullptr;

    static QString avatarColor(const QString& callsign);
};
