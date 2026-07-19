#pragma once
#include <QWidget>
#include <QTextEdit>
#include <QPushButton>

class EmojiPicker;

class InputBar : public QWidget {
    Q_OBJECT
public:
    explicit InputBar(QWidget* parent = nullptr);

    void setEnabled(bool enabled);
    void setPlaceholder(const QString& text);
    void clear();

signals:
    void messageSubmitted(const QString& text);

private slots:
    void onSendClicked();
    void onEmojiClicked();
    void onEmojiSelected(const QString& emoji);

private:
    QTextEdit* m_edit;
    QPushButton* m_sendButton;
    QPushButton* m_emojiButton;
    EmojiPicker* m_emojiPicker = nullptr;

    bool eventFilter(QObject* obj, QEvent* event) override;
};
