#pragma once
#include <QWidget>
#include <QStringList>

class EmojiPicker : public QWidget {
    Q_OBJECT
public:
    explicit EmojiPicker(QWidget* parent = nullptr);

signals:
    void emojiSelected(const QString& emoji);

private:
    QStringList m_emojis;
    void buildGrid();
};
