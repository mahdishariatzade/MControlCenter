// DDC/CI (VESA MCCS) control of external monitors over the video cable (DisplayPort / HDMI / USB-C DP-alt): brightness, contrast, input, colour...
// Pure Linux i2c-dev, no ddcutil needed. Only ever talks to the monitor's DDC address 0x37 on the I2C bus that belongs to a connected DRM connector.
// Every call blocks for ~50-300 ms, so it is only used from DdcWorker (a worker thread), never from the GUI thread.
#ifndef DDC_H
#define DDC_H
#include <QObject>
#include <QString>
#include <QList>
#include <QMap>
#include <QSet>
#include <QVariantMap>

struct VcpValue { bool ok = false; int cur = 0, max = 0; };
using VcpMap = QMap<int, VcpValue>;

struct DdcMonitor {
    QString connector;             // e.g. "DP-1", "HDMI-A-1"
    int bus = -1;                  // /dev/i2c-N
    QString maker, model, edidName;
    QString caps;                  // raw MCCS capabilities string ("" if the monitor did not answer)
    QSet<int> codes;               // supported VCP codes (from caps)
    QMap<int, QList<int>> allowed; // non-continuous features: allowed values (e.g. input sources)
    bool ddcOk = false;            // answered a brightness read
    QString title() const { return model.isEmpty() ? connector : model; }
};

namespace ddc {
QList<DdcMonitor> scan();                                  // connected external connectors + EDID (cheap, no i2c traffic) - DDC probing is done by probe()
void probe(DdcMonitor &m);                                 // opens the bus, reads brightness + capabilities (slow)
bool get(int bus, int code, VcpValue &out);
bool set(int bus, int code, int value);
QString connectorsSignature();                             // changes when a monitor is plugged/unplugged (sysfs only, very cheap)
QString busAccessHint(int bus);                            // "" if /dev/i2c-N is usable, else why not
}

// Lives in its own thread. All results come back as queued signals.
class DdcWorker : public QObject {
    Q_OBJECT
public slots:
    void rescan();                                                       // scan + probe every external monitor
    void readAll(int bus, QList<int> codes);                             // read these VCP codes
    void write(int bus, int code, int value, QList<int> readBack);       // set, then read these back
signals:
    void scanned(QList<DdcMonitor> monitors);
    void values(int bus, VcpMap v);
    void failed(int bus, QString what);
};
Q_DECLARE_METATYPE(QList<DdcMonitor>)
Q_DECLARE_METATYPE(VcpMap)
#endif
