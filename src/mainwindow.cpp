/* Copyright (C) 2022  Dmitry Serov
 *
 * This file is part of MControlCenter.
 *
 * MControlCenter is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * MControlCenter is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with MControlCenter. If not, see <https://www.gnu.org/licenses/>.
 */

#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "operate.h"
#include "settings.h"
#include <QTimer>
#include <QMessageBox>
#include <QJsonObject>
#include <QJsonDocument>
#include <QFile>
#include <QProcess>
#include <QStandardPaths>
#include <QDateTime>
#include <QDir>
#include <QDBusReply>
#include <QDBusVariant>
#include <QDBusConnectionInterface>
#include <QEventLoop>
#include <QDesktopServices>
#include <QUrl>
#include <QClipboard>
#include <QGuiApplication>
#include <QDBusInterface>
#include <QSignalBlocker>
#include <QSlider>
#include <QSpinBox>
#include <QSysInfo>
#include <QElapsedTimer>
#include <QRegularExpression>
#include <QDBusConnection>
#include "theme.h"

static QString slurp(const QString &path) { QFile f(path); return f.open(QIODevice::ReadOnly) ? QString::fromUtf8(f.readAll()).trimmed() : QString(); }
Operate operate;
// operate.isMsiEcLoaded() is a blocking D-Bus round trip to the root helper (~60 ms each); this is the same answer from the filesystem.
static bool msiEcPresent() { return QFile::exists("/sys/devices/platform/msi-ec/shift_mode"); }
PowerMonitor powerMonitor;

bool isActive = false;
bool isUpdateDataError = false;

QTimer *realtimeUpdateTimer = new QTimer;

MainWindow::MainWindow(QWidget *parent)
        : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);

    connect(ui->advancedFanControlCheckBox, &QCheckBox::toggled, this, &MainWindow::setFanModeAdvanced);

    connect(ui->fan1Speed1Slider, &QSlider::valueChanged, this, [this]() {
        ui->fan1Speed1Label->setText(QString("%1%").arg(ui->fan1Speed1Slider->value()));
        checkFanSettingsChanged();
    });
    connect(ui->fan1Speed2Slider, &QSlider::valueChanged, this, [this]() {
        ui->fan1Speed2Label->setText(QString("%1%").arg(ui->fan1Speed2Slider->value()));
        checkFanSettingsChanged();
    });
    connect(ui->fan1Speed3Slider, &QSlider::valueChanged, this, [this]() {
        ui->fan1Speed3Label->setText(QString("%1%").arg(ui->fan1Speed3Slider->value()));
        checkFanSettingsChanged();
    });
    connect(ui->fan1Speed4Slider, &QSlider::valueChanged, this, [this]() {
        ui->fan1Speed4Label->setText(QString("%1%").arg(ui->fan1Speed4Slider->value()));
        checkFanSettingsChanged();
    });
    connect(ui->fan1Speed5Slider, &QSlider::valueChanged, this, [this]() {
        ui->fan1Speed5Label->setText(QString("%1%").arg(ui->fan1Speed5Slider->value()));
        checkFanSettingsChanged();
    });
    connect(ui->fan1Speed6Slider, &QSlider::valueChanged, this, [this]() {
        ui->fan1Speed6Label->setText(QString("%1%").arg(ui->fan1Speed6Slider->value()));
        checkFanSettingsChanged();
    });
    connect(ui->fan1Speed7Slider, &QSlider::valueChanged, this, [this]() {
        ui->fan1Speed7Label->setText(QString("%1%").arg(ui->fan1Speed7Slider->value()));
        checkFanSettingsChanged();
    });

    connect(ui->fan1Speed2TempSpinBox, &QSpinBox::textChanged, this, [this]() {
        ui->fan1Speed1TempLabel->setText(QString("< %1 °C").arg(ui->fan1Speed2TempSpinBox->value()));
        checkFanSettingsChanged();
    });
    connect(ui->fan1Speed3TempSpinBox, &QSpinBox::textChanged, this, [this]() {
        checkFanSettingsChanged();
    });
    connect(ui->fan1Speed4TempSpinBox, &QSpinBox::textChanged, this, [this]() {
        checkFanSettingsChanged();
    });
    connect(ui->fan1Speed5TempSpinBox, &QSpinBox::textChanged, this, [this]() {
        checkFanSettingsChanged();
    });
    connect(ui->fan1Speed6TempSpinBox, &QSpinBox::textChanged, this, [this]() {
        checkFanSettingsChanged();
    });
    connect(ui->fan1Speed7TempSpinBox, &QSpinBox::textChanged, this, [this]() {
        checkFanSettingsChanged();
    });

    connect(ui->fan2Speed1Slider, &QSlider::valueChanged, this, [this]() {
        ui->fan2Speed1Label->setText(QString("%1%").arg(ui->fan2Speed1Slider->value()));
        checkFanSettingsChanged();
    });
    connect(ui->fan2Speed2Slider, &QSlider::valueChanged, this, [this]() {
        ui->fan2Speed2Label->setText(QString("%1%").arg(ui->fan2Speed2Slider->value()));
        checkFanSettingsChanged();
    });
    connect(ui->fan2Speed3Slider, &QSlider::valueChanged, this, [this]() {
        ui->fan2Speed3Label->setText(QString("%1%").arg(ui->fan2Speed3Slider->value()));
        checkFanSettingsChanged();
    });
    connect(ui->fan2Speed4Slider, &QSlider::valueChanged, this, [this]() {
        ui->fan2Speed4Label->setText(QString("%1%").arg(ui->fan2Speed4Slider->value()));
        checkFanSettingsChanged();
    });
    connect(ui->fan2Speed5Slider, &QSlider::valueChanged, this, [this]() {
        ui->fan2Speed5Label->setText(QString("%1%").arg(ui->fan2Speed5Slider->value()));
        checkFanSettingsChanged();
    });
    connect(ui->fan2Speed6Slider, &QSlider::valueChanged, this, [this]() {
        ui->fan2Speed6Label->setText(QString("%1%").arg(ui->fan2Speed6Slider->value()));
        checkFanSettingsChanged();
    });
    connect(ui->fan2Speed7Slider, &QSlider::valueChanged, this, [this]() {
        ui->fan2Speed7Label->setText(QString("%1%").arg(ui->fan2Speed7Slider->value()));
        checkFanSettingsChanged();
    });

    connect(ui->fan2Speed2TempSpinBox, &QSpinBox::textChanged, this, [this]() {
        ui->fan2Speed1TempLabel->setText(QString("< %1 °C").arg(ui->fan2Speed2TempSpinBox->value()));
        checkFanSettingsChanged();
    });
    connect(ui->fan2Speed3TempSpinBox, &QSpinBox::textChanged, this, [this]() {
        checkFanSettingsChanged();
    });
    connect(ui->fan2Speed4TempSpinBox, &QSpinBox::textChanged, this, [this]() {
        checkFanSettingsChanged();
    });
    connect(ui->fan2Speed5TempSpinBox, &QSpinBox::textChanged, this, [this]() {
        checkFanSettingsChanged();
    });
    connect(ui->fan2Speed6TempSpinBox, &QSpinBox::textChanged, this, [this]() {
        checkFanSettingsChanged();
    });
    connect(ui->fan2Speed7TempSpinBox, &QSpinBox::textChanged, this, [this]() {
        checkFanSettingsChanged();
    });

    connect(ui->fanSpeedResetButton, &QPushButton::clicked, this, &MainWindow::updateFanSpeedSettings);
    connect(ui->fanSpeedApplyButton, &QPushButton::clicked, this, &MainWindow::setFanSpeedSettings);
    connect(&powerMonitor, &PowerMonitor::currentChargerState, this, &MainWindow::on_ChargerStateChange);
    connect(&powerMonitor, &PowerMonitor::currentPowerProfile, this, &MainWindow::on_PowerProfileChange);

    connect(qApp, &QGuiApplication::saveStateRequest, this, &MainWindow::saveStateRequest);

    MainWindow::setWindowIcon(QIcon(":/images/AppIcon"));
    Settings s;
    if (s.isValueExist("MainWindow/Width") && s.isValueExist("MainWindow/Height") && s.getValue("MainWindow/Width").toInt() >= 1000)
        MainWindow::resize(s.getValue("MainWindow/Width").toInt(), s.getValue("MainWindow/Height").toInt());
    else
        MainWindow::resize(1100, 780);

    createTrayIcon();

    setStyleSheet(kThemeQss);
    dashboard = new DashboardPage(this);
    dashboard->setMinimumSize(980, 700);
    ui->tabWidget->insertTab(0, dashboard, tr("Dashboard"));
    ui->tabWidget->tabBar()->setFocusPolicy(Qt::NoFocus);
    ui->tabWidget->setTabText(1, tr("Controls"));
    for (int i = 0; i < ui->tabWidget->count(); ++i) ui->tabWidget->setTabIcon(i, QIcon());
    ui->tabWidget->setCurrentIndex(0);
    connect(dashboard, &DashboardPage::modeRequested, this, &MainWindow::setModeIndex);
    connect(dashboard, &DashboardPage::coolerBoostRequested, this, [this](bool on) { setCoolerBoostState(on); });
    connect(dashboard, &DashboardPage::chargeLimitRequested, this, &MainWindow::setChargeLimit);
    sensors = new SensorWorker;                       // all sysfs/EC reads live on this thread; the GUI thread only draws
    sensors->moveToThread(&sensorThread);
    connect(&sensorThread, &QThread::started, sensors, &SensorWorker::start);
    connect(&sensorThread, &QThread::finished, sensors, &QObject::deleteLater);
    connect(sensors, &SensorWorker::snapshot, this, [this](const Snapshot &sn) {
        int r0 = dashData.cpuFanRpm, r1 = dashData.gpuFanRpm; if (sn.msiEc) { dashData = sn.d; dashData.cpuFanRpm = r0; dashData.gpuFanRpm = r1; }
        dashboard->applySnapshot(sn); syncPages(sn); thermalWatch(sn);
    });
    sensorThread.start();
    bridge = new ControlBridge(this);
    bridge->stateFn = [this] { QMetaObject::invokeMethod(sensors, "pollNow", Qt::QueuedConnection); return stateJson(); };   // last snapshot now, a fresh one is already on its way
    connect(bridge, &ControlBridge::modeRequested, this, &MainWindow::setModeIndex);
    connect(bridge, &ControlBridge::coolerBoostRequested, this, [this](bool on) { setCoolerBoostState(on); });
    connect(bridge, &ControlBridge::chargeLimitRequested, this, &MainWindow::setChargeLimit);
    connect(bridge, &ControlBridge::showRequested, this, [this] { showNormal(); raise(); activateWindow(); });
    QDBusConnection::sessionBus().registerObject("/Control", bridge, QDBusConnection::ExportAllSlots | QDBusConnection::ExportAllSignals);

    ui->tabWidget->tabBar()->setExpanding(true);
    buildPages();
    setTabsEnabled(false);

    connect(dashboard, &DashboardPage::fixRequested, this, &MainWindow::runDoctorFix);
    checkDriver();

    if (!operate.isEcSysModuleLoaded() && !operate.loadEcSysModule())
        QMessageBox::critical(nullptr, this->windowTitle(), tr("The ec_sys module couldn't be detected, it might be required to control the fans."));



    if(operate.updateEcData())
        updateData();

    connect(realtimeUpdateTimer, &QTimer::timeout, this, &MainWindow::realtimeUpdate);
    setUpdateInterval(1000);

    // Timer to detect sleep and reapply Advanced Mode Fan if necessary
    connect(&timerSleepWatcher, &QTimer::timeout, this, &MainWindow::timerSleepTimeout);
    timerSleepWatcher.setInterval(10 * 1000);
    timerSleepWatcher.start();

    if (const char *shot = getenv("MCC_SHOT")) {            // dev only: render every tab to PNGs and quit
        QString prefix = QString::fromUtf8(shot); resize(1100, 760); show();
        QTimer::singleShot(2500, this, [this, prefix] {
            for (int i = 0; i < ui->tabWidget->count(); ++i) {
                if (!ui->tabWidget->isTabVisible(i)) continue; ui->tabWidget->setCurrentIndex(i);
                QEventLoop loop; QTimer::singleShot(1300, &loop, &QEventLoop::quit); loop.exec();            // let the entrance animation finish
                grab().save(QString("%1_%2.png").arg(prefix).arg(i));
            }
            QCoreApplication::quit();
        });
    }
    ui->QtVersionValue->setText(QT_VERSION_STR);
    ui->versionValueLabel->setText(MControlCenter_VERSION);
    ui->autoAcDcProfilesGroupBox->setChecked(s.getValue("Settings/autoAcDcProfilesState").toBool());
    ui->userModeOnBatteryComboBox->setCurrentIndex(s.getValue("Settings/UserModeOnBattery").toInt());
    ui->userModeOnChargerComboBox->setCurrentIndex(s.getValue("Settings/UserModeOnCharger").toInt());
    ui->autoPPDCheckBox->setChecked(s.getValue("Settings/autoPPDstate").toBool());
}

MainWindow::~MainWindow() {
    sensorThread.quit(); sensorThread.wait(2000);
    delete ui;
}

void MainWindow::setUpdateDataError(bool error) {
    isUpdateDataError = error;
}

void MainWindow::setTabsEnabled(bool enabled) {
    if (dashboard) dashboard->setControlsEnabled(enabled);
    for (QWidget *w : std::initializer_list<QWidget *>{controlsPage, batteryPage, fanPage}) if (w) w->setEnabled(enabled);
    ui->overviewTab->setEnabled(enabled);
    ui->modeFormWidget->setEnabled(enabled);
    ui->batteryTab->setEnabled(enabled);
    ui->fanControlTab->setEnabled(enabled);
    ui->keyboardTab->setEnabled(enabled);
    ui->debugTab->setEnabled(enabled);

    if (modeTrayMenu)
        modeTrayMenu->setEnabled(enabled);
    if (batteryTrayMenu)
        batteryTrayMenu->setEnabled(enabled);
}

void MainWindow::startRealtimeUpdate() const {
    realtimeUpdateTimer->start();
}

void MainWindow::stopRealtimeUpdate() const {
    realtimeUpdateTimer->stop();
}

void MainWindow::setUpdateInterval(int msec) const {
    realtimeUpdateTimer->setInterval(msec);
}

void MainWindow::realtimeUpdate() {
    operate.updateEcDataAsync();
    updateData();
}

void MainWindow::updateData() {
    if (!isUpdateDataError && (msiEcPresent() || operate.isEcSysModuleLoaded())) {
        if (!isActive) {
            operate.doProbe();
            setTabsEnabled(true);
            loadConfigs();
            probeCaps();
            isActive = true;
        }
        // The old per-tab labels do slow EC reads: only refresh them while one of those tabs is on screen (never while on the Dashboard).
        if (ui->tabWidget->currentWidget() != dashboard) {
            updateBatteryCharge(); updateChargingStatus(); updateCpuTemp(); updateGpuTemp(); updateFanMode();
            updateFan1Speed(); updateFan2Speed(); updateKeyboardBrightness(); updateWebCamState();
        }
        pushDashboard();

        if (msiEcPresent()) {
            ui->MsiEcStatusLabel->setText(tr("Loaded"));
        } else {
            ui->MsiEcStatusLabel->setText(tr("Fallback: Only ec_sys is loaded"));
        }
    } else {
        setTabsEnabled(false);
        ui->MsiEcStatusLabel->setText(tr("Failed to load both msi-ec/ec_sys"));
        isActive = false;
    }
}

void MainWindow::loadConfigs() {
    ui->ecVersionValueLabel->setText(QString::fromStdString(operate.getEcVersion()));
    ui->ecBuildValueLabel->setText(QString::fromStdString(operate.getEcBuild()));

    operate.loadSettings();
    updateUserMode();
    updateCoolerBoostState();

    if (operate.isBatteryThresholdSupport()) {
        updateBatteryThreshold();
    } else {
        ui->batteryTab->setEnabled(false);
        if (batteryTrayMenu)
            batteryTrayMenu->setEnabled(false);
    }

    updateFanSpeedSettings();

    if (operate.isKeyboardBacklightModeSupport()) {
        updateKeyboardBacklightMode();
    } else {
        ui->keyboardBacklightModeComboBox->setEnabled(false);
    }

    if (operate.isKeyboardBacklightSupport()) {
        updateKeyboardBrightness();
    } else {
        ui->keyboardBrightnessSlider->setEnabled(false);
        ui->tabWidget->removeTab(ui->tabWidget->indexOf(ui->keyboardTab));
    }

    if (operate.isUsbPowerShareSupport()) {
        updateUsbPowerShareState();
    } else {
        ui->usbPowerShareCheckBox->setEnabled(false);
    }

    if (operate.isWebCamOffSupport()) {
        updateWebCamState();
    } else {
        ui->webCamCheckBox->setEnabled(false);
    }

    updateFnSuperSwapState();
}

QString MainWindow::intToQString(int value) const {
    if (value < 0)
        return "-";
    return QString::number(value);
}

void MainWindow::updateBatteryCharge() {
    ui->BatteryChargeValueLabel->setText(intToQString(operate.getBatteryCharge()) + " %");
}

void MainWindow::updateBatteryThreshold() {
    if (operate.updateEcData()) {
        int batteryThreshold = operate.getBatteryThreshold();
        if (batteryThreshold == 0)
            ui->batteryThresholdValueLabel->setText("100 %");
        else
            ui->batteryThresholdValueLabel->setText(QString::number(batteryThreshold) + " %");

        switch (batteryThreshold) {
            case 0:
            case 100:
                ui->bestMobilityRadioButton->click();
                batteryThreshold = 100;
                break;
            case 60:
                ui->bestBatteryRadioButton->click();
                break;
            case 80:
                ui->balancedBatteryRadioButton->click();
                break;
            default:
                ui->customBatteryThresholdRadioButton->click();
                ui->customBatteryApplyButton->setEnabled(
                        batteryThreshold != ui->customBatteryThresholdSpinBox->value());
                break;
        }

        ui->customBatteryThresholdSpinBox->setValue(batteryThreshold);
    }
}

void MainWindow::updateChargingStatus() {
    QString chargingStatus;
    switch (operate.getChargingStatus()) {
        case charging_state::battery_charging:
            chargingStatus = tr("Charging");
            break;
        case charging_state::battery_discharging:
            chargingStatus = tr("Discharging");
            break;
        case charging_state::battery_not_charging:
            chargingStatus = tr("Not charging");
            break;
        case charging_state::battery_fully_charged:
            chargingStatus = tr("Fully charged");
            break;
        default:
            chargingStatus = tr("Unknown");
            break;
    }
    ui->chargingStatusValueLabel->setText(chargingStatus);
}

void MainWindow::updateCpuTemp() {
    ui->cpuTempValueLabel->setText(intToQString(operate.getCpuTemp()) + " °C");
}

void MainWindow::updateGpuTemp() {
    std::optional<int> temp = operate.getGpuTemp();
    if (temp.has_value()) {
        ui->gpuTempValueLabel->setVisible(true);
        ui->gpuTempLabel->setVisible(true);
        if (temp.value() != 0)
            ui->gpuTempValueLabel->setText(intToQString(temp.value()) + " °C");
        else
            ui->gpuTempValueLabel->setText(tr("OFF"));
    } else {
        ui->gpuTempValueLabel->setVisible(false);
        ui->gpuTempLabel->setVisible(false);
    }
}

void MainWindow::updateFan1Speed() {
    int speed = operate.getFan1Speed();
    if (speed != 0)
        ui->fan1ValueLabel->setText(intToQString(speed) + " " + tr("rpm"));
    else
        ui->fan1ValueLabel->setText(tr("OFF"));
}

void MainWindow::updateFan2Speed() {
    std::optional<int> speed = operate.getFan2Speed();
    if (speed.has_value()) {
        ui->fan2ValueLabel->setVisible(true);
        ui->gpuFanLabel->setVisible(true);
        if (speed.value() != 0)
            ui->fan2ValueLabel->setText(intToQString(speed.value()) + " " + tr("rpm"));
        else
            ui->fan2ValueLabel->setText(tr("OFF"));
    } else {
        ui->fan2ValueLabel->setVisible(false);
        ui->gpuFanLabel->setVisible(false);
        ui->fanControlTabWidget->setTabEnabled(1, false);
    }
}

void MainWindow::updateKeyboardBacklightMode() {
    ui->keyboardBacklightModeComboBox->setCurrentIndex(operate.getKeyboardBacklightMode());
}

void MainWindow::updateKeyboardBrightness() const {
    ui->keyboardBrightnessSlider->setSliderPosition(operate.getKeyboardBrightness());
}

void MainWindow::updateUsbPowerShareState() {
    ui->usbPowerShareCheckBox->setChecked(operate.getUsbPowerShareState());
}

void MainWindow::updateWebCamState() const {
    ui->webCamCheckBox->setChecked(operate.getWebCamState());
}

void MainWindow::updateFnSuperSwapState() {
    ui->fnSuperSwapCheckBox->setChecked(operate.getFnSuperSwapState());
}

void MainWindow::updateCoolerBoostState() const {
    ui->coolerBoostCheckBox->setChecked(operate.getCoolerBoostState());
    coolerBoostAction->setChecked(operate.getCoolerBoostState());
}

void MainWindow::updateUserMode() {
    if (operate.updateEcData()) {
        switch (operate.getUserMode()) {
            case user_mode::balanced_mode:
                ui->balancedModeRadioButton->click();
                balancedMode->setChecked(true);
                break;
            case user_mode::performance_mode:
                ui->highPerformanceModeRadioButton->click();
                highPerformanceMode->setChecked(true);
                break;
            case user_mode::silent_mode:
                ui->silentModeRadioButton->click();
                silentMode->setChecked(true);
                break;
            case user_mode::super_battery_mode:
                ui->superBatteryModeRadioButton->click();
                superBatteryMode->setChecked(true);
                break;
            case user_mode::unknown_mode:
                //fall to default, happens on some models after booting
            default:
                if (!operate.isMsiEcLoaded()){
                    ui->modeFormWidget->setDisabled(true);
                    if (modeTrayMenu)
                        modeTrayMenu->setDisabled(true);
                } else {
                    ui->superBatteryModeRadioButton->setChecked(false);
                    ui->silentModeRadioButton->setChecked(false);
                    ui->balancedModeRadioButton->setChecked(false);
                    ui->highPerformanceModeRadioButton->setChecked(false);
                }
                break;
        }
    }
}

void MainWindow::updateFanMode() {
    QString fanMode;
    switch (operate.getFanMode()) {
        case fan_mode::auto_fan_mode:
            fanMode = tr("Auto");
            break;
        case fan_mode::silent_fan_mode:
            fanMode = tr("Silent");
            break;
        case fan_mode::basic_fan_mode:
            fanMode = tr("Basic");
            break;
        case fan_mode::advanced_fan_mode:
            fanMode = tr("Advanced");
            break;
        default:
            fanMode = tr("Unknown");
            break;
    }
    ui->fanModeValueLabel->setText(fanMode);
}

void MainWindow::updateFanSpeedSettings() {
    ui->advancedFanControlCheckBox->setChecked(operate.getFanMode() == fan_mode::advanced_fan_mode);

    QVector fan1SpeedSettings = operate.getFan1SpeedSettings();
    QVector fan1TempSettings = operate.getFan1TempSettings();
    QVector fan2SpeedSettings = operate.getFan2SpeedSettings();
    QVector fan2TempSettings = operate.getFan2TempSettings();

    ui->fan1Speed1Slider->setValue(fan1SpeedSettings[0]);
    ui->fan1Speed2Slider->setValue(fan1SpeedSettings[1]);
    ui->fan1Speed3Slider->setValue(fan1SpeedSettings[2]);
    ui->fan1Speed4Slider->setValue(fan1SpeedSettings[3]);
    ui->fan1Speed5Slider->setValue(fan1SpeedSettings[4]);
    ui->fan1Speed6Slider->setValue(fan1SpeedSettings[5]);
    ui->fan1Speed7Slider->setValue(fan1SpeedSettings[6]);

    ui->fan1Speed1TempLabel->setText(QString("< %1 °C").arg(fan1TempSettings[0]));
    ui->fan1Speed2TempSpinBox->setValue(fan1TempSettings[0]);
    ui->fan1Speed3TempSpinBox->setValue(fan1TempSettings[1]);
    ui->fan1Speed4TempSpinBox->setValue(fan1TempSettings[2]);
    ui->fan1Speed5TempSpinBox->setValue(fan1TempSettings[3]);
    ui->fan1Speed6TempSpinBox->setValue(fan1TempSettings[4]);
    ui->fan1Speed7TempSpinBox->setValue(fan1TempSettings[5]);

    ui->fan2Speed1Slider->setValue(fan2SpeedSettings[0]);
    ui->fan2Speed2Slider->setValue(fan2SpeedSettings[1]);
    ui->fan2Speed3Slider->setValue(fan2SpeedSettings[2]);
    ui->fan2Speed4Slider->setValue(fan2SpeedSettings[3]);
    ui->fan2Speed5Slider->setValue(fan2SpeedSettings[4]);
    ui->fan2Speed6Slider->setValue(fan2SpeedSettings[5]);
    ui->fan2Speed7Slider->setValue(fan2SpeedSettings[6]);

    ui->fan2Speed1TempLabel->setText(QString("< %1 °C").arg(fan2TempSettings[0]));
    ui->fan2Speed2TempSpinBox->setValue(fan2TempSettings[0]);
    ui->fan2Speed3TempSpinBox->setValue(fan2TempSettings[1]);
    ui->fan2Speed4TempSpinBox->setValue(fan2TempSettings[2]);
    ui->fan2Speed5TempSpinBox->setValue(fan2TempSettings[3]);
    ui->fan2Speed6TempSpinBox->setValue(fan2TempSettings[4]);
    ui->fan2Speed7TempSpinBox->setValue(fan2TempSettings[5]);

    checkFanSettingsChanged();
}

void MainWindow::setBestMobility() {
    operate.setBatteryThreshold(100);
    updateBatteryThreshold();
}

void MainWindow::setBalancedBattery() {
    operate.setBatteryThreshold(80);
    updateBatteryThreshold();
}

void MainWindow::setBestBattery() {
    operate.setBatteryThreshold(60);
    updateBatteryThreshold();
}

void MainWindow::setHighPerformanceMode() {
    operate.setUserMode(user_mode::performance_mode);
    updateUserMode();
    pushCurrentModeToPPD();
}

void MainWindow::setBalancedMode() {
    operate.setUserMode(user_mode::balanced_mode);
    updateUserMode();
    pushCurrentModeToPPD();
}

void MainWindow::setSilentMode() {
    operate.setUserMode(user_mode::silent_mode);
    updateUserMode();
    pushCurrentModeToPPD();
}

void MainWindow::setSuperBatteryMode() {
    operate.setUserMode(user_mode::super_battery_mode);
    updateUserMode();
    pushCurrentModeToPPD();
}

void MainWindow::setCoolerBoostState(bool enabled) const {
    operate.setCoolerBoostState(enabled);
    if (operate.updateEcData())
        updateCoolerBoostState();
}

QVector<int> MainWindow::getFan1SpeedValues() const {
    QVector<int> fan1SpeedSettings;

    fan1SpeedSettings.push_back(ui->fan1Speed1Slider->value());
    fan1SpeedSettings.push_back(ui->fan1Speed2Slider->value());
    fan1SpeedSettings.push_back(ui->fan1Speed3Slider->value());
    fan1SpeedSettings.push_back(ui->fan1Speed4Slider->value());
    fan1SpeedSettings.push_back(ui->fan1Speed5Slider->value());
    fan1SpeedSettings.push_back(ui->fan1Speed6Slider->value());
    fan1SpeedSettings.push_back(ui->fan1Speed7Slider->value());

    return fan1SpeedSettings;
}

QVector<int> MainWindow::getFan2SpeedValues() const {
    QVector<int> fan2SpeedSettings;

    fan2SpeedSettings.push_back(ui->fan2Speed1Slider->value());
    fan2SpeedSettings.push_back(ui->fan2Speed2Slider->value());
    fan2SpeedSettings.push_back(ui->fan2Speed3Slider->value());
    fan2SpeedSettings.push_back(ui->fan2Speed4Slider->value());
    fan2SpeedSettings.push_back(ui->fan2Speed5Slider->value());
    fan2SpeedSettings.push_back(ui->fan2Speed6Slider->value());
    fan2SpeedSettings.push_back(ui->fan2Speed7Slider->value());

    return fan2SpeedSettings;
}

QVector<int> MainWindow::getFan1TempValues() const {
    QVector<int> fan1TempSettings;

    fan1TempSettings.push_back(ui->fan1Speed2TempSpinBox->value());
    fan1TempSettings.push_back(ui->fan1Speed3TempSpinBox->value());
    fan1TempSettings.push_back(ui->fan1Speed4TempSpinBox->value());
    fan1TempSettings.push_back(ui->fan1Speed5TempSpinBox->value());
    fan1TempSettings.push_back(ui->fan1Speed6TempSpinBox->value());
    fan1TempSettings.push_back(ui->fan1Speed7TempSpinBox->value());

    return fan1TempSettings;
}

QVector<int> MainWindow::getFan2TempValues() const {
    QVector<int> fan2TempSettings;

    fan2TempSettings.push_back(ui->fan2Speed2TempSpinBox->value());
    fan2TempSettings.push_back(ui->fan2Speed3TempSpinBox->value());
    fan2TempSettings.push_back(ui->fan2Speed4TempSpinBox->value());
    fan2TempSettings.push_back(ui->fan2Speed5TempSpinBox->value());
    fan2TempSettings.push_back(ui->fan2Speed6TempSpinBox->value());
    fan2TempSettings.push_back(ui->fan2Speed7TempSpinBox->value());

    return fan2TempSettings;
}

void MainWindow::setFanSpeedSettings() {
    operate.setFan1SpeedSettings(getFan1SpeedValues());
    operate.setFan1TempSettings(getFan1TempValues());
    operate.setFan2SpeedSettings(getFan2SpeedValues());
    operate.setFan2TempSettings(getFan2TempValues());
    if (operate.updateEcData())
        checkFanSettingsChanged();
}

void MainWindow::setFanModeAdvanced(bool enabled) const {
    operate.setFanModeAdvanced(enabled);
    ui->fanControlTabWidget->setEnabled(enabled);
    ui->fanSpeedResetButton->setEnabled(enabled);
    ui->fanSpeedApplyButton->setEnabled(enabled);
}

void MainWindow::checkFanSettingsChanged() const {
    bool fanSettingChanged = (getFan1SpeedValues() != operate.getFan1SpeedSettings() ||
                              getFan2SpeedValues() != operate.getFan2SpeedSettings() ||
                              getFan1TempValues() != operate.getFan1TempSettings() ||
                              getFan2TempValues() != operate.getFan2TempSettings());
    ui->fanSpeedApplyButton->setEnabled(fanSettingChanged);
    ui->fanSpeedResetButton->setEnabled(fanSettingChanged);
}

void MainWindow::showEvent(QShowEvent *event) {
    startRealtimeUpdate();
    if (sensors) QMetaObject::invokeMethod(sensors, "setFast", Qt::QueuedConnection, Q_ARG(bool, true));
    activateWindow();
}

void MainWindow::closeEvent(QCloseEvent *event) {
    if (trayIcon && trayIcon->isVisible()) {
        stopRealtimeUpdate();
        if (sensors) QMetaObject::invokeMethod(sensors, "setFast", Qt::QueuedConnection, Q_ARG(bool, false));
        MainWindow::hide();
        event->ignore();
        return;
    }
    quitApp();
}

void MainWindow::quitApp() const {
    operate.closeHelperApp();
    Settings::setValue("MainWindow/Width", MainWindow::width());
    Settings::setValue("MainWindow/Height", MainWindow::height());
    (void) QCoreApplication::quit();
}

void MainWindow::timerSleepTimeout() {
    qint64 timeNow = QDateTime::currentMSecsSinceEpoch();
    if (timeLastWatcherInterval == 0) {
        timeLastWatcherInterval = timeNow;
        return;
    }
    qint64 msecsSinceTimeout = timeNow - timeLastWatcherInterval;
    timeLastWatcherInterval = timeNow;
    if (msecsSinceTimeout > timerSleepWatcher.interval() + 5000) {
        // Went to sleep for at least 5 seconds
        operate.handleWakeEvent();
    }
}

void MainWindow::setModeFromSelection(PowerProfile profile) {
    switch (profile) {
    case PowerProfile::Performance:
        setHighPerformanceMode();
        break;
    case PowerProfile::Balanced:
        setBalancedMode();
        break;
    case PowerProfile::Silent:
        setSilentMode();
        break;
    case PowerProfile::PowerSaver:
        setSuperBatteryMode();
        break;
    case PowerProfile::Unknown:
    default:;
    }
}

void MainWindow::on_ChargerStateChange(bool isCharging) {
    if (ui->autoAcDcProfilesGroupBox->isChecked()) {
        Settings s;
        int SelectedModeOnBattery = s.getValue("Settings/UserModeOnBattery").toInt();
        int SelectedModeOnCharger = s.getValue("Settings/UserModeOnCharger").toInt();

        PowerProfile batteryProfile = static_cast<PowerProfile>(SelectedModeOnBattery);
        PowerProfile chargerProfile = static_cast<PowerProfile>(SelectedModeOnCharger);

        if (isCharging) {
            setModeFromSelection(chargerProfile);
        } else {
            setModeFromSelection(batteryProfile);
        }
    } else {
        ui->autoPPDCheckBox->setEnabled(1);
    }
}

PowerProfile MainWindow::ppdProfileForCurrentMode() const {
    // Silent has no PPD equivalent (same CPU shift as balanced, quieter fans)
    const QString mode = Settings().getValue("Settings/UserMode").toString();
    if (mode == "performance_mode")
        return PowerProfile::Performance;
    if (mode == "balanced_mode" || mode == "silent_mode")
        return PowerProfile::Balanced;
    if (mode == "super_battery_mode")
        return PowerProfile::PowerSaver;
    return PowerProfile::Unknown;
}

void MainWindow::pushCurrentModeToPPD() {
    if (ui->autoPPDCheckBox->isChecked())
        powerMonitor.setPowerProfile(ppdProfileForCurrentMode());
}

void MainWindow::on_PowerProfileChange(const PowerProfile profile) {
    // Ignore echoes of our own push, and keep Silent when PPD reports balanced
    if (profile == ppdProfileForCurrentMode())
        return;
    if (ui->autoPPDCheckBox->isChecked()) {
        switch (profile) {
        case PowerProfile::Performance:
            setHighPerformanceMode();
            ui->highPerformanceModeRadioButton->setChecked(true);
            break;
        case PowerProfile::Balanced:
            setBalancedMode();
            ui->balancedModeRadioButton->setChecked(true);
            break;
        case PowerProfile::PowerSaver:
            setSuperBatteryMode();
            ui->superBatteryModeRadioButton->setChecked(true);
            break;
        case PowerProfile::Unknown:
            default:;
        }
    }
}

void MainWindow::on_bestMobilityRadioButton_toggled(bool checked) {
    if (checked)
        setBestMobility();
}

void MainWindow::on_balancedBatteryRadioButton_toggled(bool checked) {
    if (checked)
        setBalancedBattery();
}

void MainWindow::on_bestBatteryRadioButton_toggled(bool checked) {
    if (checked)
        setBestBattery();
}

void MainWindow::on_customBatteryThresholdRadioButton_toggled(bool checked) {
    if (checked) {
        ui->customBatteryThresholdSpinBox->setEnabled(true);
    } else {
        ui->customBatteryThresholdSpinBox->setEnabled(false);
        ui->customBatteryApplyButton->setEnabled(false);
    }
}

void MainWindow::on_customBatteryThresholdSpinBox_valueChanged(int arg1) {
    ui->customBatteryApplyButton->setEnabled(
            ui->customBatteryThresholdRadioButton->isChecked() && operate.getBatteryThreshold() != arg1);
}

void MainWindow::on_customBatteryApplyButton_clicked() {
    operate.setBatteryThreshold(ui->customBatteryThresholdSpinBox->value());
    updateBatteryThreshold();
}

void MainWindow::on_ReadValueButton_clicked() {
    QString text = ui->addressEdit->displayText();
    int value = operate.getValue(text.toInt());
    ui->ValueSpinBox->setValue(value);
}

void MainWindow::on_WriteValueButton_clicked() const {
    QString text = ui->addressEdit->displayText();
    int address = text.toInt();
    operate.setValue(address, ui->ValueSpinBox->value());
}

void MainWindow::on_usbPowerShareCheckBox_clicked(bool checked) const {
    operate.setUsbPowerShareState(checked);
}

void MainWindow::on_webCamCheckBox_clicked(bool checked) const {
    operate.setWebCamState(checked);
    if (operate.updateEcData()) {
        updateWebCamState();
    }
}

void MainWindow::on_fnSuperSwapCheckBox_clicked(bool checked) const {
    operate.setFnSuperSwapState(checked);
}

void MainWindow::on_coolerBoostCheckBox_clicked(bool checked) const {
    if (operate.getCoolerBoostState() != checked)
        setCoolerBoostState(checked);
}

void MainWindow::on_keyboardBrightnessSlider_valueChanged(int value) const {
    operate.setKeyboardBrightness(value);
    if (operate.updateEcData()) {
        updateKeyboardBrightness();
    }
}

void MainWindow::on_keyboardBacklightModeComboBox_currentIndexChanged(int index) const {
    operate.setKeyboardBacklightMode(index);
}

void MainWindow::on_userModeOnBatteryComboBox_currentIndexChanged(int index) const {
    Settings::setValue("Settings/UserModeOnBattery", index);
    powerMonitor.queryChargerState();
}

void MainWindow::on_userModeOnChargerComboBox_currentIndexChanged(int index) const {
    Settings::setValue("Settings/UserModeOnCharger", index);
    powerMonitor.queryChargerState();
}

void MainWindow::on_autoAcDcProfilesGroupBox_toggled(bool checked) {
    if(checked) {
        if (!powerMonitor.connectToUpower()) {
            QMessageBox::critical(nullptr, this->windowTitle(), tr("Couldn't connect to UPower to get charger status.\n"
                                                                   "Make sure that UPower is installed and running then restart the system."));
            ui->autoAcDcProfilesGroupBox->setChecked(0);
            ui->autoAcDcProfilesGroupBox->setEnabled(0);
            return;
        }

        powerMonitor.disconnectFromPowerProfiles();
        ui->autoPPDCheckBox->setChecked(0);
        ui->autoPPDCheckBox->setEnabled(0);
        powerMonitor.queryChargerState();
    } else {
        ui->autoPPDCheckBox->setEnabled(1);
        powerMonitor.disconnectFromUpower();
    }

    Settings::setValue("Settings/autoAcDcProfilesState", checked);
}

void MainWindow::on_autoPPDCheckBox_toggled(bool checked) {
    if (checked) {

        if (!powerMonitor.connectToPowerProfiles()) {
            QMessageBox::critical(nullptr, this->windowTitle(), tr("Couldn't connect to Power Profiles Daemon.\n"
                                                                   "Make sure that either Power Profiles Daemon or TuneD is installed and restart the system."));
            ui->autoPPDCheckBox->setChecked(0);
            return;
        }

        powerMonitor.disconnectFromUpower();
        ui->autoAcDcProfilesGroupBox->setChecked(0);
        ui->autoAcDcProfilesGroupBox->setEnabled(0);
        Settings::setValue("Settings/autoPPDstate", checked);
        pushCurrentModeToPPD();
    } else {
        ui->highPerformanceModeRadioButton->setEnabled(1);
        ui->balancedModeRadioButton->setEnabled(1);
        ui->silentModeRadioButton->setEnabled(1);
        ui->superBatteryModeRadioButton->setEnabled(1);
        ui->autoAcDcProfilesGroupBox->setEnabled(1);
    }
    Settings::setValue("Settings/autoPPDstate", checked);
}

void MainWindow::on_highPerformanceModeRadioButton_toggled(bool checked) {
    if (checked)
        setHighPerformanceMode();
}

void MainWindow::on_balancedModeRadioButton_toggled(bool checked) {
    if (checked)
        setBalancedMode();
}

void MainWindow::on_silentModeRadioButton_toggled(bool checked) {
    if (checked)
        setSilentMode();
}

void MainWindow::on_superBatteryModeRadioButton_toggled(bool checked) {
    if (checked)
        setSuperBatteryMode();
}

void MainWindow::iconActivated(QSystemTrayIcon::ActivationReason reason) {
    switch (reason) {
        case QSystemTrayIcon::Trigger:
        case QSystemTrayIcon::DoubleClick:
            if (!MainWindow::isHidden()) {
                MainWindow::close();
            } else {
                MainWindow::showNormal();
            }
            break;
        case QSystemTrayIcon::MiddleClick:
            break;
        default:;
    }
}

void MainWindow::createTrayIcon() {
    createActions();

    modeTrayMenu = new QMenu(tr("Mode"));
    modeTrayActions = new QActionGroup(this);
    modeTrayActions->setExclusive(true);


    modeTrayActions->addAction(highPerformanceMode);
    modeTrayActions->addAction(balancedMode);
    modeTrayActions->addAction(silentMode);
    modeTrayActions->addAction(superBatteryMode);

    modeTrayMenu->addAction(highPerformanceMode);
    modeTrayMenu->addAction(balancedMode);
    modeTrayMenu->addAction(silentMode);
    modeTrayMenu->addAction(superBatteryMode);

    fanTrayMenu = new QMenu(tr("Cooling"));
    fanTrayMenu->addAction(coolerBoostAction);

    batteryTrayMenu = new QMenu(tr("Charge limit"));
    batteryTrayMenu->addAction(bestMobilityAction);
    batteryTrayMenu->addAction(balancedBatteryAction);
    batteryTrayMenu->addAction(bestBatteryAction);

    trayIconMenu = new QMenu(this);
    trayIconMenu->addAction(restoreAction);
    trayIconMenu->addSeparator();
    trayIconMenu->addMenu(modeTrayMenu);
    trayIconMenu->addMenu(fanTrayMenu);
    trayIconMenu->addMenu(batteryTrayMenu);
    trayIconMenu->addSeparator();
    trayIconMenu->addAction(quitAction);

    trayIcon = new QSystemTrayIcon(this);
    trayIcon->setContextMenu(trayIconMenu);
    auto icon = QIcon(":/images/AppIcon");
    trayIcon->setIcon(icon);
    trayIcon->setToolTip("MControlCenter");

    trayIcon->show();

    connect(trayIcon, &QSystemTrayIcon::activated, this, &MainWindow::iconActivated);
}

void MainWindow::createActions() {
    restoreAction = new QAction(tr("Show"), this);
    connect(restoreAction, &QAction::triggered, this, &MainWindow::showNormal);

    highPerformanceMode = new QAction(ui->highPerformanceModeRadioButton->text(), this);
    highPerformanceMode->setCheckable(true);

    balancedMode = new QAction(ui->balancedModeRadioButton->text(), this);
    balancedMode->setCheckable(true);

    silentMode = new QAction(ui->silentModeRadioButton->text(), this);
    silentMode->setCheckable(true);

    superBatteryMode = new QAction(ui->superBatteryModeRadioButton->text(), this);
    superBatteryMode->setCheckable(true);

    connect(highPerformanceMode, &QAction::triggered, this, &MainWindow::setHighPerformanceMode);
    connect(balancedMode, &QAction::triggered, this, &MainWindow::setBalancedMode);
    connect(silentMode, &QAction::triggered, this, &MainWindow::setSilentMode);
    connect(superBatteryMode, &QAction::triggered, this, &MainWindow::setSuperBatteryMode);

    coolerBoostAction = new QAction(tr("Cooler Boost"), this);
    coolerBoostAction->setCheckable(true);

    connect(coolerBoostAction, &QAction::triggered, this, &MainWindow::setCoolerBoostState);

    bestMobilityAction = new QAction(ui->bestMobilityRadioButton->text() + " (100%)", this);
    balancedBatteryAction = new QAction(ui->balancedBatteryRadioButton->text() + " (80%)", this);
    bestBatteryAction = new QAction(ui->bestBatteryRadioButton->text() + " (60%)", this);

    connect(bestMobilityAction, &QAction::triggered, this, &MainWindow::setBestMobility);
    connect(balancedBatteryAction, &QAction::triggered, this, &MainWindow::setBalancedBattery);
    connect(bestBatteryAction, &QAction::triggered, this, &MainWindow::setBestBattery);

    quitAction = new QAction(tr("Quit"), this);
    connect(quitAction, &QAction::triggered, this, &MainWindow::quitApp);
}

void MainWindow::saveStateRequest(QSessionManager &sessionManager) {
    sessionManager.setRestartHint(QSessionManager::RestartNever);
}

static int sysInt(const char *path, int def = -1) {
    QFile f(path); if (!f.open(QIODevice::ReadOnly)) return def;
    bool ok; int v = QString::fromUtf8(f.readAll()).trimmed().toInt(&ok); return ok ? v : def;
}

void MainWindow::pushDashboard() {
    // RPM comes from the cached ec_sys dump (no I/O). Everything else arrives from the sensor thread.
    dashData.cpuFanRpm = operate.getFan1Speed();
    std::optional<int> f2 = operate.getFan2Speed();
    dashData.gpuFanRpm = f2.has_value() ? f2.value() : 0;
    dashboard->setRpm(dashData.cpuFanRpm, dashData.gpuFanRpm);
    if (!msiEcPresent()) {                                          // legacy ec_sys-only laptops: read through the old backend
        DashData d = dashData;
        d.cpuTemp = operate.getCpuTemp(); std::optional<int> g = operate.getGpuTemp(); d.gpuTemp = g.has_value() ? g.value() : -1;
        switch (operate.getUserMode()) { case user_mode::performance_mode: d.mode = 0; break; case user_mode::balanced_mode: d.mode = 1; break; case user_mode::silent_mode: d.mode = 2; break; case user_mode::super_battery_mode: d.mode = 3; break; default: d.mode = -1; }
        d.coolerBoost = operate.getCoolerBoostState(); int thr = operate.getBatteryThreshold(); d.chargeLimit = (thr <= 0 || thr > 100) ? 100 : thr;
        dashData = d; dashboard->setData(d);
    }
    if (trayIcon) {
        static const char *names[] = {"Turbo", "Balanced", "Silent", "Super Battery"}; const DashData &d = dashData;
        trayIcon->setToolTip(QString("MControlCenter\n%1  ·  CPU %2 °C%3\nFan %4 %  ·  charge limit %5 %")
            .arg(d.mode >= 0 ? names[d.mode] : "?").arg(d.cpuTemp).arg(d.gpuTemp > 0 ? QString("  ·  GPU %1 °C").arg(d.gpuTemp) : QString()).arg(d.cpuFanPct).arg(d.chargeLimit));
    }
}

void MainWindow::setModeIndex(int mode) {
    switch (mode) {
        case 0: setHighPerformanceMode(); break;
        case 1: setBalancedMode(); break;
        case 2: setSilentMode(); break;
        case 3: setSuperBatteryMode(); break;
    }
}

void MainWindow::setChargeLimit(int percent) {
    if (percent >= 100) setBestMobility();
    else if (percent == 80) setBalancedBattery();
    else if (percent == 60) setBestBattery();
    else { operate.setBatteryThreshold(percent); updateBatteryThreshold(); }
}

QString MainWindow::stateJson() const {
    static const char *modes[] = {"turbo", "balanced", "silent", "eco"};
    QJsonObject o; const DashData &d = dashData;
    o["model"] = slurp("/sys/class/dmi/id/product_name"); o["mode"] = d.mode >= 0 ? modes[d.mode] : "unknown"; o["cpuTemp"] = d.cpuTemp; o["gpuTemp"] = d.gpuTemp;
    o["cpuFanPct"] = d.cpuFanPct; o["gpuFanPct"] = d.gpuFanPct; o["cpuFanRpm"] = d.cpuFanRpm; o["gpuFanRpm"] = d.gpuFanRpm;
    o["coolerBoost"] = d.coolerBoost; o["chargeLimit"] = d.chargeLimit;
    return QString::fromUtf8(QJsonDocument(o).toJson(QJsonDocument::Compact));
}


// What can THIS laptop do? Probed once; the dashboard hides everything the hardware does not have.
void MainWindow::probeCaps() {
    Caps c;
    c.model = slurp("/sys/class/dmi/id/product_name"); c.board = slurp("/sys/class/dmi/id/board_name");
    c.fw = slurp("/sys/devices/platform/msi-ec/fw_version"); if (c.fw.isEmpty()) c.fw = QString::fromStdString(operate.getEcVersion());
    QString cpu; for (const QString &l : slurp("/proc/cpuinfo").split('\n')) if (l.startsWith("model name")) { cpu = l.section(':', 1).trimmed(); break; }
    cpu.remove(QRegularExpression("\\((R|TM|tm|r)\\)")).remove(QRegularExpression("\\s+(CPU|Processor)\\b.*$")).remove(QRegularExpression("\\s*@.*$")); c.cpuName = cpu.simplified();
    std::optional<int> g = operate.getGpuTemp(); c.gpuTemp = g.has_value();
    std::optional<int> f2 = operate.getFan2Speed(); c.fan2 = f2.has_value();
    c.chargeLimit = operate.isBatteryThresholdSupport();
    if (operate.isMsiEcLoaded()) {                                       // msi-ec publishes exactly which modes this EC knows
        c.coolerBoost = QFile::exists("/sys/devices/platform/msi-ec/cooler_boost");
        QString sm = slurp("/sys/devices/platform/msi-ec/available_shift_modes"), fm = slurp("/sys/devices/platform/msi-ec/available_fan_modes");
        if (!sm.isEmpty()) { c.modeOk[0] = sm.contains("turbo"); c.modeOk[1] = sm.contains("comfort"); c.modeOk[3] = sm.contains("eco"); c.modeOk[2] = sm.contains("comfort") && (fm.isEmpty() || fm.contains("silent")); }
    }
    if (c.gpuTemp) {                                                      // GPU marketing name from lspci (discrete GPUs only)
        QProcess lp; lp.start("lspci", {"-mm"}); lp.waitForFinished(2000);
        for (const QString &l : QString::fromUtf8(lp.readAllStandardOutput()).split('\n'))
            if ((l.contains("VGA compatible") || l.contains("3D controller")) && !l.contains("Intel")) {
                QRegularExpression re("\"([^\"]*)\"\\s*\"([^\"]*)\"\\s*\"([^\"]*)\"\\s*\"([^\"]*)\""); auto m = re.match(l);
                if (m.hasMatch()) { QString dev = m.captured(4); int a = dev.indexOf('['), b = dev.indexOf(']'); c.gpuName = (a >= 0 && b > a ? dev.mid(a + 1, b - a - 1) : dev).replace("GeForce ", "").replace("Laptop GPU", "").simplified(); break; }
            }
    }
    c.webcam = msiEcPresent() ? QFile::exists("/sys/devices/platform/msi-ec/webcam") : operate.isWebCamOffSupport();
    c.fnSwap = msiEcPresent() ? QFile::exists("/sys/devices/platform/msi-ec/fn_key") : true;
    c.usbShare = QFile::exists("/sys/devices/platform/msi-ec/usb_share") || operate.isUsbPowerShareSupport();
    dashboard->setCaps(c); controlsPage->setCaps(c); fanPage->setCaps(c);
    aboutPage->setInfo(MControlCenter_VERSION, QT_VERSION_STR, QString("%1 · %2").arg(c.fw, QString::fromStdString(operate.getEcBuild())),
                       msiEcPresent() ? QString("msi-ec %1").arg(slurp("/sys/module/msi_ec/version").isEmpty() ? QString("(DKMS)") : slurp("/sys/module/msi_ec/version")) : QString("not loaded (ec_sys fallback)"),
                       QSysInfo::kernelVersion(), QString("%1 · %2").arg(qEnvironmentVariable("XDG_CURRENT_DESKTOP", "unknown"), qEnvironmentVariable("XDG_SESSION_TYPE", "unknown")));
    syncFanEditor();
}

// Friendly replacement for the old "msi-ec not loaded" dialog: say what is wrong and offer the one-click fix.
void MainWindow::checkDriver() {
    if (operate.isMsiEcLoaded()) { dashboard->setProblem(QString(), QString()); return; }
    const QString doctor = "/usr/libexec/mcontrolcenter/mcontrolcenter-doctor";
    QString state, fw;
    if (QFile::exists(doctor)) {
        QProcess pr; pr.start(doctor, {"status"}); pr.waitForFinished(5000);
        for (const QString &l : QString::fromUtf8(pr.readAllStandardOutput()).split('\n')) { if (l.startsWith("state=")) state = l.mid(6); else if (l.startsWith("fw=")) fw = l.mid(3); }
    }
    if (state == "not-msi") dashboard->setProblem(tr("This does not look like an MSI laptop. Running in read-only fallback mode."), QString());
    else if (state == "unsupported-firmware") dashboard->setProblem(tr("The msi-ec driver does not know this EC firmware (%1) yet. Basic monitoring works; please report the firmware to the msi-ec project.").arg(fw), QString());
    else if (state == "secureboot") dashboard->setProblem(tr("Secure Boot rejected the msi-ec module. Enroll the DKMS signing key (mokutil --import /var/lib/dkms/mok.pub) and reboot."), QString());
    else if (!state.isEmpty()) dashboard->setProblem(tr("The msi-ec driver is not built for this kernel (%1). This usually happens right after a kernel update.").arg(state), tr("Fix it now"));
    else dashboard->setProblem(tr("The msi-ec driver is not loaded. Reinstall MControlCenter with install.sh to set up the driver and automatic repair after kernel updates."), QString());
}

void MainWindow::runDoctorFix() {
    dashboard->setProblem(tr("Repairing the driver: installing kernel headers and rebuilding msi-ec. This can take a minute..."), QString());
    auto *pr = new QProcess(this);
    connect(pr, &QProcess::finished, this, [this, pr](int code, QProcess::ExitStatus) {
        QString err = QString::fromUtf8(pr->readAllStandardError()).trimmed().split('\n').last();
        pr->deleteLater();
        if (code == 0) { QProcess::startDetached("/bin/sh", {"-c", "sleep 1; exec mcontrolcenter"}); quitApp(); }
        else dashboard->setProblem(tr("Repair did not finish: %1").arg(err.isEmpty() ? tr("authentication cancelled or failed") : err), tr("Try again"));
    });
    pr->start("pkexec", {"/usr/libexec/mcontrolcenter/mcontrolcenter-doctor", "fix"});
}

// ───────────────────────────── pages: wiring between the new UI and the existing backend slots ─────────────────────────────
void MainWindow::buildPages() {
    controlsPage = new ControlsPage(this); batteryPage = new BatteryPage(this); fanPage = new FanPage(this); aboutPage = new AboutPage(this);
    auto swap = [this](QWidget *old, QWidget *neu, const QString &title) {      // the old widgets stay alive (hidden) as the backend the slots already know
        int i = ui->tabWidget->indexOf(old); if (i < 0) return; ui->tabWidget->removeTab(i); ui->tabWidget->insertTab(i, neu, title); };
    settingsPage = new SettingsPage(this);
    swap(ui->overviewTab, controlsPage, tr("Controls")); swap(ui->batteryTab, batteryPage, tr("Battery")); swap(ui->fanControlTab, fanPage, tr("Fans")); swap(ui->aboutTab, aboutPage, tr("About"));
    ui->tabWidget->insertTab(ui->tabWidget->indexOf(aboutPage), settingsPage, tr("Settings"));
    ui->tabWidget->removeTab(ui->tabWidget->indexOf(ui->debugTab));
    settingsPage->onChange = [this](SettingsPage::Item w, int v) { settingChanged(w, v); };
    connect(&gameTimer, &QTimer::timeout, this, &MainWindow::gameModeTick); gameTimer.setInterval(5000);
    if (Settings().getValue("Settings/gameMode").toBool()) gameTimer.start();
    syncSettingsPage();
    ui->tabWidget->setCurrentIndex(0);

    controlsPage->onSwitch = [this](ControlsPage::Sw s) {
        QCheckBox *c = s == ControlsPage::Webcam ? ui->webCamCheckBox : s == ControlsPage::FnSwap ? ui->fnSuperSwapCheckBox : s == ControlsPage::UsbShare ? ui->usbPowerShareCheckBox : ui->coolerBoostCheckBox;
        if (c->isEnabled()) { c->click(); controlsPage->setSwitch(s, c->isChecked()); } };
    controlsPage->onFollowPpd = [this](bool on) { if (ui->autoPPDCheckBox->isEnabled()) { ui->autoPPDCheckBox->setChecked(on); syncPages(Snapshot()); } };
    controlsPage->onAutoAcDc = [this](bool on) { if (ui->autoAcDcProfilesGroupBox->isEnabled()) { ui->autoAcDcProfilesGroupBox->setChecked(on); syncPages(Snapshot()); } };
    controlsPage->onAcDcMode = [this](bool charger, int i) { (charger ? ui->userModeOnChargerComboBox : ui->userModeOnBatteryComboBox)->setCurrentIndex(i); syncPages(Snapshot()); };
    controlsPage->onGpuMode = [this](bool on) { setGpuBootMode(on); };
    controlsPage->onReboot = [] { QDBusInterface("org.freedesktop.login1", "/org/freedesktop/login1", "org.freedesktop.login1.Manager", QDBusConnection::systemBus()).call("Reboot", true); };
    batteryPage->onLimit = [this](int v) { setChargeLimit(v); };
    fanPage->onAdvanced = [this](bool on) { ui->advancedFanControlCheckBox->setChecked(on); fanPage->setAdvanced(on); };
    fanPage->onCurveEdited = [this](int fan, const QVector<int> &sp, const QVector<int> &tp) { writeFanToUi(fan, sp, tp); };
    fanPage->onApply = [this] { ui->fanSpeedApplyButton->click(); syncFanEditor(); };
    fanPage->onReset = [this] { ui->fanSpeedResetButton->click(); syncFanEditor(); };
    aboutPage->onLink = [this](int w) { aboutLink(w); };
}

void MainWindow::syncPages(const Snapshot &s) {
    if (!controlsPage) return;
    const bool real = s.hasWebcam || s.hasFn || s.msiEc || s.gpuKnown || s.cap > 0;       // a default-constructed Snapshot only means "re-read the widgets"
    if (real) {
        auto sync = [](QCheckBox *c, bool v) { QSignalBlocker b(c); c->setChecked(v); };       // keep the hidden legacy widgets equal to the hardware so click() flips the right way
        if (s.hasWebcam) { sync(ui->webCamCheckBox, s.webcamOn); controlsPage->setSwitch(ControlsPage::Webcam, s.webcamOn); }
        if (s.hasFn) { sync(ui->fnSuperSwapCheckBox, s.fnSwapped); controlsPage->setSwitch(ControlsPage::FnSwap, s.fnSwapped); }
        if (s.hasUsb) { sync(ui->usbPowerShareCheckBox, s.usbOn); controlsPage->setSwitch(ControlsPage::UsbShare, s.usbOn); }
        if (s.msiEc) { sync(ui->coolerBoostCheckBox, s.d.coolerBoost); controlsPage->setSwitch(ControlsPage::Boost, s.d.coolerBoost); }
        controlsPage->setGpu(s.gpuKnown, s.gpuSupported, s.gpuReason, s.gpuDriver, s.gpuPower, s.gpuMode, s.gpuActive, s.gpuSusp);
        auto pt = dashboard->powerText();
        batteryPage->setLimit(s.d.chargeLimit, ui->batteryTab->isEnabled() || msiEcPresent());
        batteryPage->setHealth(pt.state, pt.health, pt.cycles, pt.draw, pt.eta, s.cap);
        fanPage->setLive(s.d.cpuTemp, s.d.cpuFanPct, qMax(s.d.gpuTemp, 0), s.d.gpuFanPct);
    }
    const int m[4] = {0, 1, 2, 3};
    controlsPage->setProfileState(ui->autoPPDCheckBox->isChecked(), ui->autoAcDcProfilesGroupBox->isChecked(), m[qBound(0, ui->userModeOnChargerComboBox->currentIndex(), 3)], m[qBound(0, ui->userModeOnBatteryComboBox->currentIndex(), 3)]);
    fanPage->setAdvanced(ui->advancedFanControlCheckBox->isChecked());
    fanPage->setDirty(ui->fanSpeedApplyButton->isEnabled());
}

void MainWindow::syncFanEditor() {
    if (!fanPage) return;
    for (int fan = 1; fan <= 2; ++fan) {
        QVector<int> sp, tp;
        for (int i = 1; i <= 7; ++i) if (auto *sl = findChild<QSlider *>(QString("fan%1Speed%2Slider").arg(fan).arg(i))) sp << sl->value();
        for (int i = 2; i <= 7; ++i) if (auto *sb = findChild<QSpinBox *>(QString("fan%1Speed%2TempSpinBox").arg(fan).arg(i))) tp << sb->value();
        if (sp.size() == 7 && tp.size() == 6) fanPage->setCurve(fan - 1, sp, tp);
    }
}

void MainWindow::writeFanToUi(int fan, const QVector<int> &sp, const QVector<int> &tp) {
    for (int i = 0; i < sp.size(); ++i) if (auto *sl = findChild<QSlider *>(QString("fan%1Speed%2Slider").arg(fan + 1).arg(i + 1))) sl->setValue(sp[i]);
    for (int i = 0; i < tp.size(); ++i) if (auto *sb = findChild<QSpinBox *>(QString("fan%1Speed%2TempSpinBox").arg(fan + 1).arg(i + 2))) sb->setValue(tp[i]);
}

void MainWindow::setGpuBootMode(bool on) {
    const QString helper = "/usr/libexec/mcontrolcenter/mcontrolcenter-gpu";
    if (!QFile::exists(helper)) { controlsPage->setGpuBusy(tr("The GPU helper is not installed. Re-run install.sh.")); return; }
    controlsPage->setGpuBusy(tr("Applying: rebuilding the boot image. This can take a minute..."));
    auto *pr = new QProcess(this);
    connect(pr, &QProcess::finished, this, [this, pr](int code, QProcess::ExitStatus) {
        const QString err = QString::fromUtf8(pr->readAllStandardError()).trimmed().split('\n').last(); pr->deleteLater();
        controlsPage->setGpuBusy(code == 0 ? QString() : tr("Not changed: %1").arg(err.isEmpty() ? tr("authentication cancelled or failed") : err)); });
    pr->start("pkexec", {helper, "set", on ? "on" : "off"});
}

static QString hardwareReport() {
    QString doctor = "/usr/libexec/mcontrolcenter/mcontrolcenter-doctor"; if (!QFile::exists(doctor)) doctor = QCoreApplication::applicationDirPath() + "/../scripts/mcontrolcenter-doctor";
    QProcess pr; pr.start(doctor, {"report"}); pr.waitForFinished(8000); return QString::fromUtf8(pr.readAllStandardOutput());
}

void MainWindow::aboutLink(int w) {
    static const char *urls[2] = {"https://github.com/p34t1k/MControlCenter", "https://github.com/dmitry-s93/MControlCenter"};
    if (w < 2) { QDesktopServices::openUrl(QUrl(urls[w])); return; }
    const QString report = hardwareReport(); QGuiApplication::clipboard()->setText(report);
    if (w == 2) {                                                         // open a Hardware-report issue with the report already filled in (kept in the clipboard as a fallback)
        QString url = "https://github.com/p34t1k/MControlCenter/issues/new?template=hardware-report.yml"; const QByteArray enc = QUrl::toPercentEncoding(report);
        if (enc.size() < 6500) url += "&report=" + QString::fromLatin1(enc);
        QDesktopServices::openUrl(QUrl(url)); return;
    }
    if (trayIcon) trayIcon->showMessage("MControlCenter", tr("Hardware report copied to the clipboard. Paste it into a GitHub issue."), QSystemTrayIcon::Information, 4000);
}

// ───────────────────────────── settings, alerts, automation ─────────────────────────────
static QString userAutostartFile() { return QDir::homePath() + "/.config/autostart/mcontrolcenter.desktop"; }
static bool autostartEnabled() {
    QFile f(userAutostartFile()); if (f.open(QIODevice::ReadOnly)) return !QString::fromUtf8(f.readAll()).contains("Hidden=true");     // a user file overrides the system one
    return QFile::exists("/etc/xdg/autostart/mcontrolcenter.desktop");
}
static void setAutostart(bool on) {
    const QString f = userAutostartFile(); QDir().mkpath(QFileInfo(f).absolutePath());
    if (on && QFile::exists("/etc/xdg/autostart/mcontrolcenter.desktop")) { QFile::remove(f); return; }              // the system default already starts it
    QFile out(f); if (!out.open(QIODevice::WriteOnly | QIODevice::Truncate)) return;
    out.write(on ? "[Desktop Entry]\nType=Application\nName=MControlCenter\nExec=mcontrolcenter\nIcon=mcontrolcenter\n" : "[Desktop Entry]\nType=Application\nName=MControlCenter\nHidden=true\n");
}
static const int kThermalLevels[3] = {85, 90, 95};

void MainWindow::syncSettingsPage() {
    Settings s; settingsPage->setState(autostartEnabled(), s.getValue("Settings/thermalAlert").toBool(), qBound(0, s.getValue("Settings/thermalLevel").toInt(), 2),
                                       s.getValue("Settings/gameMode").toBool(), !QStandardPaths::findExecutable("gamemoded").isEmpty());
}

void MainWindow::settingChanged(SettingsPage::Item what, int value) {
    Settings s;
    switch (what) {
        case SettingsPage::Autostart: setAutostart(value != 0); break;
        case SettingsPage::ThermalAlert: Settings::setValue("Settings/thermalAlert", value != 0); hotSince = 0; break;
        case SettingsPage::ThermalLevel: Settings::setValue("Settings/thermalLevel", value); break;
        case SettingsPage::GameMode: { bool on = value != 0; Settings::setValue("Settings/gameMode", on); if (on) gameTimer.start(); else { gameTimer.stop(); if (gameActive) { gameActive = false; if (gameSavedMode >= 0) setModeIndex(gameSavedMode); setCoolerBoostState(gameSavedBoost); } } break; }
        case SettingsPage::ExportCsv: {
            const QString path = QDir::homePath() + "/mcontrolcenter-telemetry-" + QDateTime::currentDateTime().toString("yyyyMMdd-HHmmss") + ".csv"; QFile f(path);
            if (f.open(QIODevice::WriteOnly)) { f.write(dashboard->telemetryCsv().toUtf8()); if (trayIcon) trayIcon->showMessage("MControlCenter", tr("Telemetry saved to %1").arg(path), QSystemTrayIcon::Information, 5000); }
            break; }
    }
    syncSettingsPage();
}

// Alert once (then at most every 5 minutes) when the CPU has stayed above the chosen temperature for 10 s.
void MainWindow::thermalWatch(const Snapshot &sn) {
    Settings s; if (!s.getValue("Settings/thermalAlert").toBool() || !sn.msiEc) { hotSince = 0; return; }
    const int level = kThermalLevels[qBound(0, s.getValue("Settings/thermalLevel").toInt(), 2)]; const qint64 now = QDateTime::currentMSecsSinceEpoch();
    if (sn.d.cpuTemp < level) { hotSince = 0; return; }
    if (!hotSince) hotSince = now;
    if (now - hotSince >= 10000 && now - lastAlertMs > 300000 && trayIcon) { lastAlertMs = now; trayIcon->showMessage("MControlCenter", tr("CPU has been at %1 °C for 10 seconds. Cooler Boost or a quieter workload would help.").arg(sn.d.cpuTemp), QSystemTrayIcon::Warning, 8000); }
}

// Feral GameMode integration: Turbo + Cooler Boost while any game holds a GameMode request, then restore exactly what the user had.
void MainWindow::gameModeTick() {
    QDBusConnectionInterface *bus = QDBusConnection::sessionBus().interface(); int clients = 0;
    if (bus && bus->isServiceRegistered("com.feralinteractive.GameMode")) {
        QDBusInterface i("com.feralinteractive.GameMode", "/com/feralinteractive/GameMode", "org.freedesktop.DBus.Properties", QDBusConnection::sessionBus()); i.setTimeout(500);
        QDBusReply<QDBusVariant> r = i.call("Get", "com.feralinteractive.GameMode", "ClientCount"); if (r.isValid()) clients = r.value().variant().toInt();
    }
    if (clients > 0 && !gameActive) {
        gameActive = true; gameSavedMode = dashData.mode; gameSavedBoost = dashData.coolerBoost; setModeIndex(0); setCoolerBoostState(true);
        if (trayIcon) trayIcon->showMessage("MControlCenter", tr("Game detected: Turbo + Cooler Boost on."), QSystemTrayIcon::Information, 3000);
    } else if (clients == 0 && gameActive) {
        gameActive = false; if (gameSavedMode >= 0) setModeIndex(gameSavedMode); setCoolerBoostState(gameSavedBoost);
        if (trayIcon) trayIcon->showMessage("MControlCenter", tr("Game finished: back to your previous mode."), QSystemTrayIcon::Information, 3000);
    }
}
