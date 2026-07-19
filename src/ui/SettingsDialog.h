#pragma once
#include <QDialog>
#include <QLineEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QSettings>

class SettingsDialog : public QDialog {
    Q_OBJECT
public:
    explicit SettingsDialog(QWidget* parent = nullptr);

    QString callsign() const;
    QString frequency() const;
    QString serialPort() const;
    int baudRate() const;
    int flowControlIndex() const;
    int txDelay() const;

private slots:
    void onAccepted();
    void refreshPorts();

private:
    QLineEdit* m_callsignEdit;
    QLineEdit* m_frequencyEdit;
    QComboBox* m_portCombo;
    QComboBox* m_baudCombo;
    QComboBox* m_flowCombo;
    QSpinBox* m_txDelaySpin;

    QSettings m_settings;

    void loadSettings();
    void saveSettings();
};
