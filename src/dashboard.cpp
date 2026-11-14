#include "dashboard.h"
#include <QPainter>
#include <QPainterPath>
#include <QConicalGradient>
#include <QLinearGradient>
#include <QRadialGradient>
#include <QAbstractButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QFile>
#include <QDir>
#include <QtMath>
#include <QStyle>
#include <QMouseEvent>
#include <cmath>

namespace {
const QColor kBg(10, 14, 23), kCard(18, 26, 43), kEdge(35, 50, 82), kText(215, 227, 255), kDim(127, 146, 184);
const QColor kCyan(25, 227, 255), kGreen(61, 255, 154), kAmber(255, 176, 32), kRed(255, 69, 96), kMagenta(255, 43, 214), kViolet(150, 110, 255);

QString readFile(const QString &path) {
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) return {};
    return QString::fromUtf8(f.readAll()).trimmed();
}
qint64 readInt(const QString &path, qint64 def = -1) {
    bool ok; qint64 v = readFile(path).toLongLong(&ok); return ok ? v : def;
}
QColor lerp(const QColor &a, const QColor &b, double t) {
    t = qBound(0.0, t, 1.0);
    return QColor::fromRgbF(a.redF() + (b.redF() - a.redF()) * t, a.greenF() + (b.greenF() - a.greenF()) * t, a.blueF() + (b.blueF() - a.blueF()) * t);
}
// temperature -> colour: cool cyan, healthy green, warm amber, hot red
QColor tempColor(double c) {
    if (c < 55) return lerp(kCyan, kGreen, (c - 35) / 20);
    if (c < 72) return lerp(kGreen, kAmber, (c - 55) / 17);
    return lerp(kAmber, kRed, (c - 72) / 18);
}
void drawCard(QPainter &p, const QRectF &r, const QColor &accent = QColor()) {
    QLinearGradient g(r.topLeft(), r.bottomRight());
    g.setColorAt(0, QColor(22, 32, 54)); g.setColorAt(1, QColor(13, 19, 33));
    p.setPen(QPen(accent.isValid() ? accent : kEdge, 1.2));
    p.setBrush(g);
    p.drawRoundedRect(r.adjusted(1, 1, -1, -1), 14, 14);
}
void drawGlowArc(QPainter &p, const QRectF &r, double startDeg, double spanDeg, const QColor &c, double w) {
    for (int i = 3; i >= 1; --i) {                       // soft glow: wide, translucent passes under the crisp arc
        QColor g = c; g.setAlpha(22 * (4 - i));
        QPen pen(g, w + i * 5, Qt::SolidLine, Qt::RoundCap); p.setPen(pen);
        p.drawArc(r, int(startDeg * 16), int(spanDeg * 16));
    }
    QPen pen(c, w, Qt::SolidLine, Qt::RoundCap); p.setPen(pen);
    p.drawArc(r, int(startDeg * 16), int(spanDeg * 16));
}
QString fmtEta(double hours) {
    if (hours <= 0 || hours > 48) return {};
    int m = int(hours * 60 + 0.5); return QString("%1h %2m").arg(m / 60).arg(m % 60, 2, 10, QChar('0'));
}
}

// ───────────────────────────── arc gauge (temperature) ─────────────────────────────
class ArcGauge : public QWidget {
public:
    ArcGauge(const QString &title, QWidget *p) : QWidget(p), title(title) { setMinimumSize(170, 190); }
    void set(double v, bool isOff = false, bool isNone = false) { target = v; off = isOff; none = isNone; }
    void step() { shown += (target - shown) * 0.18; if (qAbs(target - shown) < 0.05) shown = target; update(); }
    QString title, sub;
    bool off = false, none = false;
    double target = 0, shown = 0, lo = 30, hi = 100;
protected:
    void paintEvent(QPaintEvent *) override {
        QPainter p(this); p.setRenderHint(QPainter::Antialiasing);
        drawCard(p, rect());
        double side = qMin(width() - 34, height() - 62); QRectF r((width() - side) / 2, 16, side, side);
        QRectF arc = r.adjusted(10, 10, -10, -10);
        p.setBrush(Qt::NoBrush); p.setPen(QPen(QColor(30, 42, 68), 11, Qt::SolidLine, Qt::RoundCap)); p.drawArc(arc, 225 * 16, -270 * 16);
        for (int i = 0; i <= 27; ++i) {                          // tick ring
            double a = qDegreesToRadians(225.0 - i * 10.0); QPointF c = arc.center(); double rr = arc.width() / 2 - 14;
            p.setPen(QPen(QColor(70, 90, 130, 140), i % 9 == 0 ? 2 : 1));
            p.drawLine(c + QPointF(cos(a), -sin(a)) * rr, c + QPointF(cos(a), -sin(a)) * (rr - (i % 9 == 0 ? 7 : 4)));
        }
        double f = qBound(0.0, (shown - lo) / (hi - lo), 1.0);
        QColor c = (off || none) ? QColor(70, 85, 120) : tempColor(shown);
        if (!off && !none && f > 0.001) { p.setBrush(Qt::NoBrush); drawGlowArc(p, arc, 225, -270 * f, c, 11); }
        p.setPen(c);
        QFont big = font(); big.setPixelSize(int(side * 0.27)); big.setBold(true); p.setFont(big);
        QString txt = none ? "—" : off ? "OFF" : QString::number(int(shown + 0.5));
        p.drawText(r.adjusted(0, 0, 0, -side * 0.04), Qt::AlignCenter, txt);
        QFont small = font(); small.setPixelSize(int(side * 0.1)); p.setFont(small); p.setPen(kDim);
        if (!off && !none) p.drawText(QRectF(r.left(), r.center().y() + side * 0.14, r.width(), side * 0.15), Qt::AlignCenter, "°C");
        QFont t = font(); t.setPixelSize(13); t.setBold(true); t.setLetterSpacing(QFont::AbsoluteSpacing, 1.5); p.setFont(t); p.setPen(kText);
        p.drawText(QRectF(0, height() - 40, width(), 20), Qt::AlignCenter, title);
        QFont s = font(); s.setPixelSize(11); p.setFont(s); p.setPen(kDim);
        p.drawText(QRectF(0, height() - 22, width(), 16), Qt::AlignCenter, sub);
    }
};

// ───────────────────────────── spinning fan ─────────────────────────────
class FanDial : public QWidget {
public:
    FanDial(const QString &title, QWidget *p) : QWidget(p), title(title) { setMinimumSize(170, 190); }
    void set(int pctv, int rpmv) { pct = pctv; rpm = rpmv; }
    void step() {
        shownPct += (qMax(pct, 0) - shownPct) * 0.12;
        speed += ((shownPct < 1 ? 0 : 2.0 + shownPct * 0.22) - speed) * 0.08;    // deg per frame, eased so the fan spins up/down
        rot = fmod(rot + speed, 360.0); update();
    }
    QString title;
    int pct = -1, rpm = 0; double shownPct = 0, speed = 0, rot = 0;
protected:
    void paintEvent(QPaintEvent *) override {
        QPainter p(this); p.setRenderHint(QPainter::Antialiasing);
        drawCard(p, rect());
        double side = qMin(width() - 34, height() - 62); QRectF r((width() - side) / 2, 16, side, side);
        QRectF arc = r.adjusted(10, 10, -10, -10); QPointF c = r.center();
        QColor col = lerp(kCyan, kMagenta, shownPct / 100.0);
        p.setBrush(Qt::NoBrush); p.setPen(QPen(QColor(30, 42, 68), 7, Qt::SolidLine, Qt::RoundCap)); p.drawArc(arc, 225 * 16, -270 * 16);
        if (pct >= 0 && shownPct > 0.5) drawGlowArc(p, arc, 225, -270 * qMin(shownPct, 100.0) / 100.0, col, 7);
        p.save(); p.translate(c); p.rotate(rot);
        double R = arc.width() / 2 - 16;
        for (int i = 0; i < 7; ++i) {
            QPainterPath b; b.moveTo(R * 0.16, 0);
            b.cubicTo(R * 0.35, -R * 0.62, R * 0.92, -R * 0.62, R * 0.98, -R * 0.12);
            b.cubicTo(R * 0.80, -R * 0.20, R * 0.50, -R * 0.12, R * 0.16, R * 0.10); b.closeSubpath();
            QLinearGradient g(0, 0, R, 0); QColor a = col; a.setAlpha(230); QColor z = col; z.setAlpha(60); g.setColorAt(0, a); g.setColorAt(1, z);
            p.setBrush(g); p.setPen(QPen(col.lighter(130), 0.8)); p.drawPath(b); p.rotate(360.0 / 7);
        }
        p.restore();
        p.setPen(Qt::NoPen); QRadialGradient hub(c, R * 0.2); hub.setColorAt(0, QColor(235, 245, 255)); hub.setColorAt(1, col.darker(160)); p.setBrush(hub); p.drawEllipse(c, R * 0.17, R * 0.17);
        QFont t = font(); t.setPixelSize(13); t.setBold(true); t.setLetterSpacing(QFont::AbsoluteSpacing, 1.5); p.setFont(t); p.setPen(kText);
        p.drawText(QRectF(0, height() - 40, width(), 20), Qt::AlignCenter, title);
        QFont s = font(); s.setPixelSize(11); p.setFont(s); p.setPen(kDim);
        QString sub = pct < 0 ? "no data" : (rpm > 0 ? QString("%1 %  ·  %2 rpm").arg(pct).arg(rpm) : QString("%1 %").arg(pct));
        p.drawText(QRectF(0, height() - 22, width(), 16), Qt::AlignCenter, sub);
    }
};

// ───────────────────────────── battery ring ─────────────────────────────
class BatteryRing : public QWidget {
public:
    BatteryRing(QWidget *p) : QWidget(p) { setMinimumSize(170, 190); }
    void step() { shown += (cap - shown) * 0.12; phase = fmod(phase + 0.06, 2 * M_PI); update(); }
    int cap = 0, limit = 100; double shown = 0, phase = 0; bool charging = false, plugged = false; QString line1, line2;
protected:
    void paintEvent(QPaintEvent *) override {
        QPainter p(this); p.setRenderHint(QPainter::Antialiasing);
        drawCard(p, rect(), charging ? QColor(40, 120, 90) : QColor());
        double side = qMin(width() - 34, height() - 62); QRectF r((width() - side) / 2, 16, side, side);
        QRectF arc = r.adjusted(10, 10, -10, -10);
        QColor col = shown < 15 ? kRed : shown < 30 ? kAmber : kGreen;
        p.setBrush(Qt::NoBrush); p.setPen(QPen(QColor(30, 42, 68), 11, Qt::SolidLine, Qt::RoundCap)); p.drawArc(arc, 225 * 16, -270 * 16);
        if (limit < 100) {                                        // the part the charge limit keeps you out of, drawn as a dim hatch
            p.setPen(QPen(QColor(255, 176, 32, 50), 11, Qt::SolidLine, Qt::FlatCap)); p.drawArc(arc, int((225 - 2.7 * limit) * 16), int(-2.7 * (100 - limit) * 16));
        }
        if (shown > 0.5) drawGlowArc(p, arc, 225, -2.7 * shown, col, 11);
        if (limit < 100) {                                        // limit marker
            double a = qDegreesToRadians(225.0 - 2.7 * limit); QPointF c = arc.center(), d(cos(a), -sin(a)); double rr = arc.width() / 2;
            p.setPen(QPen(kAmber, 3, Qt::SolidLine, Qt::RoundCap)); p.drawLine(c + d * (rr - 12), c + d * (rr + 9));
        }
        p.setPen(col); QFont big = font(); big.setPixelSize(int(side * 0.27)); big.setBold(true); p.setFont(big);
        p.drawText(r.adjusted(0, 0, 0, -side * 0.04), Qt::AlignCenter, QString::number(int(shown + 0.5)));
        QFont small = font(); small.setPixelSize(int(side * 0.1)); p.setFont(small); p.setPen(kDim);
        p.drawText(QRectF(r.left(), r.center().y() + side * 0.14, r.width(), side * 0.15), Qt::AlignCenter, "%");
        if (charging) {                                           // pulsing bolt
            QPainterPath b; QPointF c(r.center().x(), r.top() + side * 0.12); double s = side * 0.07;
            b.moveTo(c + QPointF(s * 0.4, -s)); b.lineTo(c + QPointF(-s * 0.5, s * 0.15)); b.lineTo(c + QPointF(0, s * 0.15)); b.lineTo(c + QPointF(-s * 0.4, s)); b.lineTo(c + QPointF(s * 0.5, -s * 0.15)); b.lineTo(c + QPointF(0, -s * 0.15)); b.closeSubpath();
            QColor g = kGreen; g.setAlphaF(0.55 + 0.45 * sin(phase)); p.setBrush(g); p.setPen(Qt::NoPen); p.drawPath(b);
        }
        QFont t = font(); t.setPixelSize(13); t.setBold(true); t.setLetterSpacing(QFont::AbsoluteSpacing, 1.5); p.setFont(t); p.setPen(kText);
        p.drawText(QRectF(0, height() - 40, width(), 20), Qt::AlignCenter, line1);
        QFont s = font(); s.setPixelSize(11); p.setFont(s); p.setPen(kDim);
        p.drawText(QRectF(0, height() - 22, width(), 16), Qt::AlignCenter, line2);
    }
};

// ───────────────────────────── history graph ─────────────────────────────
class HistoryGraph : public QWidget {
public:
    HistoryGraph(QWidget *p) : QWidget(p) { setMinimumSize(380, 190); }
    void push(double cpu, double gpu, double watts) {
        for (auto *d : {&c, &g, &w}) if (d->size() >= N) d->pop_front();
        c.push_back(cpu); g.push_back(gpu); w.push_back(watts); update();
    }
protected:
    void paintEvent(QPaintEvent *) override {
        QPainter p(this); p.setRenderHint(QPainter::Antialiasing);
        drawCard(p, rect());
        QRectF plot(46, 34, width() - 46 - 50, height() - 34 - 22);
        QFont f = font(); f.setPixelSize(10); p.setFont(f);
        for (int t = 40; t <= 100; t += 20) {                    // grid + temperature axis
            double y = plot.bottom() - (t - 30) / 70.0 * plot.height();
            p.setPen(QPen(QColor(40, 56, 90, 140), 1, Qt::DotLine)); p.drawLine(QPointF(plot.left(), y), QPointF(plot.right(), y));
            p.setPen(kDim); p.drawText(QRectF(0, y - 8, 40, 16), Qt::AlignRight | Qt::AlignVCenter, QString("%1°").arg(t));
        }
        double wmax = 30; for (double v : w) wmax = qMax(wmax, v); wmax = ceil(wmax / 10) * 10;
        p.setPen(kDim); p.drawText(QRectF(plot.right() + 4, plot.top() - 8, 46, 16), Qt::AlignLeft | Qt::AlignVCenter, QString("%1 W").arg(int(wmax)));
        p.drawText(QRectF(plot.right() + 4, plot.bottom() - 8, 46, 16), Qt::AlignLeft | Qt::AlignVCenter, "0 W");
        p.drawText(QRectF(plot.left(), plot.bottom() + 4, 80, 14), Qt::AlignLeft, "3 min ago"); p.drawText(QRectF(plot.right() - 80, plot.bottom() + 4, 80, 14), Qt::AlignRight, "now");
        auto line = [&](const std::deque<double> &d, double lo, double hi, const QColor &col, bool fill) {
            if (d.size() < 2) return;
            QPainterPath path; bool open = false; double dx = plot.width() / (N - 1); double x0 = plot.right() - dx * (d.size() - 1);
            QPointF last, first;
            for (size_t i = 0; i < d.size(); ++i) {
                if (d[i] < 0) { open = false; continue; }
                QPointF pt(x0 + i * dx, plot.bottom() - qBound(0.0, (d[i] - lo) / (hi - lo), 1.0) * plot.height());
                if (!open) { path.moveTo(pt); first = pt; open = true; } else path.lineTo(pt);
                last = pt;
            }
            if (fill && open) {
                QPainterPath a = path; a.lineTo(last.x(), plot.bottom()); a.lineTo(first.x(), plot.bottom()); a.closeSubpath();
                QLinearGradient g(0, plot.top(), 0, plot.bottom()); QColor t = col; t.setAlpha(80); QColor b = col; b.setAlpha(0); g.setColorAt(0, t); g.setColorAt(1, b);
                p.setPen(Qt::NoPen); p.setBrush(g); p.drawPath(a);
            }
            p.setBrush(Qt::NoBrush); QColor gl = col; gl.setAlpha(60); p.setPen(QPen(gl, 5)); p.drawPath(path); p.setPen(QPen(col, 2)); p.drawPath(path);
        };
        line(w, 0, wmax, kViolet, false); line(g, 30, 100, kMagenta, false); line(c, 30, 100, kCyan, true);
        int x = 16; QFont lf = font(); lf.setPixelSize(12); lf.setBold(true); p.setFont(lf);
        struct L { QColor c; QString n; } ls[] = {{kCyan, "CPU °C"}, {kMagenta, "GPU °C"}, {kViolet, "BATTERY W"}};
        for (auto &l : ls) { p.setBrush(l.c); p.setPen(Qt::NoPen); p.drawEllipse(QPointF(x, 18), 4, 4); p.setPen(kText); p.drawText(QRectF(x + 9, 9, 110, 18), Qt::AlignVCenter, l.n); x += 118; }
    }
private:
    static constexpr int N = 180;
    std::deque<double> c, g, w;
};

// ───────────────────────────── tile button ─────────────────────────────
class ModeTile : public QAbstractButton {
public:
    ModeTile(const QString &title, const QString &sub, const QColor &accent, QWidget *p, bool checkable = true) : QAbstractButton(p), sub(sub), accent(accent) {
        setText(title); setCheckable(checkable); setCursor(Qt::PointingHandCursor); setMinimumSize(110, 56); setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    }
    QString sub; QColor accent;
protected:
    void nextCheckState() override {}                               // state is driven by the real hardware value, never by the click itself
    void enterEvent(QEnterEvent *) override { hover = true; update(); }
    void leaveEvent(QEvent *) override { hover = false; update(); }
    void paintEvent(QPaintEvent *) override {
        QPainter p(this); p.setRenderHint(QPainter::Antialiasing);
        QRectF r = QRectF(rect()).adjusted(1, 1, -1, -1); bool on = isChecked();
        QLinearGradient g(r.topLeft(), r.bottomRight());
        QColor a = accent; a.setAlpha(on ? 70 : (hover ? 34 : 14)); g.setColorAt(0, a); g.setColorAt(1, QColor(13, 19, 33));
        if (on) { for (int i = 3; i >= 1; --i) { QColor gl = accent; gl.setAlpha(18 * (4 - i)); p.setPen(QPen(gl, i * 3)); p.setBrush(Qt::NoBrush); p.drawRoundedRect(r, 12, 12); } }
        p.setPen(QPen(on ? accent : (hover ? accent.darker(140) : kEdge), on ? 1.8 : 1.2)); p.setBrush(g); p.drawRoundedRect(r, 12, 12);
        p.setPen(isEnabled() ? (on ? accent.lighter(130) : kText) : kDim);
        QFont f = font(); f.setPixelSize(14); f.setBold(true); p.setFont(f);
        p.drawText(QRectF(14, 8, width() - 28, 22), Qt::AlignLeft | Qt::AlignVCenter, text());
        QFont s = font(); s.setPixelSize(11); p.setFont(s); p.setPen(kDim);
        p.drawText(QRectF(14, 30, width() - 28, 18), Qt::AlignLeft | Qt::AlignVCenter, sub);
        if (on) { p.setPen(Qt::NoPen); p.setBrush(accent); p.drawEllipse(QPointF(width() - 16, 18), 4, 4); }
    }
private:
    bool hover = false;
};

// ───────────────────────────── page ─────────────────────────────
static QLabel *mkLabel(const QString &t, const QString &obj, QWidget *p) { auto *l = new QLabel(t, p); l->setObjectName(obj); return l; }

DashboardPage::DashboardPage(QWidget *parent) : QWidget(parent) {
    setObjectName("dashboard");
    for (const QString &n : QDir("/sys/class/power_supply").entryList({"BAT*"}, QDir::Dirs)) { batPath = "/sys/class/power_supply/" + n; break; }
    QString model = readFile("/sys/class/dmi/id/product_name"), board = readFile("/sys/class/dmi/id/board_name"), fw = readFile("/sys/devices/platform/msi-ec/fw_version");

    auto *root = new QVBoxLayout(this); root->setContentsMargins(14, 10, 14, 12); root->setSpacing(10);

    auto *head = new QHBoxLayout; auto *tv = new QVBoxLayout; tv->setSpacing(0);
    QString title = model.isEmpty() ? "MSI LAPTOP" : model.left(model.indexOf(" D2")).toUpper();
    tv->addWidget(mkLabel(title, "dashTitle", this));
    subtitle = mkLabel(QString("%1  ·  EC %2").arg(board, fw.isEmpty() ? "n/a" : fw), "dashSub", this); tv->addWidget(subtitle);
    head->addLayout(tv); head->addStretch();
    chipPower = mkLabel("", "chip", this); chipGpu = mkLabel("", "chip", this);
    head->addWidget(chipGpu); head->addSpacing(8); head->addWidget(chipPower);
    root->addLayout(head);

    auto *row1 = new QHBoxLayout; row1->setSpacing(10);
    cpuGauge = new ArcGauge("CPU", this); cpuGauge->sub = "Core Ultra 7 255HX";
    gpuGauge = new ArcGauge("GPU", this); gpuGauge->sub = "RTX 5060 Laptop";
    cpuFan = new FanDial("CPU FAN", this); gpuFan = new FanDial("GPU FAN", this); battery = new BatteryRing(this);
    for (QWidget *w : {(QWidget *) cpuGauge, (QWidget *) gpuGauge, (QWidget *) cpuFan, (QWidget *) gpuFan, (QWidget *) battery}) row1->addWidget(w, 1);
    root->addLayout(row1, 3);

    auto *row2 = new QHBoxLayout; row2->setSpacing(10);
    graph = new HistoryGraph(this); row2->addWidget(graph, 3);
    auto *stats = new QFrame(this); stats->setObjectName("card"); auto *sg = new QGridLayout(stats); sg->setContentsMargins(16, 12, 16, 12); sg->setVerticalSpacing(5);
    struct S { const char *k; QLabel **v; } rows[] = {{"CPU load", &statLoad}, {"CPU clock", &statFreq}, {"Memory", &statRam}, {"NVMe", &statNvme}, {"Battery draw", &statDraw}, {"Time left", &statEta}, {"Battery health", &statHealth}, {"Cycles", &statCycles}};
    int ri = 0; for (auto &s : rows) { sg->addWidget(mkLabel(s.k, "statKey", stats), ri, 0); *s.v = mkLabel("—", "statVal", stats); (*s.v)->setAlignment(Qt::AlignRight); sg->addWidget(*s.v, ri, 1); ++ri; }
    row2->addWidget(stats, 2); root->addLayout(row2, 2);

    // controls
    auto *ctl = new QGridLayout; ctl->setHorizontalSpacing(8); ctl->setVerticalSpacing(6);
    ctl->addWidget(mkLabel("POWER MODE", "section", this), 0, 0, 1, 4); ctl->addWidget(mkLabel("COOLING", "section", this), 0, 4); ctl->addWidget(mkLabel("CHARGE LIMIT", "section", this), 0, 5, 1, 3);
    const char *mt[4] = {"Turbo", "Balanced", "Silent", "Super Battery"}, *ms[4] = {"max performance", "comfort", "quiet fans", "lowest power"};
    QColor mc[4] = {kMagenta, kCyan, kViolet, kGreen};
    for (int i = 0; i < 4; ++i) {
        tiles[i] = new ModeTile(mt[i], ms[i], mc[i], this); ctl->addWidget(tiles[i], 1, i);
        connect(tiles[i], &QAbstractButton::clicked, this, [this, i] { for (int j = 0; j < 4; ++j) tiles[j]->setChecked(j == i); emit modeRequested(i); });
    }
    boostBtn = new ModeTile("Cooler Boost", "fans to maximum", kAmber, this); ctl->addWidget(boostBtn, 1, 4);
    connect(boostBtn, &QAbstractButton::clicked, this, [this] { bool on = !boostBtn->isChecked(); boostBtn->setChecked(on); emit coolerBoostRequested(on); });
    const int lv[3] = {60, 80, 100}; const char *ln[3] = {"Best battery", "Balanced", "Full"};
    for (int i = 0; i < 3; ++i) {
        limitBtns[i] = new ModeTile(QString("%1 %").arg(lv[i]), ln[i], kGreen, this); ctl->addWidget(limitBtns[i], 1, 5 + i);
        connect(limitBtns[i], &QAbstractButton::clicked, this, [this, i, lv] { for (int j = 0; j < 3; ++j) limitBtns[j]->setChecked(j == i); emit chargeLimitRequested(lv[i]); });
    }
    auto *pre = new QGridLayout; pre->setHorizontalSpacing(8); pre->addWidget(mkLabel("ONE-TOUCH PRESETS", "section", this), 0, 0, 1, 4);
    struct P { const char *n, *s; QColor c; } ps[] = {{"Gaming", "Turbo mode + Cooler Boost", kMagenta}, {"Work", "Balanced, fans on auto", kCyan}, {"Night", "Silent, fans on auto", kViolet}, {"Travel", "Super Battery, 80 % charge cap", kGreen}};
    for (int i = 0; i < 4; ++i) {
        auto *t = new ModeTile(ps[i].n, ps[i].s, ps[i].c, this, false); pre->addWidget(t, 1, i);
        connect(t, &QAbstractButton::clicked, this, [this, n = QString(ps[i].n)] { applyPreset(n); });
    }
    root->addLayout(ctl, 0); root->addLayout(pre, 0);

    connect(&fastTimer, &QTimer::timeout, this, &DashboardPage::animate); fastTimer.setInterval(40);
    connect(&slowTimer, &QTimer::timeout, this, &DashboardPage::sample); slowTimer.setInterval(1000);
    setControlsEnabled(false);
}

void DashboardPage::setControlsEnabled(bool en) {
    controlsEnabled = en;
    for (auto *t : findChildren<QAbstractButton *>()) t->setEnabled(en);
}

void DashboardPage::showEvent(QShowEvent *) { fastTimer.start(); slowTimer.start(); sample(); }
void DashboardPage::hideEvent(QHideEvent *) { fastTimer.stop(); slowTimer.stop(); }   // hidden in the tray = zero CPU for the animation

void DashboardPage::applyPreset(const QString &n) {
    int mode = 1; bool boost = false; int limit = -1;
    if (n == "Gaming") { mode = 0; boost = true; } else if (n == "Night") mode = 2; else if (n == "Travel") { mode = 3; limit = 80; }
    for (int j = 0; j < 4; ++j) tiles[j]->setChecked(j == mode);
    emit modeRequested(mode);
    if (boost != cur.coolerBoost) { boostBtn->setChecked(boost); emit coolerBoostRequested(boost); }
    if (limit > 0) { for (int j = 0; j < 3; ++j) limitBtns[j]->setChecked(j == 1); emit chargeLimitRequested(limit); }
}

void DashboardPage::setData(const DashData &d) {
    cur = d;
    cpuGauge->set(d.cpuTemp);
    gpuGauge->set(qMax(d.gpuTemp, 0), d.gpuTemp == 0, d.gpuTemp < 0); gpuGauge->sub = d.gpuTemp == 0 ? "dGPU asleep (D3cold)" : "RTX 5060 Laptop";
    cpuFan->set(d.cpuFanPct, d.cpuFanRpm); gpuFan->set(d.gpuFanPct, d.gpuFanRpm);
    for (int i = 0; i < 4; ++i) tiles[i]->setChecked(i == d.mode);
    boostBtn->setChecked(d.coolerBoost);
    int idx = d.chargeLimit <= 60 ? 0 : d.chargeLimit <= 80 ? 1 : 2; for (int j = 0; j < 3; ++j) limitBtns[j]->setChecked(j == idx);
    battery->limit = d.chargeLimit;
}

void DashboardPage::animate() {
    cpuGauge->step(); gpuGauge->step(); cpuFan->step(); gpuFan->step(); battery->step();
}

void DashboardPage::sample() {
    // battery
    double watts = 0, hours = 0; QString status = readFile(batPath + "/status");
    int cap = int(readInt(batPath + "/capacity", 0));
    double v = readInt(batPath + "/voltage_now", 0) / 1e6, i = qAbs(double(readInt(batPath + "/current_now", 0))) / 1e6; watts = v * i;
    double now = readInt(batPath + "/charge_now", 0), full = readInt(batPath + "/charge_full", 0), design = readInt(batPath + "/charge_full_design", 0), cur_ua = qAbs(double(readInt(batPath + "/current_now", 0)));
    bool charging = status == "Charging", dis = status == "Discharging";
    if (cur_ua > 1000) hours = dis ? now / cur_ua : charging ? (full - now) / cur_ua : 0;
    battery->cap = cap; battery->charging = charging;
    battery->line1 = charging ? "CHARGING" : dis ? "ON BATTERY" : status == "Full" ? "FULL" : status == "Not charging" ? "LIMIT REACHED" : status.toUpper();
    battery->line2 = !fmtEta(hours).isEmpty() ? (dis ? fmtEta(hours) + " left" : fmtEta(hours) + " to charge") : QString("limit %1 %").arg(cur.chargeLimit);
    statDraw->setText(dis || charging ? QString("%1 W %2").arg(watts, 0, 'f', 1).arg(charging ? "in" : "out") : "idle");
    statEta->setText(fmtEta(hours).isEmpty() ? "—" : fmtEta(hours));
    statHealth->setText(design > 0 ? QString("%1 %  (%2 / %3 Wh)").arg(int(100.0 * full / design + 0.5)).arg(full * v / 1e6 * 1.0 / 1.0, 0, 'f', 0).arg(design * v / 1e6, 0, 'f', 0) : "—");
    statCycles->setText(readInt(batPath + "/cycle_count", 0) > 0 ? QString::number(readInt(batPath + "/cycle_count")) : "not reported");
    bool ac = readInt("/sys/class/power_supply/ADP1/online", 0) == 1;
    chipPower->setText(ac ? "● AC POWER" : "● BATTERY"); chipPower->setProperty("tone", ac ? "ok" : "warn");

    // cpu load + clock
    QStringList st = readFile("/proc/stat").section('\n', 0, 0).split(' ', Qt::SkipEmptyParts);
    if (st.size() > 8) {
        quint64 tot = 0; for (int k = 1; k <= 8; ++k) tot += st[k].toULongLong(); quint64 idle = st[4].toULongLong() + st[5].toULongLong();
        if (lastTotal) statLoad->setText(QString("%1 %").arg(int(100.0 * (1.0 - double(idle - lastIdle) / double(qMax<quint64>(tot - lastTotal, 1))) + 0.5)));
        lastTotal = tot; lastIdle = idle;
    }
    double fsum = 0; int fn = 0; for (const QString &c : QDir("/sys/devices/system/cpu").entryList({"cpu[0-9]*"}, QDir::Dirs)) { qint64 f = readInt("/sys/devices/system/cpu/" + c + "/cpufreq/scaling_cur_freq"); if (f > 0) { fsum += f; ++fn; } }
    statFreq->setText(fn ? QString("%1 GHz avg").arg(fsum / fn / 1e6, 0, 'f', 2) : "—");
    QString mi = readFile("/proc/meminfo"); auto kb = [&](const char *k) { int a = mi.indexOf(QString(k) + ":"); return a < 0 ? 0.0 : mi.mid(a + QString(k).size() + 1, 20).trimmed().section(' ', 0, 0).toDouble(); };
    double mt = kb("MemTotal"), ma = kb("MemAvailable"); statRam->setText(mt > 0 ? QString("%1 / %2 GB").arg((mt - ma) / 1048576, 0, 'f', 1).arg(mt / 1048576, 0, 'f', 1) : "—");
    int nv = -1; for (const QString &h : QDir("/sys/class/hwmon").entryList({"hwmon*"}, QDir::Dirs)) if (readFile("/sys/class/hwmon/" + h + "/name") == "nvme") { nv = int(readInt("/sys/class/hwmon/" + h + "/temp1_input") / 1000); break; }
    statNvme->setText(nv > 0 ? QString("%1 °C").arg(nv) : "—");
    QString dg = readFile("/sys/bus/pci/devices/0000:01:00.0/power_state");
    chipGpu->setText(dg.isEmpty() ? "● dGPU n/a" : dg.startsWith("D3") ? "● dGPU ASLEEP" : "● dGPU ACTIVE"); chipGpu->setProperty("tone", dg.startsWith("D3") ? "ok" : "warn");
    for (QLabel *c : {chipPower, chipGpu}) { c->style()->unpolish(c); c->style()->polish(c); }

    graph->push(cur.cpuTemp > 0 ? cur.cpuTemp : -1, cur.gpuTemp > 0 ? cur.gpuTemp : -1, dis ? watts : 0);
}
