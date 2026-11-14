// Dashboard page for MControlCenter: custom-painted live gauges, fans, battery ring, history graph and one-click presets.
// Written for the MSI Crosshair 16 HX AI (MS-15P4); everything degrades gracefully on other msi-ec laptops.
#ifndef DASHBOARD_H
#define DASHBOARD_H

#include <QWidget>
#include <QTimer>
#include <QVector>
#include <QString>
#include <QElapsedTimer>
#include <deque>

class ArcGauge;
class FanDial;
class BatteryRing;
class HistoryGraph;
class ModeTile;
class QLabel;
class QPushButton;

struct DashData {
    int cpuTemp = 0;
    int gpuTemp = -1;          // -1: no GPU sensor, 0: GPU is off
    int cpuFanPct = -1;        // -1: unknown
    int gpuFanPct = -1;
    int cpuFanRpm = 0;
    int gpuFanRpm = 0;
    int mode = -1;             // 0 performance, 1 balanced, 2 silent, 3 super battery
    bool coolerBoost = false;
    int chargeLimit = 100;
};

class DashboardPage : public QWidget {
Q_OBJECT
public:
    explicit DashboardPage(QWidget *parent = nullptr);
    void setData(const DashData &d);        // called by MainWindow once a second with EC values
    void setControlsEnabled(bool enabled);

signals:
    void modeRequested(int mode);
    void coolerBoostRequested(bool on);
    void chargeLimitRequested(int percent);

protected:
    void showEvent(QShowEvent *e) override;
    void hideEvent(QHideEvent *e) override;

private:
    void sample();                           // read sysfs (battery, cpu load/freq, ram, nvme, dGPU)
    void animate();
    void applyPreset(const QString &name);

    ArcGauge *cpuGauge, *gpuGauge;
    FanDial *cpuFan, *gpuFan;
    BatteryRing *battery;
    HistoryGraph *graph;
    ModeTile *tiles[4];
    ModeTile *boostBtn;
    ModeTile *limitBtns[3];
    QLabel *subtitle, *chipPower, *chipGpu;
    QLabel *statLoad, *statFreq, *statRam, *statNvme, *statHealth, *statCycles, *statDraw, *statEta;
    QTimer fastTimer, slowTimer;
    DashData cur;
    bool controlsEnabled = false;

    // cpu load bookkeeping
    quint64 lastTotal = 0, lastIdle = 0;
    // battery sysfs
    QString batPath;
};

#endif // DASHBOARD_H
