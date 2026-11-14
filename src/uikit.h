// Shared design kit: palette, fonts, shapes and the Panel / TileStrip widgets used by every page, so the whole app speaks one visual language.
#ifndef UIKIT_H
#define UIKIT_H
#include <QWidget>
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QFile>
#include <QtMath>
#include <QFontMetricsF>
#include <functional>
#include <vector>
#include <cmath>

namespace kit {
// ── palette: warm graphite, bone-white text, ONE accent that follows the mode ──
inline const QColor kBg(14, 15, 17), kPanel(21, 23, 26), kLine(255, 255, 255, 38), kText(233, 230, 223), kDim(139, 143, 150), kHot(255, 70, 50);
inline const QColor kAccentMode[4] = {QColor(255, 75, 31), QColor(231, 224, 205), QColor(110, 170, 215), QColor(150, 205, 90)};   // turbo, balanced, silent, eco
inline QColor gAccent = kAccentMode[1], gAccentTarget = kAccentMode[1];
inline bool gBoost = false, gReduceMotion = false; inline double gClock = 0;   // gReduceMotion: no entrance animations, no hover easing (accessibility + a little less CPU)           // seconds since show, drives the hazard strip

inline const char *RAJ = "Rajdhani", *MONO = "IBM Plex Mono";
inline QFont fnt(const char *fam, int px, int weight = QFont::Normal, qreal ls = 0) {
    QFont f(fam); f.setPixelSize(px); f.setWeight(QFont::Weight(weight)); f.setLetterSpacing(QFont::AbsoluteSpacing, ls); return f;
}
inline QString readFile(const QString &path) { QFile f(path); if (!f.open(QIODevice::ReadOnly)) return {}; return QString::fromUtf8(f.readAll()).trimmed(); }
inline qint64 readInt(const QString &path, qint64 def = -1) { bool ok; qint64 v = readFile(path).toLongLong(&ok); return ok ? v : def; }
inline QColor mix(const QColor &a, const QColor &b, double t) { t = qBound(0.0, t, 1.0); return QColor::fromRgbF(a.redF() + (b.redF() - a.redF()) * t, a.greenF() + (b.greenF() - a.greenF()) * t, a.blueF() + (b.blueF() - a.blueF()) * t, a.alphaF() + (b.alphaF() - a.alphaF()) * t); }
inline QColor alpha(QColor c, double a) { c.setAlphaF(a); return c; }
inline double ease(double dt, double tau) { return 1 - exp(-dt / tau); }      // frame-rate independent smoothing factor
inline double easeOut(double t) { t = qBound(0.0, t, 1.0); return 1 - pow(1 - t, 3); }

// panel outline with two cut corners (top-right, bottom-left) - the one shape the whole UI is built from
inline QPainterPath chamfer(const QRectF &r, double c) {
    QPainterPath p; p.moveTo(r.left(), r.top()); p.lineTo(r.right() - c, r.top()); p.lineTo(r.right(), r.top() + c); p.lineTo(r.right(), r.bottom());
    p.lineTo(r.left() + c, r.bottom()); p.lineTo(r.left(), r.bottom() - c); p.closeSubpath(); return p;
}

// LED-style segmented meter. `colorAt(v)` colours each lit segment by the value it represents.
inline void segBar(QPainter &p, const QRectF &r, double frac, int n, double peak, const std::function<QColor(double)> &colorAt, double gap = 3) {
    double w = (r.width() - gap * (n - 1)) / n;
    for (int i = 0; i < n; ++i) {
        QRectF s(r.left() + i * (w + gap), r.top(), w, r.height());
        double lit = qBound(0.0, frac * n - i, 1.0);
        p.setPen(Qt::NoPen);
        p.setBrush(alpha(kText, 0.10)); p.drawRect(s);
        if (lit > 0) { QColor c = colorAt((i + 0.5) / n); c.setAlphaF(0.35 + 0.65 * lit); p.setBrush(c); p.drawRect(s); }
    }
    if (peak >= 0) {                                     // peak-hold tick: a taller mark that decays slowly
        int pi = qBound(0, int(peak * n), n - 1);
        double x = r.left() + pi * (w + gap) + w / 2;
        p.setPen(QPen(gAccent, 2)); p.drawLine(QPointF(x, r.top() - 5), QPointF(x, r.bottom() + 5));
    }
}
inline QString fmtEta(double hours) { if (hours <= 0 || hours > 48) return {}; int m = int(hours * 60 + 0.5); return QString("%1h %2m").arg(m / 60).arg(m % 60, 2, 10, QChar('0')); }
inline QColor tempColor(double v) { double c = 30 + v * 70; return c < 72 ? kText : c < 86 ? gAccent : kHot; }

#include "logos.h"
inline QColor glassAt(double t) { const QColor top(31, 33, 38), bot(18, 19, 22); t = qBound(0.0, t, 1.0); return t < 0.5 ? mix(top, kPanel, t * 2) : mix(kPanel, bot, (t - 0.5) * 2); }   // vertical gradient of every panel


// ───────────────────────────── panel base: chamfered frame, index + title, entrance animation ─────────────────────────────
class Panel : public QWidget {
public:
    Panel(const QString &idx, const QString &title, QWidget *p) : QWidget(p), idx(idx), title(title) { icon = iconForTitle(title); setMouseTracking(true); }
    QString idx, title; int icon = IcNone; double reveal = 0, hov = 0, delay = 0; bool hovering = false; std::function<void()> wake;   // wake: lets an idle page restart its animation timer
    bool tick(double clock, double dt) { reveal = gReduceMotion ? 1 : easeOut((clock - delay) / 0.38); double old = hov; hov += ((hovering ? 1 : 0) - hov) * (gReduceMotion ? 1.0 : ease(dt, 0.07)); return reveal < 1 || fabs(hov - old) > 0.002; }
protected:
    virtual void paintBody(QPainter &, const QRectF &) = 0;
    void enterEvent(QEnterEvent *) override { hovering = true; if (wake) wake(); }
    void leaveEvent(QEvent *) override { hovering = false; if (wake) wake(); }
    void paintEvent(QPaintEvent *) override {
        if (reveal <= 0.001) return;
        QPainter p(this); p.setRenderHint(QPainter::Antialiasing); p.setOpacity(reveal); p.translate(0, (1 - reveal) * 14);
        QRectF r = QRectF(rect()).adjusted(1, 1, -1, -1); QPainterPath path = chamfer(r, 13);
        QLinearGradient glass(r.topLeft(), r.bottomLeft()); glass.setColorAt(0, glassAt(0)); glass.setColorAt(0.5, glassAt(0.5)); glass.setColorAt(1, glassAt(1));
        p.fillPath(path, glass);
        QLinearGradient wash(r.topLeft(), r.topLeft() + QPointF(0, 70)); wash.setColorAt(0, alpha(gAccent, 0.07 + 0.05 * hov)); wash.setColorAt(1, alpha(gAccent, 0));
        p.fillPath(path, wash);                                        // faint accent light spilling from the top edge, like a lit glass pane
        p.setPen(QPen(alpha(kText, 0.07), 1)); p.drawLine(r.topLeft() + QPointF(16, 1.5), r.topRight() - QPointF(20, -1.5));   // specular edge
        if (hov > 0.02) neon(p, path, alpha(gAccent, 0.85 * hov), 1.1);   // hover bloom
        else { p.setPen(QPen(kLine, 1)); p.setBrush(Qt::NoBrush); p.drawPath(path); }
        p.setPen(QPen(gAccent, 2));                                    // registration ticks on the two square corners
        p.drawLine(r.topLeft() + QPointF(0, 11), r.topLeft()); p.drawLine(r.topLeft(), r.topLeft() + QPointF(11, 0));
        p.drawLine(r.bottomRight() - QPointF(0, 11), r.bottomRight()); p.drawLine(r.bottomRight(), r.bottomRight() - QPointF(11, 0));
        if (icon != IcNone) paintIcon(p, QRectF(r.left() + 16, r.top() + 9, 20, 20), icon, gAccent);
        p.setFont(fnt(RAJ, 14, QFont::DemiBold, 2.2)); p.setPen(kDim); p.drawText(QRectF(r.left() + (icon != IcNone ? 44 : 18), r.top() + 11, r.width() - 80, 16), Qt::AlignLeft | Qt::AlignVCenter, title);
        paintBody(p, QRectF(r.left() + 18, r.top() + 38, r.width() - 36, r.height() - 38 - 14));
    }
};


// ───────────────────────────── tile strip (modes / presets) ─────────────────────────────
class TileStrip : public QWidget {
public:
    struct Item { QString label, sub; QColor accent; bool enabled = true; bool visible = true; };
    TileStrip(bool checkable, QWidget *p) : QWidget(p), checkable(checkable) { setMouseTracking(true); setFixedHeight(checkable ? 58 : 46); }
    std::vector<Item> items; int checked = -1, active = -1; bool controls = false; double delay = 0, reveal = 0; std::function<void(int)> onClick;   // active: an action tile whose result is currently in effect
    std::vector<double> hov, press;
    void setItems(std::vector<Item> v) { items = std::move(v); hov.assign(items.size(), 0); press.assign(items.size(), 0); update(); }
    bool tick(double clock, double dt) { reveal = gReduceMotion ? 1 : easeOut((clock - delay) / 0.38); bool act = reveal < 1; for (size_t i = 0; i < items.size(); ++i) { double h = hov[i]; hov[i] += (((int) i == hot ? 1 : 0) - hov[i]) * ease(dt, 0.07); press[i] *= exp(-dt / 0.1); act |= fabs(hov[i] - h) > 0.002 || press[i] > 0.01; } return act; }
protected:
    int hot = -1; bool checkable;
    int visibleCount() const { int n = 0; for (auto &i : items) n += i.visible; return n; }
    QRectF rectOf(int idx) const {
        int n = visibleCount(), k = 0; if (!n) return {}; double gap = 8, w = (width() - gap * (n - 1)) / n;
        for (int i = 0; i < (int) items.size(); ++i) { if (!items[i].visible) continue; if (i == idx) return QRectF(k * (w + gap), 0, w, height()); ++k; }
        return {};
    }
    int hit(QPointF pt) const { for (int i = 0; i < (int) items.size(); ++i) if (items[i].visible && rectOf(i).contains(pt)) return i; return -1; }
    void mouseMoveEvent(QMouseEvent *e) override { hot = hit(e->position()); setCursor(hot >= 0 && controls && items[hot].enabled ? Qt::PointingHandCursor : Qt::ArrowCursor); }
    void leaveEvent(QEvent *) override { hot = -1; }
    void mousePressEvent(QMouseEvent *e) override { int i = hit(e->position()); if (i >= 0 && controls && items[i].enabled) { press[i] = 1; if (onClick) onClick(i); } }
    void paintEvent(QPaintEvent *) override {
        if (reveal <= 0.001) return;
        QPainter p(this); p.setRenderHint(QPainter::Antialiasing); p.setOpacity(reveal); p.translate(0, (1 - reveal) * 14);
        for (int i = 0; i < (int) items.size(); ++i) {
            if (!items[i].visible) continue; const Item &it = items[i]; bool on = checkable && i == checked;
            QRectF r = rectOf(i).adjusted(1, 1, -1, -1); double s = 1 - 0.03 * press[i]; r = QRectF(r.center().x() - r.width() / 2 * s, r.center().y() - r.height() / 2 * s, r.width() * s, r.height() * s);
            QColor ac = checkable ? it.accent : gAccent; QPainterPath path = chamfer(r, checkable ? 11 : 8);
            bool act = !checkable && i == active;
            p.setPen(QPen(on || act ? ac : mix(kLine, alpha(ac, 0.9), hov[i]), act ? 1.6 : 1)); p.setBrush(on ? ac : act ? alpha(ac, 0.10) : alpha(ac, 0.06 * hov[i])); p.drawPath(path);
            QColor tc = !it.enabled ? alpha(kDim, 0.5) : on ? kBg : kText;
            p.setFont(fnt(RAJ, checkable ? 21 : 17, QFont::Bold, 1.5)); p.setPen(tc); p.drawText(QRectF(r.left() + 14, r.top() + (checkable ? 7 : 5), r.width() - 28, 24), Qt::AlignLeft | Qt::AlignVCenter, it.label.toUpper());
            p.setFont(fnt(MONO, 10, QFont::Normal, 0)); p.setPen(on ? alpha(kBg, 0.7) : kDim); p.drawText(QRectF(r.left() + 14, r.top() + (checkable ? 32 : 26), r.width() - 28, 16), Qt::AlignLeft | Qt::AlignVCenter, it.sub);
            if (checkable) { p.setFont(fnt(MONO, 10, QFont::Medium, 1)); p.setPen(on ? alpha(kBg, 0.8) : alpha(kDim, 0.7)); p.drawText(QRectF(r.right() - 38, r.top() + 8, 28, 14), Qt::AlignRight, QString("M%1").arg(i + 1)); }
            else if (act) { p.setFont(fnt(MONO, 9, QFont::Medium, 1)); p.setPen(ac); p.drawText(QRectF(r.right() - 80, r.top() + 6, 68, 14), Qt::AlignRight, "ACTIVE"); }
        }
    }
};

// A heading for a group of controls: what the group is, and what it does, in one line.
class SectionLabel : public QWidget {
public:
    SectionLabel(const QString &t, const QString &h, QWidget *p) : QWidget(p), title(t), hint(h) { setFixedHeight(22); }
    QString title, hint; double reveal = 1;
protected:
    void paintEvent(QPaintEvent *) override {
        QPainter p(this); p.setRenderHint(QPainter::Antialiasing); p.setOpacity(reveal);
        QFont tf = fnt(RAJ, 15, QFont::Bold, 2.5); p.setFont(tf); p.setPen(gAccent); p.drawText(QRectF(0, 0, 400, height()), Qt::AlignLeft | Qt::AlignVCenter, title);
        double x = QFontMetricsF(tf).horizontalAdvance(title) + 14; p.setFont(fnt(MONO, 10)); p.setPen(kDim); p.drawText(QRectF(x, 0, width() - x, height()), Qt::AlignLeft | Qt::AlignVCenter, hint);
        p.setPen(QPen(alpha(kText, 0.12), 1)); p.drawLine(QPointF(0, height() - 1), QPointF(width(), height() - 1));
    }
};

// ───────────────────────────── RowsPanel: a Panel whose body is a list of self-painted rows (switch / segmented choice / button / info / text) ─────────────────────────────
struct Row {
    enum Type { Switch, Segment, Button, Info, Text, Gap, Slider } type = Info;
    int val = 0, vmax = 100;                                      // Slider: current / maximum; pick(v) fires when the drag ends
    QString label, sub, value; bool on = false, enabled = true, visible = true, accent = false;
    std::vector<QString> opts; int sel = 0;
    std::function<void()> click; std::function<void(int)> pick; double anim = 0;
};

class RowsPanel : public Panel {
public:
    RowsPanel(const QString &idx, const QString &title, QWidget *p) : Panel(idx, title, p) { setMouseTracking(true); }
    std::vector<Row> rows;
    bool animate(double dt) {                                     // eases the switches; true while anything moves
        bool moving = false;
        for (auto &r : rows) if (r.type == Row::Switch) { double t = r.on ? 1 : 0, o = r.anim; r.anim += (t - r.anim) * ease(dt, 0.06); if (fabs(t - r.anim) < 0.01) r.anim = t; moving |= o != r.anim; }
        return moving;
    }
    void relayout() { int h = 38 + 16; for (size_t i = 0; i < rows.size(); ++i) h += int(rowHeight(rows[i])); setMinimumHeight(h); updateGeometry(); update(); }
protected:
    struct Hit { QRectF r; int row, opt; };
    std::vector<Hit> hits; int hoverRow = -1, hoverOpt = -1, dragRow = -1; QRectF dragTrack;
    void setSliderFromX(double x) { Row &r = rows[dragRow]; double f = qBound(0.0, (x - dragTrack.left()) / qMax(1.0, dragTrack.width()), 1.0); int v = int(f * r.vmax + 0.5); if (v != r.val) { r.val = v; update(); } if (wake) wake(); }
    double bodyW() const { return qMax(200, width() - 38 - 36); }
    double rowHeight(const Row &r) const {
        if (!r.visible) return 0;
        switch (r.type) {
            case Row::Switch: return r.sub.isEmpty() ? 44 : 56;
            case Row::Segment: return (r.label.isEmpty() ? 0 : 22) + (r.sub.isEmpty() ? 0 : 18) + 40;
            case Row::Button: return 46;
            case Row::Info: return 26;
            case Row::Gap: return 12;
            case Row::Slider: return (r.sub.isEmpty() ? 0 : 16) + 52;
            case Row::Text: { QFontMetricsF fm(fnt(MONO, 11)); return fm.boundingRect(QRectF(0, 0, bodyW(), 4000), Qt::TextWordWrap, r.label).height() + 12; }
        }
        return 0;
    }
    void resizeEvent(QResizeEvent *) override { relayout(); }
    void mouseMoveEvent(QMouseEvent *e) override {
        if (dragRow >= 0) { setSliderFromX(e->position().x()); return; }
        int hr = -1, ho = -1; for (auto &h : hits) if (h.r.contains(e->position())) { hr = h.row; ho = h.opt; break; }
        if (hr != hoverRow || ho != hoverOpt) { hoverRow = hr; hoverOpt = ho; setCursor(hr >= 0 ? Qt::PointingHandCursor : Qt::ArrowCursor); update(); if (wake) wake(); }
    }
    void leaveEvent(QEvent *e) override { Panel::leaveEvent(e); hoverRow = hoverOpt = -1; update(); }
    void mousePressEvent(QMouseEvent *e) override {
        for (auto &h : hits) if (h.r.contains(e->position())) {
            Row &r = rows[h.row]; if (!r.enabled) return;
            if (r.type == Row::Slider) { dragRow = h.row; dragTrack = h.r; setSliderFromX(e->position().x()); return; }
            if (r.type == Row::Segment) { if (h.opt != r.sel && r.pick) r.pick(h.opt); }
            else if (r.click) r.click();
            if (wake) wake(); return;
        }
    }
    void mouseReleaseEvent(QMouseEvent *) override { if (dragRow >= 0) { Row &r = rows[dragRow]; dragRow = -1; if (r.pick) r.pick(r.val); update(); } }
    void paintBody(QPainter &p, const QRectF &b) override {
        hits.clear(); double y = b.top() - 4;
        for (size_t i = 0; i < rows.size(); ++i) {
            Row &r = rows[i]; double h = rowHeight(r); if (h <= 0) continue; QRectF rr(b.left(), y, b.width(), h); bool hov = (int) i == hoverRow;
            QColor tc = r.enabled ? kText : alpha(kDim, 0.6);
            switch (r.type) {
                case Row::Switch: {
                    p.setFont(fnt(RAJ, 18, QFont::DemiBold, 1)); p.setPen(tc); p.drawText(QRectF(rr.left(), rr.top() + 6, rr.width() - 110, 22), Qt::AlignLeft | Qt::AlignVCenter, r.label.toUpper());
                    if (!r.sub.isEmpty()) { p.setFont(fnt(MONO, 10)); p.setPen(kDim); p.drawText(QRectF(rr.left(), rr.top() + 29, rr.width() - 110, 16), Qt::AlignLeft | Qt::AlignVCenter, r.sub); }
                    QRectF tr(rr.right() - 52, rr.top() + (h - 24) / 2, 52, 24); QColor ac = r.enabled ? gAccent : kDim;
                    p.setPen(QPen(r.on ? ac : mix(kLine, ac, hov ? 0.8 : 0), 1)); p.setBrush(r.on ? alpha(ac, 0.16 + 0.14 * r.anim) : QBrush(Qt::NoBrush)); p.drawPath(chamfer(tr, 6));
                    QRectF th(tr.left() + 4 + r.anim * (tr.width() - 8 - 16), tr.top() + 4, 16, 16); p.setPen(Qt::NoPen); p.setBrush(r.on ? ac : kDim); p.drawRect(th);
                    p.setFont(fnt(MONO, 9, QFont::Medium, 1)); p.setPen(r.on ? ac : kDim); p.drawText(QRectF(tr.left() - 38, tr.top(), 32, 24), Qt::AlignRight | Qt::AlignVCenter, r.on ? "ON" : "OFF");
                    hits.push_back({rr, (int) i, 0}); break; }
                case Row::Segment: {
                    double yy = rr.top();
                    if (!r.label.isEmpty()) { p.setFont(fnt(RAJ, 18, QFont::DemiBold, 1)); p.setPen(tc); p.drawText(QRectF(rr.left(), yy + 2, rr.width(), 20), Qt::AlignLeft | Qt::AlignVCenter, r.label.toUpper()); yy += 22; }
                    if (!r.sub.isEmpty()) { p.setFont(fnt(MONO, 10)); p.setPen(kDim); p.drawText(QRectF(rr.left(), yy, rr.width(), 16), Qt::AlignLeft | Qt::AlignVCenter, r.sub); yy += 18; }
                    int n = (int) r.opts.size(); double gap = 6, w = (rr.width() - gap * (n - 1)) / n;
                    for (int k = 0; k < n; ++k) {
                        QRectF c(rr.left() + k * (w + gap), yy + 2, w, 32); bool on = k == r.sel, hv = hov && k == hoverOpt;
                        p.setPen(QPen(on ? gAccent : mix(kLine, gAccent, hv ? 0.8 : 0), 1)); p.setBrush(on ? gAccent : QBrush(Qt::NoBrush)); p.drawPath(chamfer(c, 8));
                        p.setFont(fnt(RAJ, 16, QFont::Bold, 1.5)); p.setPen(!r.enabled ? alpha(kDim, 0.6) : on ? kBg : kText); p.drawText(c, Qt::AlignCenter, r.opts[k].toUpper());
                        hits.push_back({c, (int) i, k});
                    }
                    break; }
                case Row::Button: {
                    QRectF c = rr.adjusted(0, 4, 0, -6); bool fill = r.accent;
                    p.setPen(QPen(fill || hov ? gAccent : kLine, 1)); p.setBrush(fill ? gAccent : (hov ? alpha(gAccent, 0.10) : QBrush(Qt::NoBrush))); p.drawPath(chamfer(c, 9));
                    p.setFont(fnt(RAJ, 17, QFont::Bold, 2)); p.setPen(!r.enabled ? alpha(kDim, 0.6) : fill ? kBg : kText); p.drawText(c, Qt::AlignCenter, r.label.toUpper());
                    hits.push_back({c, (int) i, 0}); break; }
                case Row::Info:
                    p.setFont(fnt(MONO, 10, QFont::Normal, 1)); p.setPen(kDim); p.drawText(QRectF(rr.left(), rr.top(), 130, rr.height()), Qt::AlignLeft | Qt::AlignVCenter, r.label.toUpper());
                    p.setFont(fnt(MONO, 11, QFont::Medium)); p.setPen(r.accent ? gAccent : kText); p.drawText(QRectF(rr.left() + 110, rr.top(), rr.width() - 110, rr.height()), Qt::AlignRight | Qt::AlignVCenter, r.value.isEmpty() ? "—" : r.value); break;
                case Row::Text:
                    p.setFont(fnt(MONO, 11)); p.setPen(r.accent ? gAccent : kDim); p.drawText(QRectF(rr.left(), rr.top() + 4, rr.width(), rr.height()), Qt::TextWordWrap, r.label); break;
                case Row::Slider: {
                    double yy = rr.top();
                    p.setFont(fnt(RAJ, 18, QFont::DemiBold, 1)); p.setPen(tc); p.drawText(QRectF(rr.left(), yy + 2, rr.width() - 90, 22), Qt::AlignLeft | Qt::AlignVCenter, r.label.toUpper());
                    p.setFont(fnt(MONO, 12, QFont::Medium)); p.setPen(r.enabled ? gAccent : kDim); p.drawText(QRectF(rr.right() - 90, yy + 2, 90, 22), Qt::AlignRight | Qt::AlignVCenter, r.value.isEmpty() ? QString::number(r.val) : r.value);
                    yy += 24; if (!r.sub.isEmpty()) { p.setFont(fnt(MONO, 10)); p.setPen(kDim); p.drawText(QRectF(rr.left(), yy, rr.width(), 14), Qt::AlignLeft | Qt::AlignVCenter, r.sub); yy += 16; }
                    QRectF bar(rr.left(), yy + 8, rr.width(), 12); double f = r.vmax ? double(r.val) / r.vmax : 0; int n = qBound(20, int(bar.width() / 11), 60);
                    segBar(p, bar, f, n, -1, [&](double) { return r.enabled ? gAccent : kDim; }, 3);
                    if (r.enabled) { double kx = bar.left() + f * bar.width(); p.setPen(QPen(kText, 2)); p.drawLine(QPointF(kx, bar.top() - 4), QPointF(kx, bar.bottom() + 4)); }
                    hits.push_back({bar.adjusted(0, -10, 0, 10), (int) i, 0}); break; }
                case Row::Gap: break;
            }
            if (r.type == Row::Switch && i + 1 < rows.size()) { p.setPen(QPen(alpha(kText, 0.07), 1, Qt::DotLine)); p.drawLine(QPointF(rr.left(), rr.bottom() - 1), QPointF(rr.right(), rr.bottom() - 1)); }
            y += h;
        }
    }
};

} // namespace kit
#endif
