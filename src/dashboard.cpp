#include "dashboard.h"
#include "sensors.h"
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFontDatabase>
#include <QFile>
#include <QDir>
#include <QtMath>
#include <QStyle>
#include <QProcess>
#include <functional>
#include <deque>
#include <vector>
#include <cmath>

namespace {
// ── palette: warm graphite, bone-white text, ONE accent that follows the mode ──
const QColor kBg(14, 15, 17), kPanel(21, 23, 26), kLine(255, 255, 255, 38), kText(233, 230, 223), kDim(139, 143, 150), kHot(255, 70, 50);
const QColor kAccentMode[4] = {QColor(255, 75, 31), QColor(231, 224, 205), QColor(110, 170, 215), QColor(150, 205, 90)};   // turbo, balanced, silent, eco
QColor gAccent = kAccentMode[1], gAccentTarget = kAccentMode[1];
bool gBoost = false; double gClock = 0;           // seconds since show, drives the hazard strip

const char *RAJ = "Rajdhani", *MONO = "IBM Plex Mono";
QFont fnt(const char *fam, int px, int weight = QFont::Normal, qreal ls = 0) {
    QFont f(fam); f.setPixelSize(px); f.setWeight(QFont::Weight(weight)); f.setLetterSpacing(QFont::AbsoluteSpacing, ls); return f;
}
QString readFile(const QString &path) { QFile f(path); if (!f.open(QIODevice::ReadOnly)) return {}; return QString::fromUtf8(f.readAll()).trimmed(); }
qint64 readInt(const QString &path, qint64 def = -1) { bool ok; qint64 v = readFile(path).toLongLong(&ok); return ok ? v : def; }
QColor mix(const QColor &a, const QColor &b, double t) { t = qBound(0.0, t, 1.0); return QColor::fromRgbF(a.redF() + (b.redF() - a.redF()) * t, a.greenF() + (b.greenF() - a.greenF()) * t, a.blueF() + (b.blueF() - a.blueF()) * t, a.alphaF() + (b.alphaF() - a.alphaF()) * t); }
QColor alpha(QColor c, double a) { c.setAlphaF(a); return c; }
double ease(double dt, double tau) { return 1 - exp(-dt / tau); }      // frame-rate independent smoothing factor
double easeOut(double t) { t = qBound(0.0, t, 1.0); return 1 - pow(1 - t, 3); }

// panel outline with two cut corners (top-right, bottom-left) - the one shape the whole UI is built from
QPainterPath chamfer(const QRectF &r, double c) {
    QPainterPath p; p.moveTo(r.left(), r.top()); p.lineTo(r.right() - c, r.top()); p.lineTo(r.right(), r.top() + c); p.lineTo(r.right(), r.bottom());
    p.lineTo(r.left() + c, r.bottom()); p.lineTo(r.left(), r.bottom() - c); p.closeSubpath(); return p;
}

// LED-style segmented meter. `colorAt(v)` colours each lit segment by the value it represents.
void segBar(QPainter &p, const QRectF &r, double frac, int n, double peak, const std::function<QColor(double)> &colorAt, double gap = 3) {
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
QString fmtEta(double hours) { if (hours <= 0 || hours > 48) return {}; int m = int(hours * 60 + 0.5); return QString("%1h %2m").arg(m / 60).arg(m % 60, 2, 10, QChar('0')); }
QColor tempColor(double v) { double c = 30 + v * 70; return c < 72 ? kText : c < 86 ? gAccent : kHot; }
}

// ───────────────────────────── panel base: chamfered frame, index + title, entrance animation ─────────────────────────────
class Panel : public QWidget {
public:
    Panel(const QString &idx, const QString &title, QWidget *p) : QWidget(p), idx(idx), title(title) { setMouseTracking(true); }
    QString idx, title; double reveal = 0, hov = 0, delay = 0; bool hovering = false;
    bool tick(double clock, double dt) { reveal = easeOut((clock - delay) / 0.38); double old = hov; hov += ((hovering ? 1 : 0) - hov) * ease(dt, 0.07); return reveal < 1 || fabs(hov - old) > 0.002; }
protected:
    virtual void paintBody(QPainter &, const QRectF &) = 0;
    void enterEvent(QEnterEvent *) override { hovering = true; }
    void leaveEvent(QEvent *) override { hovering = false; }
    void paintEvent(QPaintEvent *) override {
        if (reveal <= 0.001) return;
        QPainter p(this); p.setRenderHint(QPainter::Antialiasing); p.setOpacity(reveal); p.translate(0, (1 - reveal) * 14);
        QRectF r = QRectF(rect()).adjusted(1, 1, -1, -1); QPainterPath path = chamfer(r, 13);
        p.fillPath(path, kPanel);
        p.setPen(QPen(mix(kLine, alpha(gAccent, 0.8), hov * 0.7), 1)); p.setBrush(Qt::NoBrush); p.drawPath(path);
        p.setPen(QPen(gAccent, 2));                                    // registration ticks on the two square corners
        p.drawLine(r.topLeft() + QPointF(0, 11), r.topLeft()); p.drawLine(r.topLeft(), r.topLeft() + QPointF(11, 0));
        p.drawLine(r.bottomRight() - QPointF(0, 11), r.bottomRight()); p.drawLine(r.bottomRight(), r.bottomRight() - QPointF(11, 0));
        p.setFont(fnt(MONO, 10, QFont::Medium, 1)); p.setPen(gAccent); p.drawText(QRectF(r.left() + 18, r.top() + 12, 28, 14), Qt::AlignLeft | Qt::AlignVCenter, idx);
        p.setFont(fnt(RAJ, 14, QFont::DemiBold, 2.2)); p.setPen(kDim); p.drawText(QRectF(r.left() + 46, r.top() + 11, r.width() - 80, 16), Qt::AlignLeft | Qt::AlignVCenter, title);
        paintBody(p, QRectF(r.left() + 18, r.top() + 38, r.width() - 36, r.height() - 38 - 14));
    }
};

// ───────────────────────────── 01 thermal ─────────────────────────────
class ThermalPanel : public Panel {
public:
    ThermalPanel(QWidget *p) : Panel("01", "THERMAL", p) { setMinimumHeight(272); }
    double cpu = 0, gpu = 0, cpuPeak = 0, gpuPeak = 0, cpuS = 0, gpuS = 0; bool hasGpu = false, gpuOff = false, hasNvme = false; int nvme = 0; QString cpuName, gpuName;
    bool step(double dt) { double a = cpuS, b = gpuS; cpuS += (cpu - cpuS) * ease(dt, 0.12); gpuS += (gpu - gpuS) * ease(dt, 0.12); if (fabs(cpu - cpuS) < 0.05) cpuS = cpu; if (fabs(gpu - gpuS) < 0.05) gpuS = gpu; return a != cpuS || b != gpuS; }
protected:
    void row(QPainter &p, const QRectF &r, const QString &name, const QString &sub, double v, double peak, bool off) {
        p.setFont(fnt(RAJ, 15, QFont::DemiBold, 2)); p.setPen(kText); p.drawText(QRectF(r.left(), r.top(), 80, 18), Qt::AlignLeft, name);
        p.setFont(fnt(MONO, 10, QFont::Normal, 0)); p.setPen(kDim); p.drawText(QRectF(r.left() + 48, r.top() + 2, r.width() - 48, 16), Qt::AlignLeft | Qt::AlignVCenter, sub);
        QString num = off ? QStringLiteral("—") : QString::number(int(v + 0.5));
        p.setFont(fnt(RAJ, 66, QFont::Bold)); p.setPen(off ? kDim : tempColor((v - 30) / 70)); p.drawText(QRectF(r.left() - 2, r.top() + 12, 120, 70), Qt::AlignLeft | Qt::AlignVCenter, num);
        if (!off) { p.setFont(fnt(RAJ, 20, QFont::Medium)); p.setPen(kDim); p.drawText(QRectF(r.left() + 84, r.top() + 26, 40, 24), Qt::AlignLeft | Qt::AlignTop, "°C"); }
        else { p.setFont(fnt(MONO, 10, QFont::Medium, 1)); p.setPen(kDim); p.drawText(QRectF(r.left() + 52, r.top() + 44, 80, 16), Qt::AlignLeft, "ASLEEP"); }
        QRectF bar(r.left() + 130, r.top() + 36, r.width() - 130, 16);
        segBar(p, bar, off ? 0 : (v - 30) / 70, 40, off ? -1 : (peak - 30) / 70, [](double f) { return tempColor(f); }, 2.5);
        p.setFont(fnt(MONO, 9, QFont::Normal, 0)); p.setPen(kDim);
        for (int t = 40; t <= 100; t += 20) { double x = bar.left() + (t - 30) / 70.0 * bar.width(); p.drawText(QRectF(x - 16, bar.bottom() + 8, 32, 12), Qt::AlignCenter, QString::number(t)); }
        if (!off) { p.setFont(fnt(MONO, 9, QFont::Medium, 1)); p.setPen(gAccent); p.drawText(QRectF(bar.right() - 90, bar.top() - 20, 90, 12), Qt::AlignRight, QString("PEAK %1°").arg(int(peak + 0.5))); }
    }
    void paintBody(QPainter &p, const QRectF &b) override {
        int rows = hasGpu ? 2 : 1; double rh = b.height() / rows;
        row(p, QRectF(b.left(), b.top(), b.width(), rh), "CPU", cpuName, cpuS, cpuPeak, false);
        if (hasGpu) {
            p.setPen(QPen(kLine, 1, Qt::DotLine)); p.drawLine(QPointF(b.left(), b.top() + rh - 2), QPointF(b.right(), b.top() + rh - 2));
            row(p, QRectF(b.left(), b.top() + rh, b.width(), rh), "GPU", gpuName, gpuS, gpuPeak, gpuOff);
        } else if (hasNvme) {
            p.setFont(fnt(MONO, 10, QFont::Normal, 0)); p.setPen(kDim); p.drawText(QRectF(b.left(), b.bottom() - 14, b.width(), 14), Qt::AlignLeft, QString("NVMe  %1°C").arg(nvme));
        }
    }
};

// ───────────────────────────── 02 cooling ─────────────────────────────
class FanPanel : public Panel {
public:
    FanPanel(QWidget *p) : Panel("02", "COOLING", p) { setMinimumHeight(272); }
    int pct[2] = {-1, -1}, rpm[2] = {0, 0}; double shown[2] = {0, 0}, speed[2] = {0, 0}, rot[2] = {0, 0}; bool two = false, boost = false, boostOk = true;
    std::function<void(bool)> onBoost;
    // Rotation is integrated from real elapsed time (deg/s), so it is perfectly smooth whatever the timer does.
    // Returns true when the bar (not just the glyph) changed and the whole panel needs repainting.
    bool step(double dt) {
        bool bars = false;
        for (int i = 0; i < 2; ++i) {
            double before = shown[i]; shown[i] += (qMax(pct[i], 0) - shown[i]) * ease(dt, 0.35); if (fabs(shown[i] - qMax(pct[i], 0)) < 0.05) shown[i] = qMax(pct[i], 0); bars |= fabs(shown[i] - before) > 0.0;
            double want = shown[i] < 1 ? 0 : 60 + shown[i] * 6.0;          // deg/s: ~60 at idle .. ~660 at full tilt (below the wagon-wheel limit for 7 blades)
            speed[i] += (want - speed[i]) * ease(dt, 0.9);                   // inertia: spins up and coasts down over ~a second
            rot[i] = fmod(rot[i] + speed[i] * dt, 360.0);
        }
        return bars;
    }
    QRegion glyphRegion() const { QRegion r; for (int i = 0; i < (two ? 2 : 1); ++i) r += QRect(int(19 + 30 - 33), int(39 + (two ? 0 : 6) + i * 96 + 36 - 33), 66, 66); return r; }
protected:
    QRectF btn;
    void mousePressEvent(QMouseEvent *e) override { if (boostOk && btn.contains(e->position()) && onBoost) onBoost(!boost); }
    void mouseMoveEvent(QMouseEvent *e) override { setCursor(boostOk && btn.contains(e->position()) ? Qt::PointingHandCursor : Qt::ArrowCursor); }
    void glyph(QPainter &p, QPointF c, double R, double rot, const QColor &col) {
        p.save(); p.translate(c); p.setPen(QPen(alpha(kText, 0.25), 1.2)); p.setBrush(Qt::NoBrush); p.drawEllipse(QPointF(0, 0), R, R);
        p.rotate(rot); p.setPen(Qt::NoPen); p.setBrush(col);
        for (int i = 0; i < 7; ++i) {                      // flat, geometric blades: a swept wedge, not a glowing flower
            QPainterPath b; b.moveTo(R * 0.14, 0); b.lineTo(R * 0.92, -R * 0.06); b.quadTo(R * 0.98, R * 0.34, R * 0.52, R * 0.5); b.quadTo(R * 0.40, R * 0.2, R * 0.14, R * 0.12); b.closeSubpath();
            p.drawPath(b); p.rotate(360.0 / 7);
        }
        p.restore(); p.setBrush(kPanel); p.setPen(QPen(col, 1.5)); p.drawEllipse(c, R * 0.15, R * 0.15);
    }
    void cell(QPainter &p, const QRectF &r, int i, const QString &name) {
        QColor col = pct[i] < 0 ? kDim : mix(kText, gAccent, shown[i] / 100.0);
        glyph(p, QPointF(r.left() + 30, r.top() + 36), 27, rot[i], alpha(col, 0.92));
        p.setFont(fnt(RAJ, 15, QFont::DemiBold, 2)); p.setPen(kText); p.drawText(QRectF(r.left() + 72, r.top() - 2, 100, 18), Qt::AlignLeft, name);
        p.setFont(fnt(RAJ, 40, QFont::Bold)); p.setPen(pct[i] < 0 ? kDim : kText);
        p.drawText(QRectF(r.left() + 70, r.top() + 12, r.width() - 70, 44), Qt::AlignLeft | Qt::AlignVCenter, rpm[i] > 0 ? QString::number(rpm[i]) : (pct[i] > 0 ? QString("%1%").arg(pct[i]) : "—"));
        p.setFont(fnt(MONO, 10, QFont::Medium, 1)); p.setPen(kDim); p.drawText(QRectF(r.left() + 72, r.top() + 54, 170, 14), Qt::AlignLeft, rpm[i] > 0 ? QString("RPM · %1 % PWM").arg(qMax(pct[i], 0)) : "PWM");
        segBar(p, QRectF(r.left(), r.top() + 76, r.width(), 8), qMax(pct[i], 0) / 100.0, 20, -1, [col](double) { return col; }, 2.5);
    }
    void paintBody(QPainter &p, const QRectF &b) override {
        double h = 92;
        if (two) { cell(p, QRectF(b.left(), b.top(), b.width(), h), 0, "CPU FAN"); cell(p, QRectF(b.left(), b.top() + h + 4, b.width(), h), 1, "GPU FAN"); }
        else cell(p, QRectF(b.left(), b.top() + 6, b.width(), h), 0, "FAN");
        if (!boostOk) { btn = QRectF(); return; }
        btn = QRectF(b.left(), b.bottom() - 28, b.width(), 28); QPainterPath bp = chamfer(btn, 7);
        p.setPen(QPen(boost ? gAccent : kLine, 1)); p.setBrush(boost ? alpha(gAccent, 0.18) : Qt::NoBrush); p.drawPath(bp);
        p.setFont(fnt(RAJ, 14, QFont::DemiBold, 2)); p.setPen(boost ? gAccent : kText); p.drawText(btn.adjusted(12, 0, 0, 0), Qt::AlignLeft | Qt::AlignVCenter, "COOLER BOOST");
        p.setFont(fnt(MONO, 10, QFont::Medium, 1)); p.setPen(boost ? gAccent : kDim); p.drawText(btn.adjusted(0, 0, -12, 0), Qt::AlignRight | Qt::AlignVCenter, boost ? "ON" : "OFF");
    }
};

// ───────────────────────────── 03 power ─────────────────────────────
class PowerPanel : public Panel {
public:
    PowerPanel(QWidget *p) : Panel("03", "POWER", p) { setMinimumHeight(272); }
    int cap = 0, limit = 100; double shown = 0; bool charging = false, limitOk = true; QString state, draw, eta, health, cycles; bool ac = false;
    std::function<void(int)> onLimit;
    bool step(double dt) { double a = shown; shown += (cap - shown) * ease(dt, 0.4); if (fabs(cap - shown) < 0.05) shown = cap; return a != shown; }
protected:
    QRectF chips[3];
    void mousePressEvent(QMouseEvent *e) override { if (!limitOk) return; const int v[3] = {60, 80, 100}; for (int i = 0; i < 3; ++i) if (chips[i].contains(e->position()) && onLimit) onLimit(v[i]); }
    void mouseMoveEvent(QMouseEvent *e) override { bool on = false; if (limitOk) for (auto &c : chips) on |= c.contains(e->position()); setCursor(on ? Qt::PointingHandCursor : Qt::ArrowCursor); }
    void paintBody(QPainter &p, const QRectF &b) override {
        QColor c = shown < 15 ? kHot : kText;
        p.setFont(fnt(RAJ, 66, QFont::Bold)); p.setPen(c); p.drawText(QRectF(b.left() - 2, b.top() - 4, 110, 70), Qt::AlignLeft | Qt::AlignVCenter, QString::number(int(shown + 0.5)));
        p.setFont(fnt(RAJ, 20, QFont::Medium)); p.setPen(kDim); p.drawText(QRectF(b.left() + 84, b.top() + 8, 30, 24), Qt::AlignLeft | Qt::AlignTop, "%");
        p.setFont(fnt(RAJ, 15, QFont::DemiBold, 2)); p.setPen(charging ? gAccent : kText); p.drawText(QRectF(b.left() + 120, b.top() + 6, b.width() - 120, 18), Qt::AlignRight, state);
        p.setFont(fnt(MONO, 10, QFont::Normal, 0)); p.setPen(kDim); p.drawText(QRectF(b.left() + 120, b.top() + 26, b.width() - 120, 14), Qt::AlignRight, ac ? "AC connected" : "on battery");
        QRectF bar(b.left(), b.top() + 66, b.width(), 14);
        bool low = shown < 15; segBar(p, bar, shown / 100.0, 25, -1, [low](double) { return low ? kHot : kText; }, 2.5);
        if (limit < 100) {                                                  // notch where the charge limit stops
            double x = bar.left() + limit / 100.0 * bar.width();
            p.setPen(QPen(gAccent, 2)); p.drawLine(QPointF(x, bar.top() - 7), QPointF(x, bar.bottom() + 7));
            p.setFont(fnt(MONO, 9, QFont::Medium, 1)); p.setPen(gAccent); p.drawText(QRectF(x - 40, bar.bottom() + 8, 80, 12), Qt::AlignCenter, QString("LIMIT %1").arg(limit));
        }
        double y = bar.bottom() + 28; const std::pair<const char *, QString> kv[] = {{"DRAW", draw}, {"TIME", eta}, {"HEALTH", health}, {"CYCLES", cycles}};
        for (auto &e : kv) {
            p.setFont(fnt(MONO, 10, QFont::Normal, 1)); p.setPen(kDim); p.drawText(QRectF(b.left(), y, 80, 14), Qt::AlignLeft, e.first);
            p.setFont(fnt(MONO, 11, QFont::Medium, 0)); p.setPen(kText); p.drawText(QRectF(b.left() + 70, y, b.width() - 70, 14), Qt::AlignRight, e.second.isEmpty() ? "—" : e.second); y += 17;
        }
        if (limitOk) {
            double w = (b.width() - 12) / 3; const int v[3] = {60, 80, 100};
            for (int i = 0; i < 3; ++i) {
                chips[i] = QRectF(b.left() + i * (w + 6), b.bottom() - 24, w, 24); bool on = (i == 0 && limit <= 60) || (i == 1 && limit > 60 && limit <= 80) || (i == 2 && limit > 80);
                p.setPen(QPen(on ? gAccent : kLine, 1)); p.setBrush(on ? gAccent : Qt::NoBrush); p.drawPath(chamfer(chips[i], 6));
                p.setFont(fnt(RAJ, 14, QFont::DemiBold, 1)); p.setPen(on ? kBg : kText); p.drawText(chips[i], Qt::AlignCenter, QString("%1%").arg(v[i]));
            }
        }
    }
};

// ───────────────────────────── 04 history graph ─────────────────────────────
class GraphPanel : public Panel {
public:
    GraphPanel(QWidget *p) : Panel("04", "TELEMETRY · LAST 3 MIN", p) { setMinimumHeight(150); }
    void push(double cpu, double gpu, double watts) { for (auto *d : {&c, &g, &w}) if (d->size() >= N) d->pop_front(); c.push_back(cpu); g.push_back(gpu); w.push_back(watts); }
    bool hasGpu = false;
protected:
    double hoverX = -1;
    void mouseMoveEvent(QMouseEvent *e) override { hoverX = e->position().x(); }
    void leaveEvent(QEvent *e) override { Panel::leaveEvent(e); hoverX = -1; }
    void paintBody(QPainter &p, const QRectF &b) override {
        QRectF plot(b.left() + 34, b.top() + 4, b.width() - 34 - 44, b.height() - 4 - 16);
        p.setFont(fnt(MONO, 9, QFont::Normal, 0));
        for (int t = 40; t <= 100; t += 20) { double y = plot.bottom() - (t - 30) / 70.0 * plot.height(); p.setPen(QPen(alpha(kText, 0.08), 1, Qt::DashLine)); p.drawLine(QPointF(plot.left(), y), QPointF(plot.right(), y)); p.setPen(kDim); p.drawText(QRectF(b.left(), y - 7, 30, 14), Qt::AlignRight | Qt::AlignVCenter, QString::number(t)); }
        double wmax = 30; for (double v : w) wmax = qMax(wmax, v); wmax = ceil(wmax / 10) * 10;
        p.setPen(kDim); p.drawText(QRectF(plot.right() + 6, plot.top() - 6, 40, 12), Qt::AlignLeft, QString("%1W").arg(int(wmax)));
        p.drawText(QRectF(plot.left(), plot.bottom() + 3, 60, 12), Qt::AlignLeft, "-3:00"); p.drawText(QRectF(plot.right() - 40, plot.bottom() + 3, 40, 12), Qt::AlignRight, "now");
        double dx = plot.width() / (N - 1);
        auto path = [&](const std::deque<double> &d, double lo, double hi, QPainterPath *fill) {
            QPainterPath pa; bool open = false; double x0 = plot.right() - dx * (d.size() - 1); double lx = 0;
            for (size_t i = 0; i < d.size(); ++i) {
                if (d[i] < 0) { open = false; continue; }
                QPointF pt(x0 + i * dx, plot.bottom() - qBound(0.0, (d[i] - lo) / (hi - lo), 1.0) * plot.height());
                if (!open) { pa.moveTo(pt); open = true; } else pa.lineTo(pt); lx = pt.x();
            }
            if (fill && open) { *fill = pa; fill->lineTo(lx, plot.bottom()); fill->lineTo(x0, plot.bottom()); fill->closeSubpath(); }
            return pa;
        };
        QPainterPath area; QPainterPath cp = path(c, 30, 100, &area);
        p.setPen(Qt::NoPen); p.setBrush(alpha(gAccent, 0.10)); p.drawPath(area);
        p.setBrush(Qt::NoBrush);
        p.setPen(QPen(alpha(kText, 0.45), 1.3, Qt::SolidLine)); p.drawPath(path(w, 0, wmax, nullptr));
        if (hasGpu) { p.setPen(QPen(kText, 1.3, Qt::DashLine)); p.drawPath(path(g, 30, 100, nullptr)); }
        p.setPen(QPen(gAccent, 1.8, Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin)); p.drawPath(cp);
        p.setFont(fnt(MONO, 10, QFont::Medium, 1)); double x = b.right() - 240;
        struct K { QColor c; Qt::PenStyle s; QString n; bool on; } ks[] = {{gAccent, Qt::SolidLine, "CPU", true}, {kText, Qt::DashLine, "GPU", hasGpu}, {alpha(kText, 0.45), Qt::SolidLine, "WATTS", true}};
        for (auto &k : ks) { if (!k.on) continue; p.setPen(QPen(k.c, 2, k.s)); p.drawLine(QPointF(x, b.top() - 21), QPointF(x + 16, b.top() - 21)); p.setPen(kDim); p.drawText(QRectF(x + 22, b.top() - 28, 60, 14), Qt::AlignLeft | Qt::AlignVCenter, k.n); x += 78; }
        if (hoverX >= plot.left() && hoverX <= plot.right() && !c.empty()) {      // crosshair read-out
            int back = int(round((plot.right() - hoverX) / dx)); int idx = int(c.size()) - 1 - back;
            if (idx >= 0 && idx < int(c.size())) {
                double xx = plot.right() - back * dx; p.setPen(QPen(alpha(kText, 0.5), 1)); p.drawLine(QPointF(xx, plot.top()), QPointF(xx, plot.bottom()));
                QString t = QString("-%1s   CPU %2°").arg(back).arg(int(c[idx])); if (hasGpu && g[idx] > 0) t += QString("   GPU %1°").arg(int(g[idx])); t += QString("   %1 W").arg(w[idx], 0, 'f', 1);
                QFont f = fnt(MONO, 10, QFont::Medium, 0); p.setFont(f); QRectF tr(0, 0, QFontMetricsF(f).horizontalAdvance(t) + 16, 20); tr.moveTopLeft(QPointF(qBound(plot.left(), xx - tr.width() / 2, plot.right() - tr.width()), plot.top() + 2));
                p.setPen(QPen(kLine, 1)); p.setBrush(kBg); p.drawRect(tr); p.setPen(kText); p.drawText(tr, Qt::AlignCenter, t);
            }
        }
    }
private:
    static constexpr int N = 180; std::deque<double> c, g, w;
};

// ───────────────────────────── tile strip (modes / presets) ─────────────────────────────
class TileStrip : public QWidget {
public:
    struct Item { QString label, sub; QColor accent; bool enabled = true; bool visible = true; };
    TileStrip(bool checkable, QWidget *p) : QWidget(p), checkable(checkable) { setMouseTracking(true); setFixedHeight(checkable ? 62 : 50); }
    std::vector<Item> items; int checked = -1; bool controls = false; double delay = 0, reveal = 0; std::function<void(int)> onClick;
    std::vector<double> hov, press;
    void setItems(std::vector<Item> v) { items = std::move(v); hov.assign(items.size(), 0); press.assign(items.size(), 0); update(); }
    bool tick(double clock, double dt) { reveal = easeOut((clock - delay) / 0.38); bool act = reveal < 1; for (size_t i = 0; i < items.size(); ++i) { double h = hov[i]; hov[i] += (((int) i == hot ? 1 : 0) - hov[i]) * ease(dt, 0.07); press[i] *= exp(-dt / 0.1); act |= fabs(hov[i] - h) > 0.002 || press[i] > 0.01; } return act; }
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
            p.setPen(QPen(on ? ac : mix(kLine, alpha(ac, 0.9), hov[i]), 1)); p.setBrush(on ? ac : alpha(ac, 0.06 * hov[i])); p.drawPath(path);
            QColor tc = !it.enabled ? alpha(kDim, 0.5) : on ? kBg : kText;
            p.setFont(fnt(RAJ, checkable ? 21 : 17, QFont::Bold, 1.5)); p.setPen(tc); p.drawText(QRectF(r.left() + 14, r.top() + (checkable ? 7 : 5), r.width() - 28, 24), Qt::AlignLeft | Qt::AlignVCenter, it.label.toUpper());
            p.setFont(fnt(MONO, 10, QFont::Normal, 0)); p.setPen(on ? alpha(kBg, 0.7) : kDim); p.drawText(QRectF(r.left() + 14, r.top() + (checkable ? 32 : 26), r.width() - 28, 16), Qt::AlignLeft | Qt::AlignVCenter, it.sub);
            if (checkable) { p.setFont(fnt(MONO, 10, QFont::Medium, 1)); p.setPen(on ? alpha(kBg, 0.8) : alpha(kDim, 0.7)); p.drawText(QRectF(r.right() - 38, r.top() + 8, 28, 14), Qt::AlignRight, QString("M%1").arg(i + 1)); }
        }
    }
};

// ───────────────────────────── header + problem banner ─────────────────────────────
class HeaderBar : public QWidget {
public:
    HeaderBar(QWidget *p) : QWidget(p) { setFixedHeight(66); }
    QString title, sub, left, right; bool leftOk = true, rightOk = true; double reveal = 0, delay = 0;
    bool tick(double clock, double) { reveal = easeOut((clock - delay) / 0.4); return reveal < 1 || gBoost; }
protected:
    void paintEvent(QPaintEvent *) override {
        if (reveal <= 0.001) return; QPainter p(this); p.setRenderHint(QPainter::Antialiasing); p.setOpacity(reveal);
        p.setPen(Qt::NoPen); p.setBrush(gAccent); p.drawRect(QRectF(0, 8, 5, 38));
        p.setFont(fnt(RAJ, 34, QFont::Bold, 3)); p.setPen(kText); p.drawText(QRectF(16, 0, width() - 300, 40), Qt::AlignLeft | Qt::AlignVCenter, title);
        p.setFont(fnt(MONO, 10, QFont::Normal, 0.5)); p.setPen(kDim); p.drawText(QRectF(17, 38, width() - 300, 14), Qt::AlignLeft | Qt::AlignVCenter, sub);
        p.setFont(fnt(MONO, 11, QFont::Medium, 1)); double x = width();
        for (auto *s : {&right, &left}) {
            if (s->isEmpty()) continue; double w = QFontMetricsF(p.font()).horizontalAdvance(*s) + 26; x -= w;
            bool ok = (s == &right) ? rightOk : leftOk; p.setPen(Qt::NoPen); p.setBrush(ok ? kText : gAccent); p.drawRect(QRectF(x, 25, 7, 7));
            p.setPen(kText); p.drawText(QRectF(x + 16, 16, w, 24), Qt::AlignLeft | Qt::AlignVCenter, *s); x -= 14;
        }
        if (gBoost) {                                                  // hazard strip while Cooler Boost is on
            QRectF h(0, height() - 8, width(), 6); p.setClipRect(h); p.setPen(Qt::NoPen); p.setBrush(gAccent); p.drawRect(h);
            p.setBrush(kBg); double off = fmod(gClock * 24, 22);
            for (double sx = -22 + off; sx < width() + 22; sx += 22) { QPolygonF q; q << QPointF(sx, h.bottom()) << QPointF(sx + 6, h.top()) << QPointF(sx + 17, h.top()) << QPointF(sx + 11, h.bottom()); p.drawPolygon(q); }
        } else { p.setPen(QPen(kLine, 1)); p.drawLine(QPointF(0, height() - 4), QPointF(width(), height() - 4)); }
    }
};

class BannerBar : public QWidget {
public:
    BannerBar(QWidget *p) : QWidget(p) { setFixedHeight(54); hide(); setMouseTracking(true); }
    QString text, button; std::function<void()> onClick; QRectF btn;
protected:
    void mousePressEvent(QMouseEvent *e) override { if (btn.contains(e->position()) && onClick) onClick(); }
    void mouseMoveEvent(QMouseEvent *e) override { setCursor(btn.contains(e->position()) ? Qt::PointingHandCursor : Qt::ArrowCursor); }
    void paintEvent(QPaintEvent *) override {
        QPainter p(this); p.setRenderHint(QPainter::Antialiasing); QRectF r = QRectF(rect()).adjusted(1, 1, -1, -1);
        p.setPen(QPen(gAccent, 1)); p.setBrush(alpha(gAccent, 0.10)); p.drawPath(chamfer(r, 11));
        p.setFont(fnt(RAJ, 17, QFont::DemiBold, 1)); p.setPen(kText); p.drawText(QRectF(18, 0, width() - 240, height()), Qt::AlignLeft | Qt::AlignVCenter | Qt::TextWordWrap, text);
        if (!button.isEmpty()) {
            btn = QRectF(width() - 200, 11, 184, 32); p.setPen(Qt::NoPen); p.setBrush(gAccent); p.drawPath(chamfer(btn, 8));
            p.setFont(fnt(RAJ, 16, QFont::Bold, 2)); p.setPen(kBg); p.drawText(btn, Qt::AlignCenter, button.toUpper());
        } else btn = QRectF();
    }
};

// ───────────────────────────── page ─────────────────────────────
DashboardPage::DashboardPage(QWidget *parent) : QWidget(parent) {
    setObjectName("dashboard");
    static bool fontsLoaded = false;
    if (!fontsLoaded) { fontsLoaded = true; for (const char *f : {"Rajdhani-Medium", "Rajdhani-SemiBold", "Rajdhani-Bold", "IBMPlexMono-Regular", "IBMPlexMono-Medium"}) QFontDatabase::addApplicationFont(QString(":/fonts/%1.ttf").arg(f)); }

    auto *root = new QVBoxLayout(this); root->setContentsMargins(18, 12, 18, 14); root->setSpacing(10);
    header = new HeaderBar(this); root->addWidget(header);
    banner = new BannerBar(this); root->addWidget(banner);
    banner->onClick = [this] { emit fixRequested(); };

    thermal = new ThermalPanel(this); fans = new FanPanel(this); power = new PowerPanel(this); graph = new GraphPanel(this);
    auto *row1 = new QHBoxLayout; row1->setSpacing(10); row1->addWidget(thermal, 5); row1->addWidget(fans, 3); row1->addWidget(power, 4); root->addLayout(row1, 5);
    root->addWidget(graph, 3);
    modes = new TileStrip(true, this); presets = new TileStrip(false, this);
    root->addWidget(modes); root->addWidget(presets);
    header->delay = 0; thermal->delay = 0.08; fans->delay = 0.16; power->delay = 0.24; graph->delay = 0.34; modes->delay = 0.44; presets->delay = 0.52;

    modes->setItems({{"Turbo", "max performance", kAccentMode[0]}, {"Balanced", "comfort", kAccentMode[1]}, {"Silent", "quiet fans", kAccentMode[2]}, {"Eco", "lowest power", kAccentMode[3]}});
    modes->onClick = [this](int i) { modes->checked = i; cur.mode = i; retargetAccent(); emit modeRequested(i); };
    presets->setItems({{"Gaming", "turbo + cooler boost", kText}, {"Work", "balanced, fans auto", kText}, {"Night", "silent, fans auto", kText}, {"Travel", "eco + 80% charge cap", kText}});
    presets->onClick = [this](int i) { applyPreset(i); };
    fans->onBoost = [this](bool on) { fans->boost = on; gBoost = on; emit coolerBoostRequested(on); };
    power->onLimit = [this](int v) { power->limit = v; emit chargeLimitRequested(v); };

    connect(&fastTimer, &QTimer::timeout, this, &DashboardPage::animate); fastTimer.setInterval(16); fastTimer.setTimerType(Qt::PreciseTimer);
    setControlsEnabled(false);
}

void DashboardPage::setCaps(const Caps &c) {
    caps = c;
    header->title = (c.model.isEmpty() ? QString("MSI LAPTOP") : c.model.left(c.model.indexOf(" D2") > 0 ? c.model.indexOf(" D2") : c.model.size())).toUpper();
    header->sub = QString("%1  ·  EC %2").arg(c.board.isEmpty() ? "—" : c.board, c.fw.isEmpty() ? "n/a" : c.fw);
    thermal->hasGpu = c.gpuTemp; thermal->cpuName = c.cpuName; thermal->gpuName = c.gpuName; graph->hasGpu = c.gpuTemp;
    fans->two = c.fan2; fans->boostOk = c.coolerBoost; power->limitOk = c.chargeLimit;
    for (int i = 0; i < 4; ++i) modes->items[i].visible = c.modeOk[i];
    modes->update(); header->update();
}

void DashboardPage::setProblem(const QString &text, const QString &button) { banner->text = text; banner->button = button; banner->setVisible(!text.isEmpty()); banner->update(); }

void DashboardPage::setControlsEnabled(bool en) { controlsEnabled = en; modes->controls = en; presets->controls = en; for (auto &i : modes->items) i.enabled = en; for (auto &i : presets->items) i.enabled = en; }

void DashboardPage::showEvent(QShowEvent *) { sinceShow.start(); gClock = 0; fastTimer.start(); }   // restarts the entrance animation each time the window opens
void DashboardPage::hideEvent(QHideEvent *) { fastTimer.stop(); }                               // hidden in the tray = zero CPU

void DashboardPage::retargetAccent() { gAccentTarget = cur.mode >= 0 ? kAccentMode[cur.mode] : kAccentMode[1]; }

void DashboardPage::applyPreset(int idx) {
    int mode = 1; bool boost = false; int limit = -1;
    if (idx == 0) { mode = 0; boost = true; } else if (idx == 2) mode = 2; else if (idx == 3) { mode = 3; limit = 80; }
    if (!caps.modeOk[mode]) mode = 1;
    modes->checked = mode; cur.mode = mode; retargetAccent(); emit modeRequested(mode);
    if (caps.coolerBoost && boost != cur.coolerBoost) { fans->boost = boost; gBoost = boost; cur.coolerBoost = boost; emit coolerBoostRequested(boost); }
    if (limit > 0 && caps.chargeLimit) { power->limit = limit; emit chargeLimitRequested(limit); }
}

void DashboardPage::setData(const DashData &d) {
    cur = d;
    thermal->cpu = d.cpuTemp; thermal->gpu = qMax(d.gpuTemp, 0); thermal->gpuOff = d.gpuTemp == 0;
    fans->pct[0] = d.cpuFanPct; fans->pct[1] = d.gpuFanPct; fans->rpm[0] = d.cpuFanRpm; fans->rpm[1] = d.gpuFanRpm; fans->boost = d.coolerBoost; gBoost = d.coolerBoost;
    modes->checked = d.mode; power->limit = d.chargeLimit; retargetAccent();
}

void DashboardPage::animate() {
    double now = sinceShow.elapsed() / 1000.0, rawDt = now - gClock, dt = qBound(0.001, rawDt, 0.05); gClock = now;
    static const bool frameLog = qEnvironmentVariableIsSet("MCC_FRAMELOG"); if (frameLog && rawDt > 0.030) fprintf(stderr, "[frame] gap %.0f ms at t=%.2f\n", rawDt * 1000, now);
    QColor before = gAccent; gAccent = mix(gAccent, gAccentTarget, ease(dt, 0.12));      // accent cross-fades when the mode changes
    bool accentMoved = qAbs(before.red() - gAccent.red()) + qAbs(before.green() - gAccent.green()) + qAbs(before.blue() - gAccent.blue()) > 0;
    if (accentMoved) for (QWidget *w : std::initializer_list<QWidget *>{header, thermal, fans, power, graph, modes, presets}) w->update();
    // Only repaint what actually changed: panels idle at zero cost, the fan glyphs get a tiny partial repaint at 60 fps.
    if (thermal->tick(now, dt) | thermal->step(dt)) thermal->update();
    if (power->tick(now, dt) | power->step(dt)) power->update();
    if (graph->tick(now, dt)) graph->update();
    bool fanFull = fans->tick(now, dt); fanFull |= fans->step(dt);
    if (fanFull) fans->update(); else fans->update(fans->glyphRegion());
    if (header->tick(now, dt)) header->update();
    if (modes->tick(now, dt)) modes->update();
    if (presets->tick(now, dt)) presets->update();
}

void DashboardPage::setRpm(int cpuRpm, int gpuRpm) { fans->rpm[0] = cpuRpm; fans->rpm[1] = gpuRpm; }

// Everything the UI shows from sysfs arrives here, already read on the sensor thread: this function only formats and assigns (microseconds).
void DashboardPage::applySnapshot(const Snapshot &s) {
    if (s.msiEc) { int r0 = fans->rpm[0], r1 = fans->rpm[1]; setData(s.d); fans->rpm[0] = r0; fans->rpm[1] = r1; }
    else cur.chargeLimit = s.d.chargeLimit;
    double hours = 0, watts = s.volts * s.currentUa / 1e6;
    bool charging = s.status == "Charging", dis = s.status == "Discharging";
    if (s.currentUa > 1000) hours = dis ? s.chargeNow / s.currentUa : charging ? (s.chargeFull - s.chargeNow) / s.currentUa : 0;
    power->cap = s.cap; power->charging = charging; power->ac = s.ac || charging;
    power->state = charging ? "CHARGING" : dis ? "DISCHARGING" : s.status == "Full" ? "FULL" : s.status == "Not charging" ? "HOLDING AT LIMIT" : s.status.toUpper();
    power->draw = (dis || charging) ? QString("%1 W %2").arg(watts, 0, 'f', 1).arg(charging ? "in" : "out") : "idle";
    power->eta = fmtEta(hours).isEmpty() ? QString() : fmtEta(hours) + (dis ? " left" : " to full");
    power->health = s.chargeDesign > 0 ? QString("%1 %  ·  %2 / %3 Wh").arg(int(100.0 * s.chargeFull / s.chargeDesign + 0.5)).arg(s.chargeFull * s.volts / 1e6, 0, 'f', 0).arg(s.chargeDesign * s.volts / 1e6, 0, 'f', 0) : QString();
    power->cycles = s.cycles > 0 ? QString::number(s.cycles) : QStringLiteral("not reported");
    header->left = power->ac ? "AC POWER" : "BATTERY"; header->leftOk = power->ac;
    if (caps.gpuTemp && !s.dgpu.isEmpty()) { header->right = s.dgpu.startsWith("D3") ? "dGPU ASLEEP" : "dGPU ACTIVE"; header->rightOk = s.dgpu.startsWith("D3"); } else header->right.clear();
    thermal->hasNvme = s.nvme > 0; thermal->nvme = s.nvme;
    thermal->cpuPeak = qMax<double>(cur.cpuTemp, thermal->cpuPeak - 0.6); thermal->gpuPeak = qMax<double>(cur.gpuTemp, thermal->gpuPeak - 0.6);
    graph->push(cur.cpuTemp > 0 ? cur.cpuTemp : -1, cur.gpuTemp > 0 ? cur.gpuTemp : -1, dis ? watts : 0);
    header->sub = QString("%1  ·  EC %2  ·  CPU %3 · %4 GHz · RAM %5/%6 GB").arg(caps.board.isEmpty() ? "—" : caps.board, caps.fw.isEmpty() ? "n/a" : caps.fw, s.load.isEmpty() ? "—" : s.load).arg(s.ghz, 0, 'f', 1).arg(s.memUsedGb, 0, 'f', 1).arg(s.memTotalGb, 0, 'f', 0);
    for (QWidget *w : std::initializer_list<QWidget *>{header, thermal, power, graph, fans}) w->update();     // new text/values once a second
}
