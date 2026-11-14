// Per-application profiles: when a chosen program is running, switch the power mode / Cooler Boost / laptop-screen refresh rate, and put everything
// back when it closes. Cheap: one pass over /proc/*/comm every few seconds, and only while the feature is on and at least one rule exists.
#ifndef APPPROFILES_H
#define APPPROFILES_H
#include <QObject>
#include <QTimer>
#include <QString>
#include <QList>

struct AppRule {
    QString proc;        // process name as shown by `ps -e` / the Running list (first 15 characters are compared, case-insensitive)
    int mode = -1;       // -1 leave alone, 0 turbo, 1 balanced, 2 silent, 3 eco
    int boost = -1;      // -1 leave alone, 0 off, 1 on
    int hz = -1;         // laptop screen: -1 leave alone, 0 highest, otherwise that many Hz
};

class AppProfiles : public QObject {
    Q_OBJECT
public:
    explicit AppProfiles(QObject *p = nullptr);
    QList<AppRule> rules; bool enabled = false;
    void load(); void save(); void setEnabled(bool on);
    static QStringList runningApps();                 // distinct process names, for the picker
    bool edit(QWidget *parent);                       // modal editor; true if the user saved
signals:
    void activated(AppRule rule);
    void deactivated();
private:
    void tick(); void restart();
    QTimer timer; QString active;
};
#endif
