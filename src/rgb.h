// Keyboard lighting for MSI laptops with a "MysticLight" HID controller (USB 1462:16xx, MS-156x/160x).
// Protocol facts (zone-select packet, effect/keyframe packet, 24-zone vs 4-zone) come from the community reverse engineering of
// natanalt, mystickeys, and the msi-mysticlight Linux projects; this is an independent implementation.
// SAFETY: only report 2 / packet 1 (zone select) and packet 2 (effect), plus packet 0xB0 (reload what the keyboard has stored). We NEVER send
// the flash-save packet (0xA0) or unknown OEM packets, so nothing here can persist or reconfigure the controller; a reboot restores factory lighting.
#ifndef RGB_H
#define RGB_H
#include <QObject>
#include <QColor>
#include <QVector>
#include <QJsonObject>
#include <QString>

struct RgbProfile {
    enum Mode { Static, Zones, Breathing, Cycle, Wave, Off, Meter };
    Mode mode = Static;
    QColor color = QColor(255, 75, 31);
    QColor colorB = QColor(110, 170, 215);           // secondary colour (gradients)
    int meterSource = 0;                             // Meter mode: 0 CPU temperature, 1 battery level
    QVector<QColor> zones = QVector<QColor>(24, QColor(255, 75, 31));
    QVector<QColor> palette = {QColor(255, 75, 31), QColor(255, 176, 32), QColor(110, 170, 215)};     // colours of the hardware effects
    int brightness = 100;           // 0..100 (applied by scaling the colours)
    int speed = 1;                  // 0 slow, 1 medium, 2 fast
    int direction = 1;              // wave: 1 left-to-right, 0 right-to-left
    QJsonObject toJson() const;
    static RgbProfile fromJson(const QJsonObject &o);
    bool operator==(const RgbProfile &o) const { return toJson() == o.toJson(); }
};
Q_DECLARE_METATYPE(RgbProfile)

// Blocking device access. Used only from the RgbWorker thread.
class MysticLight {
public:
    bool open();                                   // finds /dev/hidrawN for a supported controller
    void close();
    bool isOpen() const { return fd >= 0; }
    int zoneCount() const { return zones; }
    bool experimental() const { return zones == 4; }
    QString deviceName() const { return name; }
    QString error() const { return err; }
    bool apply(const RgbProfile &p);
    bool blinkZone(int zone);                      // calibration: blink exactly one zone red, everything else dark
    bool reloadStored();                           // ask the keyboard to go back to the lighting stored in its own flash
private:
    bool send(const QByteArray &payload);
    bool selectMask(quint32 mask);
    bool selectAll();
    bool effect(int mode, int ticks, int direction, const QVector<QColor> &keys);
    int fd = -1, zones = 24; QString name, err, path;
};

class RgbWorker : public QObject {
Q_OBJECT
public:
    RgbWorker() { qRegisterMetaType<RgbProfile>("RgbProfile"); }
    ~RgbWorker() override { dev.close(); }
public slots:
    void probe();                                  // (re)open and report
    void apply(const RgbProfile &p);
    void flash(const QColor &c, int times, const RgbProfile &then);   // short attention flash, then back to `then`
    void reloadStored();
    void previewZone(int zone);                    // calibration helper (not persisted anywhere)
signals:
    void state(bool present, int zones, const QString &name, const QString &error);
private:
    MysticLight dev; bool ensure();
};
#endif
