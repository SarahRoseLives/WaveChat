#include "SettingsDialog.h"
#include "tnc/PortEnumerator.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QTabWidget>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QLabel>

SettingsDialog::SettingsDialog(QWidget* parent)
    : QDialog(parent)
    , m_settings("WaveChat", "WaveChat")
{
    setWindowTitle("WaveChat Settings");
    setMinimumWidth(440);

    auto* mainLayout = new QVBoxLayout(this);

    auto* tabs = new QTabWidget(this);

    // === Station tab ===
    auto* stationTab = new QWidget(this);
    auto* stationLayout = new QFormLayout(stationTab);

    m_callsignEdit = new QLineEdit(stationTab);
    m_callsignEdit->setPlaceholderText("e.g. W1AW");
    m_callsignEdit->setMaxLength(10);
    QFont callsignFont;
    callsignFont.setPointSize(14);
    callsignFont.setBold(true);
    m_callsignEdit->setFont(callsignFont);
    stationLayout->addRow("Callsign:", m_callsignEdit);

    tabs->addTab(stationTab, "Station");

    // === TNC tab ===
    auto* tncTab = new QWidget(this);
    auto* tncLayout = new QVBoxLayout(tncTab);

    // -- Device group --
    auto* serialGroup = new QGroupBox("Serial / Bluetooth Device", tncTab);
    auto* serialForm = new QFormLayout(serialGroup);

    auto* portRow = new QHBoxLayout();
    m_portCombo = new QComboBox(serialGroup);
    m_portCombo->setMinimumWidth(260);
    portRow->addWidget(m_portCombo, 1);

    auto* refreshBtn = new QPushButton("Refresh", serialGroup);
    refreshBtn->setFixedWidth(70);
    connect(refreshBtn, &QPushButton::clicked, this, &SettingsDialog::refreshPorts);
    portRow->addWidget(refreshBtn);

    serialForm->addRow("Device:", portRow);

    m_baudCombo = new QComboBox(serialGroup);
    m_baudCombo->addItems({"1200", "2400", "4800", "9600", "19200", "38400", "57600", "115200"});
    m_baudCombo->setCurrentText("9600");
    serialForm->addRow("Baud rate:", m_baudCombo);

    m_flowCombo = new QComboBox(serialGroup);
    m_flowCombo->addItem("Hardware (RTS/CTS) — recommended", 1);
    m_flowCombo->addItem("None", 0);
    m_flowCombo->addItem("Software (XON/XOFF)", 2);
    m_flowCombo->setCurrentIndex(0);
    serialForm->addRow("Flow control:", m_flowCombo);

    tncLayout->addWidget(serialGroup);

    // -- Advanced group --
    auto* advancedGroup = new QGroupBox("KISS Timing", tncTab);
    auto* advancedForm = new QFormLayout(advancedGroup);

    m_txDelaySpin = new QSpinBox(advancedGroup);
    m_txDelaySpin->setRange(0, 1000);
    m_txDelaySpin->setValue(300);
    m_txDelaySpin->setSuffix(" ms");
    m_txDelaySpin->setToolTip(
        "Delay between PTT key-up and data start.\n"
        "300ms is safe; lower for faster TNCs, higher for older radios.");
    advancedForm->addRow("TX Delay:", m_txDelaySpin);

    tncLayout->addWidget(advancedGroup);
    tncLayout->addStretch();

    tabs->addTab(tncTab, "TNC");

    mainLayout->addWidget(tabs);

    // === Buttons ===
    auto* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    auto* okButton = buttonBox->button(QDialogButtonBox::Ok);
    okButton->setObjectName("primaryButton");
    connect(buttonBox, &QDialogButtonBox::accepted, this, &SettingsDialog::onAccepted);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttonBox);

    refreshPorts();
    loadSettings();
}

QString SettingsDialog::callsign() const { return m_callsignEdit->text().trimmed().toUpper(); }
QString SettingsDialog::serialPort() const { return m_portCombo->currentData().toString(); }
int SettingsDialog::baudRate() const { return m_baudCombo->currentText().toInt(); }
int SettingsDialog::flowControlIndex() const { return m_flowCombo->currentData().toInt(); }
int SettingsDialog::txDelay() const { return m_txDelaySpin->value(); }

void SettingsDialog::onAccepted()
{
    saveSettings();
    accept();
}

void SettingsDialog::refreshPorts()
{
    QString previous = m_portCombo->currentData().toString();
    m_portCombo->clear();

    const auto ports = PortEnumerator::enumerate();
    for (const auto& info : ports) {
        m_portCombo->addItem(info.displayText, info.portName);
    }

    if (!previous.isEmpty()) {
        for (int i = 0; i < m_portCombo->count(); ++i) {
            if (m_portCombo->itemData(i).toString() == previous) {
                m_portCombo->setCurrentIndex(i);
                return;
            }
        }
    }
}

void SettingsDialog::loadSettings()
{
    QString savedCallsign = m_settings.value("station/callsign", "").toString();
    m_callsignEdit->setText(savedCallsign);

    QString savedPort = m_settings.value("tnc/port", "").toString();
    if (!savedPort.isEmpty()) {
        for (int i = 0; i < m_portCombo->count(); ++i) {
            if (m_portCombo->itemData(i).toString() == savedPort) {
                m_portCombo->setCurrentIndex(i);
                break;
            }
        }
    }

    int savedBaud = m_settings.value("tnc/baudrate", 9600).toInt();
    QString baudStr = QString::number(savedBaud);
    if (m_baudCombo->findText(baudStr) >= 0)
        m_baudCombo->setCurrentText(baudStr);

    int fcIdx = m_settings.value("tnc/flowcontrol", 1).toInt();
    for (int i = 0; i < m_flowCombo->count(); ++i) {
        if (m_flowCombo->itemData(i).toInt() == fcIdx) {
            m_flowCombo->setCurrentIndex(i);
            break;
        }
    }

    m_txDelaySpin->setValue(m_settings.value("tnc/txdelay", 300).toInt());
}

void SettingsDialog::saveSettings()
{
    m_settings.setValue("station/callsign", callsign());
    m_settings.setValue("tnc/port", serialPort());
    m_settings.setValue("tnc/baudrate", baudRate());
    m_settings.setValue("tnc/flowcontrol", flowControlIndex());
    m_settings.setValue("tnc/txdelay", txDelay());
}
