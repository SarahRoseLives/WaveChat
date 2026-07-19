#include "EmojiPicker.h"
#include <QVBoxLayout>
#include <QGridLayout>
#include <QPushButton>
#include <QLabel>
#include <QFont>

EmojiPicker::EmojiPicker(QWidget* parent)
    : QWidget(parent, Qt::Popup)
{
    setObjectName("emojiPicker");
    setFixedSize(360, 280);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(6, 6, 6, 6);
    layout->setSpacing(2);

    auto* header = new QLabel("Pick an emoji", this);
    header->setObjectName("emojiPickerHeader");
    QFont hf;
    hf.setPointSize(9);
    hf.setBold(true);
    header->setFont(hf);
    layout->addWidget(header);

    m_emojis = QStringList{
        "\xF0\x9F\x98\x8A", // 😊
        "\xF0\x9F\x98\x82", // 😂
        "\xF0\x9F\x91\x8D", // 👍
        "\xF0\x9F\x91\x8B", // 👋
        "\xF0\x9F\x94\xA5", // 🔥
        "\xF0\x9F\xA4\x94", // 🤔
        "\xF0\x9F\x92\xAF", // 💯
        "\xF0\x9F\x8E\x89", // 🎉
        "\xF0\x9F\x99\x8F", // 🙏
        "\xE2\x9D\xA4\xEF\xB8\x8F", // ❤️
        "\xF0\x9F\x91\x80", // 👀
        "\xE2\x9C\x85", // ✅
        "\xE2\x9D\x8C", // ❌
        "\xE2\x9A\xA1", // ⚡
        "\xF0\x9F\x93\xBB", // 📻
        "\xF0\x9F\x9B\xB0\xEF\xB8\x8F", // 🛰️
        "\xF0\x9F\x8C\xA4\xEF\xB8\x8F", // 🌤️
        "\xF0\x9F\x8C\x99", // 🌙
        "\xF0\x9F\x8F\x94\xEF\xB8\x8F", // 🏔️
        "\xF0\x9F\x97\xBA\xEF\xB8\x8F", // 🗺️
        "\xF0\x9F\xA7\xAD", // 🧭
        "\xF0\x9F\x94\xA7", // 🔧
        "\xF0\x9F\x92\xAC", // 💬
        "\xF0\x9F\x8E\x99\xEF\xB8\x8F", // 🎙️
        "\xF0\x9F\x94\x8A", // 🔊
        "\xF0\x9F\x93\xA1", // 📡
        "\xF0\x9F\x8F\xA0", // 🏠
        "\xF0\x9F\x9A\x97", // 🚗
        "\xE2\x9C\x88\xEF\xB8\x8F", // ✈️
        "\xF0\x9F\x9A\x81", // 🚁
        "\xE2\x9B\xB5", // ⛵
        "\xF0\x9F\x8C\x8A", // 🌊
        "\xE2\x9A\x93", // ⚓
        "\xF0\x9F\x97\xBC", // 🗼
        "\xF0\x9F\x8F\x86", // 🏆
        "\xF0\x9F\x8E\xAF", // 🎯
        "\xF0\x9F\x92\xA1", // 💡
        "\xF0\x9F\x94\x91", // 🔑
        "\xE2\x9A\xA0\xEF\xB8\x8F", // ⚠️
        "\xF0\x9F\x86\x98", // 🆘
        "\xE2\x84\xB9\xEF\xB8\x8F", // ℹ️
        "\xF0\x9F\x98\x8E", // 😎
        "\xF0\x9F\x98\xB1", // 😱
        "\xF0\x9F\xA4\xAF", // 🤯
        "\xF0\x9F\x98\xA1", // 😡
        "\xF0\x9F\x98\xAD", // 😭
        "\xF0\x9F\xA5\xB3", // 🥳
        "\xF0\x9F\x91\x8F", // 👏
    };

    buildGrid();
}

void EmojiPicker::buildGrid()
{
    auto* grid = new QGridLayout();
    grid->setContentsMargins(0, 0, 0, 0);
    grid->setSpacing(1);

    const int cols = 8;
    for (int i = 0; i < m_emojis.size(); ++i) {
        auto* btn = new QPushButton(m_emojis[i], this);
        btn->setObjectName("pickerEmoji");
        btn->setFixedSize(40, 36);
        QFont f;
        f.setPointSize(13);
        btn->setFont(f);
        btn->setCursor(Qt::PointingHandCursor);

        connect(btn, &QPushButton::clicked, this, [this, i]() {
            emit emojiSelected(m_emojis[i]);
            close();
        });

        int row = i / cols;
        int col = i % cols;
        grid->addWidget(btn, row, col);
    }

    static_cast<QVBoxLayout*>(layout())->addLayout(grid);
}
