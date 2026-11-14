// Displays page: laptop panel refresh rate (with the "follow the power source" automation) and DDC/CI control of external monitors.
#ifndef DISPLAYS_H
#define DISPLAYS_H
#include "pages.h"
#include "ddc.h"
#include <QThread>
#include <QProcess>

// The laptop's own panel, via KDE's kscreen-doctor (asynchronous, never blocks the GUI).
class PanelRefresh : public QObject {
    Q_OBJECT
public:
    explicit PanelRefresh(QObject *p = nullptr) : QObject(p) {}
    QString output, res, backend; QList<int> hz; int current = -1; bool ok = false, available = false;   // backend: "kscreen" (KDE, X11+Wayland), "xrandr" (any X11 desktop) or ""
    void refresh();                         // re-read the modes
    void setHzIndex(int i);                 // pick one of `hz`
    void applyForPower(bool onCharger);     // highest rate on the charger, 60 Hz on battery
signals:
    void changed();
private:
    struct Mode { int id, w, h; double hz; QString hzText; };
    void detectBackend(); void parse(const QString &txt);
    QList<Mode> modes; int pendingPower = -1;
    void setMode(int id);
};

class DisplaysPage : public AnimPage {
public:
    explicit DisplaysPage(QWidget *p);
    ~DisplaysPage() override;
    void setLaptop(const QString &res, const QList<int> &hz, int sel, bool autoOn, bool available);
    std::function<void(int)> onHz; std::function<void(bool)> onAuto;
protected:
    void showEvent(QShowEvent *e) override; void hideEvent(QHideEvent *e) override;
private:
    struct Bind { int row, code; };
    struct Mon { DdcMonitor m; kit::RowsPanel *panel = nullptr; std::vector<Bind> binds; };
    void rebuild(const QList<DdcMonitor> &ms); void applyValues(int bus, const QMap<int, VcpValue> &v); void check();
    kit::RowsPanel *laptop = nullptr; QVBoxLayout *ext = nullptr; kit::RowsPanel *none = nullptr;
    std::vector<Mon> mons; QThread thread; DdcWorker *worker; QTimer watch; QString sig; bool first = true, scanning = false;
};
#endif
