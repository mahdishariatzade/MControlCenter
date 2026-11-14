#include "keyboard.h"
#include "settings.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QFile>
#include <QSet>
#include <algorithm>

using namespace kit;

// ───────────────────────────── layout ─────────────────────────────
static const double kRowY[6] = {0, 1.15, 2.15, 3.15, 4.15, 5.15};
static QVector<KeyDef> buildLayout() {
    QVector<KeyDef> k; auto add = [&](const QString &l, double x, double y, double w = 1, double h = 1) { k.push_back({l, QRectF(x, y, w, h)}); };
    add("Esc", 0, kRowY[0], 1, 0.8); for (int f = 1; f <= 12; ++f) add(QString("F%1").arg(f), f, kRowY[0], 1, 0.8); add("PrtSc", 13, kRowY[0], 1, 0.8); add("Del", 14, kRowY[0], 1, 0.8);
    double x = 0; for (const char *c : {"`", "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "-", "="}) add(c, x++, kRowY[1]); add("Bksp", x, kRowY[1], 2);
    add("Tab", 0, kRowY[2], 1.5); x = 1.5; for (const char *c : {"Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P", "[", "]"}) add(c, x++, kRowY[2]); add("\\", x, kRowY[2], 1.5);
    add("Caps", 0, kRowY[3], 1.75); x = 1.75; for (const char *c : {"A", "S", "D", "F", "G", "H", "J", "K", "L", ";", "'"}) add(c, x++, kRowY[3]); add("Enter", x, kRowY[3], 2.25);
    add("Shift", 0, kRowY[4], 2.25); x = 2.25; for (const char *c : {"Z", "X", "C", "V", "B", "N", "M", ",", ".", "/"}) add(c, x++, kRowY[4]); add("Shift", x, kRowY[4], 1.5); add("▲", x + 1.5, kRowY[4]);
    add("Ctrl", 0, kRowY[5], 1.25); add("Fn", 1.25, kRowY[5]); add("Win", 2.25, kRowY[5], 1.25); add("Alt", 3.5, kRowY[5], 1.25); add("Space", 4.75, kRowY[5], 5.25); add("Alt", 10, kRowY[5]); add("Ctrl", 11, kRowY[5]);
    add("◀", 12, kRowY[5]); add("▼", 13, kRowY[5]); add("▶", 14, kRowY[5]);
    const double nx = 15.4;                                                          // numpad
    const char *np1[4] = {"Num", "/", "*", "-"}; for (int i = 0; i < 4; ++i) add(np1[i], nx + i, kRowY[1]);
    const char *np2[3] = {"7", "8", "9"}; for (int i = 0; i < 3; ++i) add(np2[i], nx + i, kRowY[2]); add("+", nx + 3, kRowY[2], 1, 2);
    const char *np3[3] = {"4", "5", "6"}; for (int i = 0; i < 3; ++i) add(np3[i], nx + i, kRowY[3]);
    const char *np4[3] = {"1", "2", "3"}; for (int i = 0; i < 3; ++i) add(np4[i], nx + i, kRowY[4]); add("Ent", nx + 3, kRowY[4], 1, 2);
    add("0", nx, kRowY[5], 2); add(".", nx + 2, kRowY[5]);
    return k;
}
const QVector<KeyDef> &keyboardLayout() { static const QVector<KeyDef> l = buildLayout(); return l; }
QSizeF keyboardExtent() { return QSizeF(19.4, 6.15); }

// ───────────────────────────── zone map ─────────────────────────────
static QString mapId() { QFile f("/sys/class/dmi/id/board_name"); return f.open(QIODevice::ReadOnly) ? QString::fromUtf8(f.readAll()).trimmed() : QString("unknown"); }

KeyMap KeyMap::defaults() {
    KeyMap m; const auto &L = keyboardLayout(); m.keyZone.resize(L.size());
    for (int i = 0; i < L.size(); ++i) { int row = 0; for (int r = 0; r < 6; ++r) if (L[i].r.top() >= kRowY[r] - 0.01) row = r; int band = qBound(0, int((L[i].r.center().x() / keyboardExtent().width()) * 4), 3); m.keyZone[i] = row * 4 + band; }
    return m;                                                                 // NOT verified on hardware: the calibration wizard replaces it
}
KeyMap KeyMap::load() {
    KeyMap d = defaults(); Settings s; const QString j = s.getValue("Rgb/keymap").toString(); if (j.isEmpty()) return d;
    const QJsonObject o = QJsonDocument::fromJson(j.toUtf8()).object(); if (o["board"].toString() != mapId()) return d;
    QVector<int> kz; for (const QJsonValue &v : o["keyZone"].toArray()) kz << v.toInt(-1);
    if (kz.size() != d.keyZone.size()) return d; d.keyZone = kz; d.calibrated = o["calibrated"].toBool(); return d;
}
void KeyMap::save() const { QJsonArray a; for (int z : keyZone) a.append(z); QJsonObject o; o["board"] = mapId(); o["calibrated"] = calibrated; o["keyZone"] = a; Settings::setValue("Rgb/keymap", QString::fromUtf8(QJsonDocument(o).toJson(QJsonDocument::Compact))); }
void KeyMap::reset() { Settings::setValue("Rgb/keymap", QString()); }
QPointF KeyMap::zoneCentre(int zone) const {
    const auto &L = keyboardLayout(); double sx = 0, sy = 0; int n = 0; for (int i = 0; i < L.size() && i < keyZone.size(); ++i) if (keyZone[i] == zone) { sx += L[i].r.center().x(); sy += L[i].r.center().y(); ++n; }
    return n ? QPointF(sx / n, sy / n) : QPointF(-1, -1);
}
bool KeyMap::zoneMapped(int zone) const { return zoneCentre(zone).x() >= 0; }
QVector<int> KeyMap::zonesBottomUp() const {
    QVector<int> z; for (int i = 0; i < 24; ++i) if (zoneMapped(i)) z << i;
    std::sort(z.begin(), z.end(), [this](int a, int b) { QPointF pa = zoneCentre(a), pb = zoneCentre(b); if (qAbs(pa.y() - pb.y()) > 0.6) return pa.y() > pb.y(); return pa.x() < pb.x(); });
    return z;
}

// ───────────────────────────── widget ─────────────────────────────
KeyboardMap::KeyboardMap(QWidget *p) : Panel("", "KEYBOARD", p) { setMinimumHeight(300); map = KeyMap::load(); }

QRectF KeyboardMap::keyRect(int i) const { const QRectF &r = keyboardLayout()[i].r; return QRectF(origin.x() + r.x() * scale, origin.y() + r.y() * scale, r.width() * scale, r.height() * scale).adjusted(2, 2, -2, -2); }
int KeyboardMap::keyAt(QPointF pt) const { for (int i = 0; i < keyboardLayout().size(); ++i) if (keyRect(i).contains(pt)) return i; return -1; }

void KeyboardMap::startCalibration() { calib = true; marked = QVector<QSet<int>>(zones); gotoStep(0); update(); }
void KeyboardMap::gotoStep(int s) { step = qBound(0, s, zones - 1); if (onCalibZone) onCalibZone(step); update(); }
void KeyboardMap::finishCalibration() {
    KeyMap m = map; m.keyZone = QVector<int>(m.keyZone.size(), -1);
    for (int z = 0; z < zones; ++z) for (int k : marked[z]) m.keyZone[k] = z;
    m.calibrated = true; m.save(); map = m; calib = false; recomputeSel(); if (onMapChanged) onMapChanged(); if (onCalibEnd) onCalibEnd(); update();
}

void KeyboardMap::mousePressEvent(QMouseEvent *e) {
    const QPointF pt = e->position();
    if (calib) {
        if (bCancel.contains(pt)) { calib = false; if (onCalibEnd) onCalibEnd(); update(); return; }
        if (bFinish.contains(pt)) { finishCalibration(); return; }
        for (int z = 0; z < zones; ++z) if (chips[z].contains(pt)) { gotoStep(z); return; }
        if (bBack.contains(pt) && step > 0) { gotoStep(step - 1); return; }
        if (bNext.contains(pt)) { if (step >= zones - 1) finishCalibration(); else gotoStep(step + 1); return; }
        int k = keyAt(pt); if (k >= 0) { if (marked[step].contains(k)) marked[step].remove(k); else { for (auto &s : marked) s.remove(k); marked[step].insert(k); } drag = k; update(); }
        return;
    }
    if (!advanced) return;
    if (bAll.contains(pt)) { selectAll(); if (onSelect) onSelect(); return; }
    if (bZones.contains(pt)) { showZones = !showZones; update(); return; }
    if (bCal.contains(pt)) { startCalibration(); return; }
    int k = keyAt(pt); if (k < 0 || !present || map.keyZone.value(k, -1) < 0) return;
    if (e->modifiers() & Qt::ControlModifier) { if (selKeys.contains(k)) selKeys.remove(k); else selKeys.insert(k); } else selKeys = QSet<int>{k};
    drag = k; recomputeSel(); if (onSelect) onSelect(); update();
}
void KeyboardMap::mouseMoveEvent(QMouseEvent *e) {
    int k = keyAt(e->position()); if (k != hotKey) { hotKey = k; update(); }
    const QPointF pt = e->position(); bool btn = bAll.contains(pt) || bZones.contains(pt) || bCal.contains(pt) || (calib && (bBack.contains(pt) || bNext.contains(pt) || bCancel.contains(pt) || bFinish.contains(pt)));
    if (calib) for (int z = 0; z < zones; ++z) btn |= chips[z].contains(pt);
    setCursor(btn || k >= 0 ? Qt::PointingHandCursor : Qt::ArrowCursor);
    if (drag >= 0 && k >= 0 && k != drag) {
        if (calib) { if (!marked[step].contains(k)) { for (auto &s : marked) s.remove(k); marked[step].insert(k); update(); } }
        else if (map.keyZone.value(k, -1) >= 0 && !selKeys.contains(k)) { selKeys.insert(k); recomputeSel(); if (onSelect) onSelect(); update(); }
    }
}
void KeyboardMap::mouseReleaseEvent(QMouseEvent *) { drag = -1; }

static QColor zoneColor(const RgbProfile &p, int z) {
    switch (p.mode) { case RgbProfile::Off: return QColor(34, 35, 38); case RgbProfile::Static: return p.color; case RgbProfile::Zones: return p.zones.value(z, p.color);
        case RgbProfile::Meter: return p.color; default: { int n = qMax(1, int(p.palette.size())); return p.palette[z % n]; } }
}

void KeyboardMap::paintBody(QPainter &p, const QRectF &b) {
    auto button = [&](QRectF &r, double right, const QString &t, bool on) { QFont f = fnt(RAJ, 13, QFont::Bold, 1.3); p.setFont(f); double w = QFontMetricsF(f).horizontalAdvance(t) + 22; r = QRectF(right - w, b.top() - 31, w, 22);
        p.setPen(QPen(on ? gAccent : kLine, 1)); p.setBrush(on ? alpha(gAccent, 0.18) : QBrush(Qt::NoBrush)); p.drawPath(chamfer(r, 5)); p.setPen(on ? gAccent : kText); p.drawText(r, Qt::AlignCenter, t); return r.left() - 6; };
    double rx = b.right();
    if (!calib && !advanced) { bAll = bZones = bCal = QRectF(); }
    if (!calib && advanced) { rx = button(bAll, rx, "SELECT ALL", false); rx = button(bZones, rx, showZones ? "HIDE ZONE NUMBERS" : "SHOW ZONE NUMBERS", showZones); button(bCal, rx, map.calibrated ? "RE-CALIBRATE" : "CALIBRATE KEYBOARD", !map.calibrated); }

    // keyboard drawing
    const QSizeF ext = keyboardExtent(); const double footer = calib ? 54 : 22; area = QRectF(b.left(), b.top() + 2, b.width(), b.height() - footer - 4);
    scale = qMin(area.width() / ext.width(), area.height() / ext.height()); origin = QPointF(area.left() + (area.width() - scale * ext.width()) / 2, area.top() + (area.height() - scale * ext.height()) / 2);
    const auto &L = keyboardLayout(); QSet<int> markedAny; if (calib) for (int z = 0; z < zones; ++z) if (z != step) for (int k : marked[z]) markedAny.insert(k);
    for (int i = 0; i < L.size(); ++i) {
        const QRectF r = keyRect(i); int z = map.keyZone.value(i, -1); bool selected = !calib && advanced && selKeys.contains(i); bool affected = !calib && advanced && !selected && z >= 0 && (sel & (1u << z)); bool hot = i == hotKey;
        QColor fill;
        if (calib) fill = marked[step].contains(i) ? QColor(255, 40, 30) : markedAny.contains(i) ? QColor(40, 70, 45) : QColor(28, 30, 33);
        else if (z < 0) fill = QColor(28, 30, 33);
        else { fill = prof ? zoneColor(*prof, z) : kDim; if (prof && prof->brightness < 100) { int f = 30 + prof->brightness * 70 / 100; fill = QColor(fill.red() * f / 100, fill.green() * f / 100, fill.blue() * f / 100); } }
        p.setPen(QPen(selected ? kText : affected ? gAccent : hot ? gAccent : alpha(kText, z < 0 && !calib ? 0.08 : 0.25), selected ? 2.5 : affected ? 1.6 : 1, affected ? Qt::DashLine : Qt::SolidLine)); p.setBrush(fill); p.drawRoundedRect(r, 3, 3);
        double lum = fill.redF() * 0.3 + fill.greenF() * 0.59 + fill.blueF() * 0.11; p.setPen(lum > 0.55 ? QColor(20, 20, 20) : alpha(kText, 0.85));
        QFont lf = fnt(RAJ, qMax(8, int(scale * 0.34)), QFont::DemiBold); p.setFont(lf); p.drawText(r, Qt::AlignCenter, L[i].label);
        if (showZones && z >= 0 && !calib) { p.setFont(fnt(MONO, qMax(7, int(scale * 0.22)), QFont::Medium)); p.setPen(lum > 0.55 ? QColor(20, 20, 20, 190) : alpha(kText, 0.6)); p.drawText(r.adjusted(3, 1, 0, 0), Qt::AlignTop | Qt::AlignLeft, QString::number(z)); }
    }

    // footer
    p.setFont(fnt(MONO, 10));
    if (!calib && !advanced) {
        p.setPen(kDim); p.drawText(QRectF(b.left(), b.bottom() - 18, b.width(), 18), Qt::AlignLeft | Qt::AlignVCenter, present ? "Live preview of your keyboard. Choose a look and a colour below." : "No keyboard controller connected.");
    } else if (!calib) {
        p.setPen(present ? kDim : gAccent);
        p.drawText(QRectF(b.left(), b.bottom() - 18, b.width(), 18), Qt::AlignLeft | Qt::AlignVCenter,
                   !present ? "No keyboard controller connected." : map.calibrated ? "Click keys (drag or Ctrl+click for more), then choose a colour. Dashed keys share a hardware zone with your pick and change too."
                                                                                  : "Click keys to pick them. Dashed keys share a hardware zone with your pick and change too. The key-to-zone map is a guess until you CALIBRATE.");
    } else {
        // progress strip: one chip per zone (done / current / empty); click a chip to jump to that zone
        const double cw = 17, cg = 3; double cx = b.right() - zones * (cw + cg) + cg; int done = 0;
        for (int z = 0; z < zones; ++z) {
            chips[z] = QRectF(cx + z * (cw + cg), b.top() - 31, cw, 22); bool cur = z == step, has = !marked[z].isEmpty(); done += has;
            p.setPen(QPen(cur ? kText : kLine, cur ? 2 : 1)); p.setBrush(has ? QColor(255, 70, 40, 200) : QBrush(Qt::NoBrush)); p.drawRect(chips[z]);
            p.setFont(fnt(MONO, 8, QFont::Medium)); p.setPen(has ? kText : kDim); p.drawText(chips[z], Qt::AlignCenter, QString::number(z + 1));
        }
        p.setPen(gAccent); p.setFont(fnt(RAJ, 17, QFont::Bold, 1.5)); p.drawText(QRectF(b.left(), b.bottom() - 52, b.width() - 480, 22), Qt::AlignLeft | Qt::AlignVCenter, QString("CALIBRATING  ·  ZONE %1 OF %2  ·  %3 DONE").arg(step + 1).arg(zones).arg(done));
        p.setPen(kDim); p.setFont(fnt(MONO, 10)); p.drawText(QRectF(b.left(), b.bottom() - 30, b.width() - 480, 30), Qt::AlignLeft | Qt::TextWordWrap, "On your keyboard some keys blink red. Click exactly those keys here (drag to add several). Nothing blinking? Press NEXT. You can save any time.");
        QFont f = fnt(RAJ, 14, QFont::Bold, 1.5); p.setFont(f); double x = b.right();
        auto btn = [&](QRectF &r, const QString &t, bool fill, bool en) { double w = QFontMetricsF(f).horizontalAdvance(t) + 24; r = QRectF(x - w, b.bottom() - 40, w, 32); x -= w + 8; p.setPen(QPen(fill ? gAccent : kLine, 1)); p.setBrush(fill ? gAccent : QBrush(Qt::NoBrush)); p.drawPath(chamfer(r, 7)); p.setPen(!en ? alpha(kDim, 0.5) : fill ? kBg : kText); p.drawText(r, Qt::AlignCenter, t); };
        btn(bFinish, "SAVE & FINISH", true, true);
        if (step < zones - 1) btn(bNext, "NEXT ZONE", false, true); else bNext = QRectF();
        btn(bBack, "BACK", false, step > 0); btn(bCancel, "CANCEL", false, true);
    }
}
