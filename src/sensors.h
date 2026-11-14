// Background sensor polling. Reads of EC-backed sysfs files (battery, msi-ec, hwmon) can block for 100+ ms, so they must never run on
// the GUI thread: the worker gathers everything into a Snapshot once per interval and hands it to the UI by queued signal.
#ifndef SENSORS_H
#define SENSORS_H
#include <QObject>
#include <QTimer>
#include <QString>
#include "dashboard.h"

struct Snapshot {
    DashData d;                 // temps, fan %, mode, cooler boost, charge limit (from msi-ec sysfs)
    bool msiEc = false;         // false: the msi-ec driver is absent, the GUI thread falls back to the ec_sys path
    int cap = 0; QString status; double volts = 0, currentUa = 0, chargeNow = 0, chargeFull = 0, chargeDesign = 0; qint64 cycles = 0; bool ac = false;
    QString dgpu; int nvme = -1; QString load; double ghz = 0, memUsedGb = 0, memTotalGb = 0;
};
Q_DECLARE_METATYPE(Snapshot)

class SensorWorker : public QObject {
Q_OBJECT
public:
    SensorWorker();
public slots:
    void start();                 // call from the worker thread (QThread::started)
    void setFast(bool fast);      // 1 s while the window is visible, 5 s while it sits in the tray
    void pollNow();
signals:
    void snapshot(const Snapshot &s);
private:
    void poll();
    QTimer *timer = nullptr; quint64 lastTotal = 0, lastIdle = 0; QString batPath;
};
#endif
