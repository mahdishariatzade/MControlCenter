#include "rgb.h"
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QThread>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <cerrno>
#include <cstring>

// ───────────────────────────── profile (de)serialisation ─────────────────────────────
static QJsonArray colorsToJson(const QVector<QColor> &v) { QJsonArray a; for (const QColor &c : v) a.append(c.name()); return a; }
static QVector<QColor> colorsFromJson(const QJsonArray &a, const QVector<QColor> &def) { QVector<QColor> v; for (const QJsonValue &x : a) { QColor c(x.toString()); if (c.isValid()) v << c; } return v.isEmpty() ? def : v; }
QJsonObject RgbProfile::toJson() const {
    QJsonObject o; o["mode"] = int(mode); o["color"] = color.name(); o["zones"] = colorsToJson(zones); o["palette"] = colorsToJson(palette);
    o["colorB"] = colorB.name(); o["meterSource"] = meterSource; o["brightness"] = brightness; o["speed"] = speed; o["direction"] = direction; return o;
}
RgbProfile RgbProfile::fromJson(const QJsonObject &o) {
    RgbProfile p; if (o.isEmpty()) return p;
    p.mode = Mode(qBound(0, o["mode"].toInt(0), int(Meter))); QColor cb(o["colorB"].toString()); if (cb.isValid()) p.colorB = cb; p.meterSource = qBound(0, o["meterSource"].toInt(0), 1); QColor c(o["color"].toString()); if (c.isValid()) p.color = c;
    p.zones = colorsFromJson(o["zones"].toArray(), p.zones); while (p.zones.size() < 24) p.zones << p.color; p.zones.resize(24);
    p.palette = colorsFromJson(o["palette"].toArray(), p.palette); if (p.palette.size() > 4) p.palette.resize(4);
    p.brightness = qBound(0, o["brightness"].toInt(100), 100); p.speed = qBound(0, o["speed"].toInt(1), 2); p.direction = o["direction"].toInt(1) ? 1 : 0; return p;
}

// ───────────────────────────── device ─────────────────────────────
#define HID_SET_FEATURE(len) _IOC(_IOC_READ | _IOC_WRITE, 'H', 0x06, len)
#define HID_GET_FEATURE(len) _IOC(_IOC_READ | _IOC_WRITE, 'H', 0x07, len)

static QString sysRead(const QString &p) { QFile f(p); return f.open(QIODevice::ReadOnly) ? QString::fromUtf8(f.readAll()).trimmed() : QString(); }

bool MysticLight::open() {
    close(); err.clear();
    for (const QString &n : QDir("/sys/class/hidraw").entryList({"hidraw*"}, QDir::Dirs | QDir::NoDotAndDotDot)) {
        const QString uev = sysRead("/sys/class/hidraw/" + n + "/device/uevent"); QString id, nm;
        for (const QString &l : uev.split('\n')) { if (l.startsWith("HID_ID=")) id = l.mid(7); else if (l.startsWith("HID_NAME=")) nm = l.mid(9); }
        // HID_ID=0003:00001462:00001603
        const QStringList parts = id.split(':'); if (parts.size() != 3) continue;
        bool okV = false, okP = false; const uint vid = parts[1].toUInt(&okV, 16), pid = parts[2].toUInt(&okP, 16); if (!okV || !okP) continue;
        const bool msi = vid == 0x1462 || vid == 0x0DB0; if (!msi || !nm.contains("MysticLight", Qt::CaseInsensitive)) continue;     // never touch anything that is not a MysticLight keyboard controller
        if (pid >= 0x7B00 && pid <= 0x7FFF) continue;                              // motherboard / desktop controllers speak a different protocol
        path = "/dev/" + n; fd = ::open(path.toLocal8Bit().constData(), O_RDWR | O_CLOEXEC);
        if (fd < 0) { err = QString("cannot open %1: %2").arg(path, strerror(errno)); if (errno == EACCES) err += " (install the udev rule: re-run install.sh)"; return false; }
        zones = (pid == 0x1603 || pid == 0x1605) ? 24 : 4; name = nm; return true;
    }
    err = "no MSI MysticLight keyboard controller found"; return false;
}
void MysticLight::close() { if (fd >= 0) ::close(fd); fd = -1; }

bool MysticLight::send(const QByteArray &payload) {
    char buf[64] = {0}; memcpy(buf, payload.constData(), qMin(64, int(payload.size())));
    if (ioctl(fd, HID_SET_FEATURE(64), buf) < 0) { err = QString("write failed: %1").arg(strerror(errno)); close(); return false; }
    char rd[64] = {0}; rd[0] = 1; (void) ioctl(fd, HID_GET_FEATURE(64), rd);    // the controller expects its status report to be read back
    QThread::usleep(2000); return true;
}
bool MysticLight::selectMask(quint32 mask) { QByteArray p(6, 0); p[0] = 2; p[1] = 1; for (int i = 0; i < 4; ++i) p[2 + i] = char((mask >> (8 * i)) & 0xFF); if (zones == 4) { p.resize(3); } return send(p); }
bool MysticLight::selectAll() { QByteArray p(2, 0); p[0] = 2; p[1] = 1; if (zones == 24) p.append(QByteArray(32, char(0xFF))); else p.append(char(0x0F)); return send(p); }

// effect packet: report 2, packet 2: [mode, duration LE16, 0, 0, zone byte, 1, direction, keyframes (time 0..100, r, g, b)...]
bool MysticLight::effect(int mode, int ticks, int direction, const QVector<QColor> &keys) {
    QByteArray p(64, 0); p[0] = 2; p[1] = 2; p[2] = char(mode); p[3] = char(ticks & 0xFF); p[4] = char((ticks >> 8) & 0xFF);
    p[7] = zones == 24 ? char(0xFF) : char(0x0F); p[8] = 1; p[9] = char(direction);
    const int n = qBound(1, int(keys.size()), 9); int pos = 10;
    for (int i = 0; i < n; ++i) { p[pos] = char(i * 100 / n); p[pos + 1] = char(keys[i].red()); p[pos + 2] = char(keys[i].green()); p[pos + 3] = char(keys[i].blue()); pos += 4; }
    p[pos] = 100; p[pos + 1] = char(keys[0].red()); p[pos + 2] = char(keys[0].green()); p[pos + 3] = char(keys[0].blue());      // close the loop on the first colour
    return send(p);
}

static QColor scaled(const QColor &c, int pct) { return QColor(c.red() * pct / 100, c.green() * pct / 100, c.blue() * pct / 100); }

bool MysticLight::apply(const RgbProfile &p) {
    if (!isOpen()) return false;
    static const int speedTicks[3] = {600, 300, 120};
    QVector<QColor> pal; for (const QColor &c : p.palette) pal << scaled(c, p.brightness); if (pal.isEmpty()) pal << scaled(p.color, p.brightness);
    switch (p.mode) {
        case RgbProfile::Off: return selectAll() && effect(0, 100, 1, {QColor(0, 0, 0)});
        case RgbProfile::Static: return selectAll() && effect(1, 100, 1, {scaled(p.color, p.brightness)});
        case RgbProfile::Breathing: return selectAll() && effect(2, speedTicks[p.speed], 1, pal);
        case RgbProfile::Cycle: return selectAll() && effect(3, speedTicks[p.speed], 1, pal);
        case RgbProfile::Wave: return selectAll() && effect(4, speedTicks[p.speed], p.direction, pal);
        case RgbProfile::Meter:                     // the caller expands a meter into Zones before sending; as a fallback show colour A
            return selectAll() && effect(1, 100, 1, {scaled(p.color, p.brightness)});
        case RgbProfile::Zones: {
            // one select + one static packet per DISTINCT colour (zones sharing a colour are set together)
            QMap<QRgb, quint32> groups; for (int z = 0; z < zones && z < p.zones.size(); ++z) groups[scaled(p.zones[z], p.brightness).rgb()] |= (1u << z);
            for (auto it = groups.begin(); it != groups.end(); ++it) if (!(selectMask(it.value()) && effect(1, 100, 1, {QColor(it.key())}))) return false;
            return true; }
    }
    return false;
}
bool MysticLight::blinkZone(int z) {
    if (!isOpen() || z < 0 || z >= zones) return false;
    return selectAll() && effect(0, 100, 1, {QColor(0, 0, 0)}) && selectMask(1u << z) && effect(2, 90, 1, {QColor(255, 0, 0), QColor(0, 0, 0)});   // fast red breathing on one zone: easy to spot in daylight
}
bool MysticLight::reloadStored() { return isOpen() && send(QByteArray::fromHex("02b0")); }

// ───────────────────────────── worker ─────────────────────────────
bool RgbWorker::ensure() { if (dev.isOpen()) return true; bool ok = dev.open(); emit state(ok, dev.zoneCount(), dev.deviceName(), dev.error()); return ok; }
void RgbWorker::probe() { dev.close(); ensure(); }
void RgbWorker::apply(const RgbProfile &p) { if (!ensure()) return; if (!dev.apply(p)) { emit state(false, dev.zoneCount(), dev.deviceName(), dev.error()); } }
void RgbWorker::previewZone(int zone) { if (ensure()) dev.blinkZone(zone); }
void RgbWorker::reloadStored() { if (ensure()) dev.reloadStored(); }
void RgbWorker::flash(const QColor &c, int times, const RgbProfile &then) {
    if (!ensure()) return; RgbProfile f; f.mode = RgbProfile::Static; f.color = c; f.brightness = 100;
    for (int i = 0; i < times; ++i) { dev.apply(f); QThread::msleep(220); RgbProfile o; o.mode = RgbProfile::Off; dev.apply(o); QThread::msleep(180); }
    dev.apply(then);
}
