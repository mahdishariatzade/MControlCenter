// The non-dashboard pages (Controls, Battery, Fans, About), built from the same design kit as the dashboard.
#ifndef PAGES_H
#define PAGES_H
#include <QWidget>
#include <QTimer>
#include <QElapsedTimer>
#include <QVector>
#include <functional>
#include <vector>
#include "dashboard.h"

namespace kit { class Panel; class RowsPanel; }
class ChargePanel; class CurvePanel; struct Snapshot;

// Base for the pages: one precise timer that runs only while something is animating and the page is visible.
class AnimPage : public QWidget {
public:
    explicit AnimPage(QWidget *p);
    void adopt(kit::Panel *pn, double delay);
protected:
    void showEvent(QShowEvent *) override; void hideEvent(QHideEvent *) override;
    virtual bool animateExtra(double, double) { return false; }
    void kick();
private:
    void tick();
    QTimer timer; QElapsedTimer clock; double last = 0; int idle = 0; std::vector<kit::Panel *> panels;
};

class ControlsPage : public AnimPage {
public:
    explicit ControlsPage(QWidget *p);
    enum Sw { Webcam, FnSwap, UsbShare, Boost };
    void setCaps(const Caps &c);
    void setSwitch(Sw s, bool on);
    void setProfileState(bool followPpd, bool autoAcDc, int onCharger, int onBattery);
    void setGpu(bool known, bool supported, const QString &reason, const QString &driver, const QString &power, const QString &mode, qint64 active, qint64 suspended);
    void setGpuBusy(const QString &msg);
    std::function<void(Sw)> onSwitch; std::function<void(bool)> onFollowPpd, onAutoAcDc; std::function<void(bool, int)> onAcDcMode;
    std::function<void(bool)> onGpuMode; std::function<void()> onReboot;
private:
    kit::RowsPanel *profile, *hardware, *gpu; Caps caps; bool gpuHaveInfo = false, gpuSupported = false, gpuModeOn = true, gpuRunningOff = false; QString gpuBusy; bool gpuPending = false;
    void rebuildGpuRows(const QString &reason, const QString &driver, const QString &power, qint64 a, qint64 s);
    QString gReason, gDriver, gPower; qint64 gA = 0, gS = 0;
};

class BatteryPage : public AnimPage {
public:
    explicit BatteryPage(QWidget *p);
    void setLimit(int percent, bool supported);
    void setHealth(const QString &status, const QString &health, const QString &cycles, const QString &draw, const QString &eta, int cap);
    std::function<void(int)> onLimit;
private:
    ChargePanel *charge; kit::RowsPanel *info;
};

class FanPage : public AnimPage {
public:
    explicit FanPage(QWidget *p);
    void setCaps(const Caps &c);
    void setCurve(int fan, const QVector<int> &speeds, const QVector<int> &temps);   // 7 speeds (%), 6 temperature steps (°C)
    void setLive(int cpuTemp, int cpuPct, int gpuTemp, int gpuPct);
    void setAdvanced(bool on); void setDirty(bool dirty);
    std::function<void(int, const QVector<int> &, const QVector<int> &)> onCurveEdited;   // fan, speeds, temps
    std::function<void(bool)> onAdvanced; std::function<void()> onApply, onReset;
private:
    kit::RowsPanel *head; CurvePanel *curve; int fan = 0; bool two = false, advanced = false;
};

class AboutPage : public AnimPage {
public:
    explicit AboutPage(QWidget *p);
    void setInfo(const QString &version, const QString &qt, const QString &ec, const QString &driver, const QString &kernel, const QString &desktop);
    std::function<void(int)> onLink;       // 0 source, 1 upstream, 2 issues, 3 copy hardware report
private:
    kit::RowsPanel *credits, *system, *links;
};
#endif
