// Physical keyboard drawing + the zone <-> key map of the lighting controller.
// The controller exposes 24 numbered zones, but which keys belong to which zone differs per keyboard, and the published guesses
// (row-major 6x4) did not match real hardware. So the map is MEASURED on the user's own keyboard by the calibration wizard and stored.
#ifndef KEYBOARD_H
#define KEYBOARD_H
#include <QRectF>
#include <QString>
#include <QVector>
#include <QPointF>
#include <QJsonObject>
#include <QSet>
#include <functional>
#include "uikit.h"
#include "rgb.h"

struct KeyDef { QString label; QRectF r; };           // r in key units: 1.0 = one standard key
const QVector<KeyDef> &keyboardLayout();             // 16" MSI layout with numpad (the same on every Crosshair/Vector/Pulse 16")
QSizeF keyboardExtent();

struct KeyMap {
    QVector<int> keyZone;                             // per key: zone 0..23, or -1 = not mapped
    bool calibrated = false;
    static KeyMap defaults();                         // the row-major guess, flagged uncalibrated
    static KeyMap load(); void save() const; static void reset();
    QPointF zoneCentre(int zone) const;               // centre of a zone's keys in key units (for gradients / meters)
    QVector<int> zonesBottomUp() const;               // zones ordered bottom row first, then left to right (for bar meters)
    bool zoneMapped(int zone) const;
};

class KeyboardMap : public kit::Panel {
public:
    explicit KeyboardMap(QWidget *p);
    RgbProfile *prof = nullptr; KeyMap map; int zones = 24; bool present = false; quint32 sel = 0xFFFFFFu;
    bool showZones = false;
    std::function<void()> onSelect, onMapChanged;
    std::function<void(int)> onCalibZone;             // wizard: blink this zone on the keyboard
    std::function<void()> onCalibEnd;
    void startCalibration(); bool calibrating() const { return calib; }
    QSet<int> selKeys;                                 // the keys the user picked; `sel` is the set of hardware zones they belong to
    void setSelectedKeys(const QSet<int> &k) { selKeys = k; recomputeSel(); update(); }
    QSet<int> selectedKeys() const { return selKeys; }
    void recomputeSel() { sel = 0; for (int k : selKeys) { int z = map.keyZone.value(k, -1); if (z >= 0) sel |= 1u << z; } }
    void selectAll() { selKeys.clear(); for (int i = 0; i < map.keyZone.size(); ++i) if (map.keyZone[i] >= 0) selKeys.insert(i); recomputeSel(); update(); }
protected:
    void paintBody(QPainter &p, const QRectF &b) override;
    void mousePressEvent(QMouseEvent *e) override; void mouseMoveEvent(QMouseEvent *e) override; void mouseReleaseEvent(QMouseEvent *) override;
private:
    QRectF keyRect(int i) const; int keyAt(QPointF pt) const; void finishCalibration(); void gotoStep(int s);
    QRectF area, bAll, bZones, bCal, bBack, bNext, bCancel, bFinish, chips[24]; double scale = 1; QPointF origin; int hotKey = -1, drag = -1;
    bool calib = false; int step = 0; QVector<QSet<int>> marked;
};
#endif
