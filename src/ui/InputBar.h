#pragma once
#include <QWidget>
#include <QTextEdit>
#include <QPushButton>

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
    void onAttachClicked();

private:
    QTextEdit* m_edit;
    QPushButton* m_sendButton;
    QPushButton* m_attachButton;

    bool eventFilter(QObject* obj, QEvent* event) override;
};
