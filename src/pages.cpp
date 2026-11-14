#include "pages.h"
#include <algorithm>
#include "sensors.h"
#include "uikit.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QScrollArea>

using namespace kit;

// ───────────────────────────── page animation driver ─────────────────────────────
AnimPage::AnimPage(QWidget *p) : QWidget(p) { setObjectName("page"); timer.setInterval(16); timer.setTimerType(Qt::PreciseTimer); connect(&timer, &QTimer::timeout, this, [this] { tick(); }); }
void AnimPage::adopt(Panel *pn, double delay) { pn->delay = delay; pn->wake = [this] { kick(); }; panels.push_back(pn); }
void AnimPage::drop(Panel *pn) { panels.erase(std::remove(panels.begin(), panels.end(), pn), panels.end()); }
void AnimPage::kick() { idle = 0; if (!timer.isActive() && isVisible()) { last = clock.elapsed() / 1000.0; timer.start(); } }
void AnimPage::showEvent(QShowEvent *) { clock.start(); last = 0; for (auto *p : panels) p->reveal = 0; idle = 0; timer.start(); }       // entrance animation replays each time the page opens
void AnimPage::hideEvent(QHideEvent *) { timer.stop(); }
void AnimPage::tick() {
    double now = clock.elapsed() / 1000.0, dt = qBound(0.001, now - last, 0.05); last = now; bool any = false;
    QColor before = gAccent; gAccent = mix(gAccent, gAccentTarget, ease(dt, 0.12)); bool accentMoved = before != gAccent;
    for (Panel *p : panels) { bool a = p->tick(now, dt); if (auto *rp = dynamic_cast<RowsPanel *>(p)) a |= rp->animate(dt); if (a || accentMoved) p->update(); any |= a; }
    any |= animateExtra(now, dt) | accentMoved;
    if (any) idle = 0; else if (++idle > 8) timer.stop();
}

static std::vector<QString> modeNames() { return {"Turbo", "Balanced", "Silent", "Eco"}; }

// ───────────────────────────── charge limit ─────────────────────────────
class ChargePanel : public Panel {
public:
    ChargePanel(QWidget *p) : Panel("", "CHARGE LIMIT", p) { setMinimumHeight(300); }
    int limit = 100, drag = -1, recommended = 0; bool supported = true; std::function<void(int)> onCommit;
    void setValue(int v) { if (drag < 0) limit = v; }
protected:
    QRectF track; QRectF chips[3];
    int valueAt(double x) const { double f = qBound(0.0, (x - track.left()) / track.width(), 1.0); return qBound(30, int(round((30 + f * 70) / 5.0) * 5), 100); }
    void mousePressEvent(QMouseEvent *e) override {
        if (!supported) return; const int pv[3] = {60, 80, 100};
        for (int i = 0; i < 3; ++i) if (chips[i].contains(e->position())) { limit = pv[i]; update(); if (onCommit) onCommit(limit); return; }
        if (track.adjusted(-8, -22, 8, 22).contains(e->position())) { drag = 1; limit = valueAt(e->position().x()); update(); }
    }
    void mouseMoveEvent(QMouseEvent *e) override {
        if (drag >= 0) { limit = valueAt(e->position().x()); update(); }
        bool hot = supported && (track.adjusted(-8, -22, 8, 22).contains(e->position())); for (auto &c : chips) hot |= supported && c.contains(e->position()); setCursor(hot ? Qt::PointingHandCursor : Qt::ArrowCursor);
    }
    void mouseReleaseEvent(QMouseEvent *) override { if (drag >= 0) { drag = -1; if (onCommit) onCommit(limit); update(); } }
    void paintBody(QPainter &p, const QRectF &b) override {
        p.setFont(fnt(RAJ, 96, QFont::Bold)); p.setPen(supported ? kText : kDim); p.drawText(QRectF(b.left() - 4, b.top() - 6, 200, 100), Qt::AlignLeft | Qt::AlignVCenter, QString::number(limit));
        p.setFont(fnt(RAJ, 30, QFont::Medium)); p.setPen(kDim); p.drawText(QRectF(b.left() + (limit >= 100 ? 150 : 100), b.top() + 14, 40, 36), Qt::AlignLeft | Qt::AlignTop, "%");
        p.setFont(fnt(MONO, 11)); p.setPen(kDim);
        p.drawText(QRectF(b.left() + 210, b.top() + 10, b.width() - 210, 70), Qt::AlignLeft | Qt::AlignVCenter | Qt::TextWordWrap,
                   !supported ? "This laptop's firmware does not expose a charge limit." : limit >= 100 ? "No limit: the battery charges to full." : QString("Starts charging below %1 %, stops at %2 %.\nKeeps the battery healthier when you are mostly plugged in.").arg(limit - 10).arg(limit));
        track = QRectF(b.left(), b.top() + 124, b.width(), 22);
        const int n = 35; double gap = 3, w = (track.width() - gap * (n - 1)) / n; double f = (limit - 30) / 70.0;
        for (int i = 0; i < n; ++i) { QRectF s(track.left() + i * (w + gap), track.top(), w, track.height()); bool on = (i + 0.5) / n <= f + 0.001; p.setPen(Qt::NoPen); p.setBrush(on ? (supported ? kText : kDim) : alpha(kText, 0.10)); p.drawRect(s); }
        double hx = track.left() + f * track.width(); p.setPen(QPen(gAccent, 3)); p.drawLine(QPointF(hx, track.top() - 10), QPointF(hx, track.bottom() + 10));
        p.setBrush(gAccent); p.setPen(Qt::NoPen); QPolygonF tri; tri << QPointF(hx - 6, track.top() - 18) << QPointF(hx + 6, track.top() - 18) << QPointF(hx, track.top() - 8); p.drawPolygon(tri);
        p.setFont(fnt(MONO, 10)); p.setPen(kDim); for (int v : {30, 40, 60, 80, 100}) { double x = track.left() + (v - 30) / 70.0 * track.width(); p.drawText(QRectF(x - 20, track.bottom() + 14, 40, 14), Qt::AlignCenter, QString::number(v)); }
        const int pv[3] = {60, 80, 100}; const char *nm[3] = {"BEST FOR BATTERY", "BALANCED", "BEST FOR MOBILITY"}; double cw = (b.width() - 12) / 3;
        for (int i = 0; i < 3; ++i) {
            chips[i] = QRectF(b.left() + i * (cw + 6), b.bottom() - 46, cw, 46); bool on = limit == pv[i];
            p.setPen(QPen(on ? gAccent : kLine, 1)); p.setBrush(on ? gAccent : QBrush(Qt::NoBrush)); p.drawPath(chamfer(chips[i], 9));
            p.setFont(fnt(RAJ, 22, QFont::Bold, 1)); p.setPen(on ? kBg : (supported ? kText : kDim)); p.drawText(QRectF(chips[i].left() + 14, chips[i].top() + 4, cw, 24), Qt::AlignLeft | Qt::AlignVCenter, QString("%1 %").arg(pv[i]));
            p.setFont(fnt(MONO, 9, QFont::Normal, 1)); p.setPen(on ? alpha(kBg, 0.75) : kDim); p.drawText(QRectF(chips[i].left() + 14, chips[i].top() + 27, cw, 14), Qt::AlignLeft | Qt::AlignVCenter, nm[i]);
            if (recommended == pv[i]) { p.setFont(fnt(MONO, 8, QFont::Medium, 1)); p.setPen(on ? kBg : gAccent); p.drawText(QRectF(chips[i].left(), chips[i].top() + 5, cw - 10, 12), Qt::AlignRight, "MSI RECOMMENDS"); }
        }
    }
};

// Battery health over time (one point a day) with a marker on every calibration: shows whether calibration / the charge limit actually helps.
class HistoryPanel : public Panel {
public:
    HistoryPanel(QWidget *p) : Panel("", "HEALTH HISTORY", p) { setMinimumHeight(250); }
    QVector<HistPoint> pts; QVector<HistEvent> ev; QString note;
protected:
    void paintBody(QPainter &p, const QRectF &b) override {
        QRectF g(b.left() + 34, b.top() + 8, b.width() - 44, b.height() - 56);
        p.setFont(fnt(MONO, 10)); p.setPen(kDim);
        if (pts.size() < 2) { p.drawText(b, Qt::AlignCenter | Qt::TextWordWrap, "One point is recorded each day the app runs.\nThe graph appears after two days."); return; }
        double lo = 100, hi = 0; for (const auto &q : pts) { lo = qMin(lo, q.health); hi = qMax(hi, q.health); }
        lo = qMax(0.0, floor(lo - 1)); hi = qMin(100.0, ceil(hi + 0.5)); if (hi - lo < 3) { lo = qMax(0.0, hi - 3); }
        const qint64 d0 = pts.first().day.toJulianDay(), d1 = qMax(pts.last().day.toJulianDay(), d0 + 1);
        auto X = [&](qint64 d) { return g.left() + double(d - d0) / double(d1 - d0) * g.width(); }; auto Y = [&](double v) { return g.bottom() - (v - lo) / (hi - lo) * g.height(); };
        p.setPen(QPen(alpha(kText, 0.08), 1, Qt::DotLine)); for (int i = 0; i <= 3; ++i) { double v = lo + (hi - lo) * i / 3, y = Y(v); p.drawLine(QPointF(g.left(), y), QPointF(g.right(), y)); p.setPen(kDim); p.drawText(QRectF(b.left(), y - 7, 30, 14), Qt::AlignRight | Qt::AlignVCenter, QString::number(v, 'f', 0) + "%"); p.setPen(QPen(alpha(kText, 0.08), 1, Qt::DotLine)); }
        for (const auto &e : ev) { const double x = X(e.day.toJulianDay()); p.setPen(QPen(gAccent, 1, Qt::DashLine)); p.drawLine(QPointF(x, g.top()), QPointF(x, g.bottom())); p.setFont(fnt(MONO, 9, QFont::Medium, 1)); p.setPen(gAccent); p.drawText(QRectF(x + 3, g.top(), 80, 12), Qt::AlignLeft, "CALIBRATED"); }
        QPainterPath path; for (int i = 0; i < pts.size(); ++i) { const QPointF pt(X(pts[i].day.toJulianDay()), Y(pts[i].health)); i ? path.lineTo(pt) : path.moveTo(pt); }
        neon(p, path, kText, 1.8);
        p.setBrush(gAccent); p.setPen(Qt::NoPen); p.drawEllipse(QPointF(X(d1), Y(pts.last().health)), 3.5, 3.5);
        p.setFont(fnt(MONO, 10)); p.setPen(kDim); p.drawText(QRectF(g.left(), g.bottom() + 6, 120, 14), Qt::AlignLeft, pts.first().day.toString("d MMM yyyy")); p.drawText(QRectF(g.right() - 120, g.bottom() + 6, 120, 14), Qt::AlignRight, pts.last().day.toString("d MMM yyyy"));
        p.setPen(kText); p.drawText(QRectF(g.left(), g.bottom() + 22, g.width(), 14), Qt::AlignLeft, note);
    }
};

BatteryPage::BatteryPage(QWidget *p) : AnimPage(p) {
    auto *outer = new QVBoxLayout(this); outer->setContentsMargins(0, 0, 0, 0);
    auto *scroll = new QScrollArea(this); scroll->setWidgetResizable(true); scroll->setFrameShape(QFrame::NoFrame); scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll->setStyleSheet("QScrollArea { background: transparent; border: none; } QScrollArea > QWidget > QWidget { background: transparent; }");
    auto *content = new QWidget; scroll->setWidget(content); outer->addWidget(scroll);
    auto *grid = new QGridLayout(content); grid->setContentsMargins(18, 16, 18, 16); grid->setSpacing(10);
    charge = new ChargePanel(content); info = new RowsPanel("", "BATTERY", content); exercise = new RowsPanel("", "BATTERY CALIBRATION", content);
    charge->onCommit = [this](int v) { if (onLimit) onLimit(v); };
    charge->setFixedHeight(300);
    history = new HistoryPanel(content);
    grid->addWidget(charge, 0, 0); grid->addWidget(info, 0, 1); grid->addWidget(history, 1, 0, 1, 2); grid->addWidget(exercise, 2, 0, 1, 2); grid->setColumnStretch(0, 3); grid->setColumnStretch(1, 2); grid->setRowStretch(3, 1);
    adopt(charge, 0.0); adopt(info, 0.1); adopt(history, 0.15); adopt(exercise, 0.2);
    for (auto [l, k] : std::vector<std::pair<QString, int>>{{"Status", 0}, {"Charge", 1}, {"Health", 2}, {"Cycles", 3}, {"Draw", 4}, {"Time", 5}}) { Row r; r.type = Row::Info; r.label = l; info->rows.push_back(r); (void) k; }
    Row g; g.type = Row::Gap; info->rows.push_back(g);
    Row t; t.type = Row::Text; t.label = "MSI recommends its \"Best for Battery\" setting (60 %) when the laptop is mostly used plugged in, \"Balanced\" (80 %) in between, and \"Best for Mobility\" (100 %) for frequent battery use. The tag above follows how you really use yours. Changes apply instantly."; info->rows.push_back(t); info->relayout();

    // battery calibration, following MSI's published guidance (no questions asked: interval and target come from MSI + your real usage)
    { Row r; r.type = Row::Switch; r.label = "Keep my battery calibrated"; r.sub = "MSI's recommendation, applied automatically"; r.click = [this] { if (onExercise) onExercise(0, exercise->rows[0].on ? 0 : 1); }; exercise->rows.push_back(r); }
    for (const char *l : {"Your usage", "MSI recommends", "Last calibration", "Next due", "Lowest in 30 days"}) { Row r; r.type = Row::Info; r.label = l; exercise->rows.push_back(r); }
    { Row r; r.type = Row::Text; r.accent = true; exercise->rows.push_back(r); }
    { Row r; r.type = Row::Button; r.label = "Start calibration"; r.accent = true; r.click = [this] { if (onExercise) onExercise(exercise->rows[7].label.startsWith("Cancel") ? 4 : 3, 0); }; exercise->rows.push_back(r); }
    { Row r; r.type = Row::Text; r.label = "Why: the battery's built-in gauge slowly loses track of its real capacity, especially when it sits at one level. MSI's own tool fixes that with a full charge, then a discharge, then a recharge, all with the charger connected. On supported MSI laptops this app does exactly that by itself: it charges to 100 %, tells the firmware to behave as if the charger were unplugged (the same register MSI's tool uses), discharges to 20 %, then restores the charger and your normal limit. If your laptop cannot do that, a guided manual version is used instead: it charges to 100 %, tells you when to unplug, then when to plug back in at 20 %. No memory effect: you never need to do this for any other reason. Storing the laptop for a long time? MSI suggests about 40 % charge and an EC reset."; exercise->rows.push_back(r); }
    exercise->relayout();
}
void BatteryPage::setHistory(const QVector<HistPoint> &pts, const QVector<HistEvent> &ev, const QString &note) { auto *h = static_cast<HistoryPanel *>(history); h->pts = pts; h->ev = ev; h->note = note; h->update(); }
void BatteryPage::setRecommended(int percent) { if (charge->recommended != percent) { charge->recommended = percent; charge->update(); } }
void BatteryPage::setExercise(const ExerciseView &v) {
    const QString sig = QString("%1|%2|%3|%4|%5|%6|%7|%8|%9|%10|%11").arg(v.enabled).arg(v.stage).arg(v.due).arg(v.never).arg(v.last, v.next, v.lowest).arg(v.usage, v.rule).arg(v.cap).arg(v.target) + (v.autoRun ? "A" : "M");
    if (sig == exSig) return; exSig = sig;                       // only touch the layout when something visible changed
    auto &R = exercise->rows; R[0].on = v.enabled; for (int i = 1; i <= 8; ++i) R[i].visible = v.enabled;
    R[1].value = v.usage; R[2].value = v.rule; R[3].value = v.last; R[4].value = v.next; R[5].value = v.lowest;
    R[6].label = v.autoRun && v.stage == 1 ? QString("AUTOMATIC · STEP 1 OF 2 · CHARGING TO 100 %: nothing to do, keep the charger connected. Now %1 %.").arg(v.cap)
               : v.autoRun && v.stage >= 2 ? QString("AUTOMATIC · STEP 2 OF 2 · DISCHARGING WITH THE CHARGER CONNECTED, down to %2 %. Now %1 %. Your laptop runs from the battery for a while; it recharges by itself afterwards and your normal charge limit comes back.").arg(v.cap).arg(v.target)
               : v.stage == 1 ? QString("STEP 1 OF 2 · CHARGING TO 100 %: keep the charger connected. Now %1 %. I will tell you when to unplug.").arg(v.cap)
               : v.stage == 2 ? QString("STEP 2 OF 2 · DISCHARGING: unplug the charger and use the laptop normally. Now %1 %. I will tell you when to plug back in (at %2 %).").arg(v.cap).arg(v.target)
               : v.due ? QString("CALIBRATION DUE: when it suits you, press Start. It takes one charge and one normal day of use.")
               : v.never ? QString("Nothing to do yet. Tracking has started: the first reminder comes on %1.").arg(v.next) : QString("All good. Next calibration is due %1.").arg(v.next);
    R[7].label = v.stage ? "Cancel calibration" : "Start calibration"; R[7].accent = !v.stage;
    exercise->relayout(); exercise->update(); kick();
}
void BatteryPage::setLimit(int percent, bool supported) { charge->supported = supported; charge->setValue(percent); charge->update(); }
void BatteryPage::setHealth(const QString &status, const QString &health, const QString &cycles, const QString &draw, const QString &eta, int cap) {
    const QString v[6] = {status, QString("%1 %").arg(cap), health, cycles, draw, eta}; for (int i = 0; i < 6; ++i) info->rows[i].value = v[i]; info->update();
}

// ───────────────────────────── fan curve editor ─────────────────────────────
class CurvePanel : public Panel {
public:
    CurvePanel(QWidget *p) : Panel("", "FAN CURVE", p) { setMinimumHeight(330); }
    QVector<int> sp[2], tp[2]; int fan = 0; bool enabled = false, dirty = false; double liveT[2] = {0, 0}, liveP[2] = {-1, -1};
    std::function<void()> onEdit, onApply, onReset;
protected:
    static constexpr double TMIN = 20, TMAX = 105, SMAX = 150;
    QRectF plot, bApply, bReset; int dragKind = -1, dragIdx = -1, hotKind = -1, hotIdx = -1;      // kind 0: speed handle, 1: temperature step
    double X(double t) const { return plot.left() + (t - TMIN) / (TMAX - TMIN) * plot.width(); }
    double Y(double s) const { return plot.bottom() - s / SMAX * plot.height(); }
    double segStart(int i) const { return i == 0 ? TMIN : tp[fan][i - 1]; }
    double segEnd(int i) const { return i == 6 ? TMAX : tp[fan][i]; }
    bool ready() const { return sp[fan].size() == 7 && tp[fan].size() == 6; }
    void pick(QPointF pt, int &kind, int &idx) const {
        kind = idx = -1; if (!ready()) return;
        for (int i = 0; i < 7; ++i) if (QRectF(X((segStart(i) + segEnd(i)) / 2) - 13, Y(sp[fan][i]) - 13, 26, 26).contains(pt)) { kind = 0; idx = i; return; }
        for (int k = 0; k < 6; ++k) if (QRectF(X(tp[fan][k]) - 11, plot.bottom() - 4, 22, 24).contains(pt)) { kind = 1; idx = k; return; }
    }
    void mousePressEvent(QMouseEvent *e) override {
        if (bApply.contains(e->position()) && dirty && onApply) { onApply(); return; }
        if (bReset.contains(e->position()) && onReset) { onReset(); return; }
        if (!enabled) return; pick(e->position(), dragKind, dragIdx);
    }
    void mouseMoveEvent(QMouseEvent *e) override {
        if (dragKind >= 0 && ready()) {
            if (dragKind == 0) sp[fan][dragIdx] = qBound(0, int(round((plot.bottom() - e->position().y()) / plot.height() * SMAX)), int(SMAX));
            else { int t = int(round(TMIN + (e->position().x() - plot.left()) / plot.width() * (TMAX - TMIN))); int lo = dragIdx == 0 ? int(TMIN) + 1 : tp[fan][dragIdx - 1] + 1, hi = dragIdx == 5 ? int(TMAX) - 1 : tp[fan][dragIdx + 1] - 1; tp[fan][dragIdx] = qBound(lo, t, hi); }
            dirty = true; if (onEdit) onEdit(); update(); return;
        }
        int k, i; pick(e->position(), k, i); bool btn = (bApply.contains(e->position()) && dirty) || bReset.contains(e->position());
        if (enabled && (k != hotKind || i != hotIdx)) { hotKind = k; hotIdx = i; update(); }
        setCursor(btn ? Qt::PointingHandCursor : enabled && k == 0 ? Qt::SizeVerCursor : enabled && k == 1 ? Qt::SizeHorCursor : Qt::ArrowCursor);
    }
    void mouseReleaseEvent(QMouseEvent *) override { dragKind = dragIdx = -1; update(); }
    void paintBody(QPainter &p, const QRectF &b) override {
        plot = QRectF(b.left() + 40, b.top() + 8, b.width() - 40 - 8, b.height() - 8 - 34 - 52);
        p.setFont(fnt(MONO, 9));
        for (int s = 0; s <= 150; s += 25) { double y = Y(s); p.setPen(QPen(alpha(kText, s == 0 ? 0.25 : 0.07), 1, s == 0 ? Qt::SolidLine : Qt::DashLine)); p.drawLine(QPointF(plot.left(), y), QPointF(plot.right(), y)); if (s % 50 == 0) { p.setPen(kDim); p.drawText(QRectF(b.left(), y - 7, 34, 14), Qt::AlignRight | Qt::AlignVCenter, QString("%1%").arg(s)); } }
        for (int t = 30; t <= 100; t += 10) { double x = X(t); p.setPen(QPen(alpha(kText, 0.06), 1, Qt::DashLine)); p.drawLine(QPointF(x, plot.top()), QPointF(x, plot.bottom())); p.setPen(kDim); p.drawText(QRectF(x - 16, plot.bottom() + 24, 32, 14), Qt::AlignCenter, QString("%1°").arg(t)); }
        p.save(); p.setOpacity(p.opacity() * (enabled ? 1.0 : 0.55));
        if (ready()) {
            QPainterPath step, area; for (int i = 0; i < 7; ++i) { double x0 = X(segStart(i)), x1 = X(segEnd(i)), y = Y(sp[fan][i]); if (i == 0) { step.moveTo(x0, y); area.moveTo(x0, plot.bottom()); area.lineTo(x0, y); } else { step.lineTo(x0, y); area.lineTo(x0, y); } step.lineTo(x1, y); area.lineTo(x1, y); }
            area.lineTo(X(TMAX), plot.bottom()); area.closeSubpath(); p.setPen(Qt::NoPen); p.setBrush(alpha(gAccent, 0.10)); p.drawPath(area);
            p.setBrush(Qt::NoBrush); p.setPen(QPen(gAccent, 2, Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin)); p.drawPath(step);
            for (int k = 0; k < 6; ++k) { bool hot = (hotKind == 1 && hotIdx == k) || (dragKind == 1 && dragIdx == k); double x = X(tp[fan][k]); QPolygonF d; d << QPointF(x, plot.bottom() - 2) << QPointF(x + 7, plot.bottom() + 6) << QPointF(x, plot.bottom() + 14) << QPointF(x - 7, plot.bottom() + 6);
                p.setPen(QPen(hot ? kText : gAccent, 1)); p.setBrush(hot ? gAccent : kPanel); p.drawPolygon(d); }
            for (int i = 0; i < 7; ++i) { bool hot = (hotKind == 0 && hotIdx == i) || (dragKind == 0 && dragIdx == i); double cx = X((segStart(i) + segEnd(i)) / 2), cy = Y(sp[fan][i]);
                p.setPen(QPen(gAccent, 2)); p.setBrush(hot ? gAccent : kPanel); p.drawRect(QRectF(cx - 6, cy - 6, 12, 12));
                if (hot) { QString t = QString("%1 %  @ %2°–%3°").arg(sp[fan][i]).arg(int(segStart(i))).arg(i == 6 ? QString("max") : QString::number(int(segEnd(i)))); QFont f = fnt(MONO, 10, QFont::Medium); p.setFont(f); QRectF tr(0, 0, QFontMetricsF(f).horizontalAdvance(t) + 16, 20); tr.moveTopLeft(QPointF(qBound(plot.left(), cx - tr.width() / 2, plot.right() - tr.width()), qMax(plot.top(), cy - 34))); p.setPen(QPen(kLine, 1)); p.setBrush(kBg); p.drawRect(tr); p.setPen(kText); p.drawText(tr, Qt::AlignCenter, t); } }
        }
        p.restore();
        if (liveP[fan] >= 0 && liveT[fan] > 0) {                                                  // "you are here": the current temperature and the fan speed it produces
            double x = X(qBound(TMIN, liveT[fan], TMAX)), y = Y(qBound(0.0, liveP[fan], SMAX)); p.setPen(QPen(alpha(kText, 0.5), 1, Qt::DotLine)); p.drawLine(QPointF(x, plot.top()), QPointF(x, plot.bottom()));
            p.setPen(QPen(kText, 1.5)); p.setBrush(kBg); p.drawEllipse(QPointF(x, y), 5, 5); p.setFont(fnt(MONO, 9, QFont::Medium, 1)); p.setPen(kText); p.drawText(QRectF(x + 8, y - 16, 90, 14), Qt::AlignLeft, QString("NOW %1° · %2%").arg(int(liveT[fan])).arg(int(liveP[fan])));
        }
        if (!enabled) { p.setFont(fnt(RAJ, 17, QFont::DemiBold, 2)); p.setPen(gAccent); p.drawText(QRectF(plot.left(), plot.top(), plot.width(), 30), Qt::AlignCenter, "SHOWING THE CURRENT TABLE  ·  TURN ON ADVANCED FAN CONTROL TO EDIT"); }
        p.setFont(fnt(MONO, 10)); p.setPen(kDim); p.drawText(QRectF(b.left(), b.bottom() - 32, b.width() - 300, 32), Qt::AlignLeft | Qt::AlignVCenter | Qt::TextWordWrap, "Drag the squares up/down to set a fan speed. Drag the diamonds sideways to move a temperature step.");
        bReset = QRectF(b.right() - 250, b.bottom() - 34, 110, 32); bApply = QRectF(b.right() - 130, b.bottom() - 34, 130, 32);
        p.setPen(QPen(kLine, 1)); p.setBrush(Qt::NoBrush); p.drawPath(chamfer(bReset, 8)); p.setFont(fnt(RAJ, 16, QFont::Bold, 2)); p.setPen(kText); p.drawText(bReset, Qt::AlignCenter, "RESET");
        p.setPen(QPen(dirty ? gAccent : kLine, 1)); p.setBrush(dirty ? gAccent : QBrush(Qt::NoBrush)); p.drawPath(chamfer(bApply, 8)); p.setPen(dirty ? kBg : alpha(kDim, 0.6)); p.drawText(bApply, Qt::AlignCenter, "APPLY");
    }
};

FanPage::FanPage(QWidget *p) : AnimPage(p) {
    auto *root = new QVBoxLayout(this); root->setContentsMargins(18, 16, 18, 16); root->setSpacing(10);
    head = new RowsPanel("", "FAN CONTROL", this); curve = new CurvePanel(this); 
    root->addWidget(head); root->addWidget(curve, 1); adopt(head, 0.0); adopt(curve, 0.12);
    Row sel; sel.type = Row::Segment; sel.opts = {"CPU fan", "GPU fan"}; sel.pick = [this](int i) { fan = i; head->rows[0].sel = i; curve->fan = i; head->update(); curve->update(); }; head->rows.push_back(sel);
    Row adv; adv.type = Row::Switch; adv.label = "Advanced fan control"; adv.sub = "Replace the firmware fan table with your own curve"; adv.click = [this] { if (onAdvanced) onAdvanced(!advanced); }; head->rows.push_back(adv);
    Row warn; warn.type = Row::Text; warn.label = "Aggressively low speeds can overheat the laptop. The firmware still protects the CPU, but use judgement."; head->rows.push_back(warn); head->relayout();
    curve->onEdit = [this] { if (onCurveEdited) onCurveEdited(fan, curve->sp[fan], curve->tp[fan]); };
    curve->onApply = [this] { if (onApply) onApply(); }; curve->onReset = [this] { if (onReset) onReset(); };
}
void FanPage::setCaps(const Caps &c) { two = c.fan2; head->rows[0].visible = c.fan2; if (!c.fan2) { fan = 0; curve->fan = 0; head->rows[0].sel = 0; } head->relayout(); }
void FanPage::setCurve(int f, const QVector<int> &speeds, const QVector<int> &temps) { if (f < 0 || f > 1) return; if (!curve->dirty || true) { curve->sp[f] = speeds; curve->tp[f] = temps; } curve->update(); }
void FanPage::setLive(int ct, int cp, int gt, int gp) { curve->liveT[0] = ct; curve->liveP[0] = cp; curve->liveT[1] = gt; curve->liveP[1] = gp; curve->update(); }
void FanPage::setAdvanced(bool on) { advanced = on; head->rows[1].on = on; curve->enabled = on; head->update(); curve->update(); kick(); }
void FanPage::setDirty(bool d) { curve->dirty = d; curve->update(); }

// ───────────────────────────── controls ─────────────────────────────
ControlsPage::ControlsPage(QWidget *p) : AnimPage(p) {
    auto *root = new QHBoxLayout(this); root->setContentsMargins(18, 16, 18, 16); root->setSpacing(10);
    auto *left = new QVBoxLayout; left->setSpacing(10); auto *right = new QVBoxLayout; right->setSpacing(10);
    profile = new RowsPanel("", "POWER PROFILE", this); hardware = new RowsPanel("", "HARDWARE SWITCHES", this); gpu = new RowsPanel("", "DISCRETE GPU", this);
    left->addWidget(profile); left->addWidget(hardware); left->addStretch(1); right->addWidget(gpu); right->addStretch(1);
    root->addLayout(left, 1); root->addLayout(right, 1); adopt(profile, 0.0); adopt(hardware, 0.1); adopt(gpu, 0.2);
    { Row r; r.type = Row::Switch; r.label = "Follow system power profile"; r.sub = "Sync with the desktop power slider (KDE / GNOME)"; r.click = [this] { if (onFollowPpd) onFollowPpd(!profile->rows[0].on); }; profile->rows.push_back(r); }
    { Row r; r.type = Row::Switch; r.label = "Auto-switch on AC / battery"; r.sub = "A different mode for each power source"; r.click = [this] { if (onAutoAcDc) onAutoAcDc(!profile->rows[1].on); }; profile->rows.push_back(r); }
    for (int k = 0; k < 2; ++k) { Row r; r.type = Row::Segment; r.label = k == 0 ? "On charger" : "On battery"; r.opts = modeNames(); r.visible = false; r.pick = [this, k](int i) { if (onAcDcMode) onAcDcMode(k == 0, i); }; profile->rows.push_back(r); }
    { Row r; r.type = Row::Text; r.label = "Switch modes any time on the Dashboard. These options only decide what happens automatically."; profile->rows.push_back(r); }
    profile->relayout();
    const char *lab[4] = {"Webcam", "Swap Fn and Win keys", "USB power share", "Cooler Boost"}, *sub[4] = {"Hardware switch: off disconnects the camera", "Exchange the physical key positions", "Keep USB ports powered when the laptop is off", "Run all fans at maximum speed"};
    for (int i = 0; i < 4; ++i) { Row r; r.type = Row::Switch; r.label = lab[i]; r.sub = sub[i]; r.click = [this, i] { if (onSwitch) onSwitch(Sw(i)); }; hardware->rows.push_back(r); }
    hardware->relayout(); rebuildGpuRows(QString(), QString(), QString(), 0, 0);
}
void ControlsPage::setCaps(const Caps &c) { caps = c; hardware->rows[Webcam].visible = c.webcam; hardware->rows[FnSwap].visible = c.fnSwap; hardware->rows[UsbShare].visible = c.usbShare; hardware->rows[Boost].visible = c.coolerBoost; hardware->relayout(); }
void ControlsPage::setSwitch(Sw s, bool on) { if (hardware->rows[s].on != on) { hardware->rows[s].on = on; kick(); } }
void ControlsPage::setProfileState(bool f, bool a, int c, int b) {
    bool ch = profile->rows[0].on != f || profile->rows[1].on != a || profile->rows[2].sel != c || profile->rows[3].sel != b || profile->rows[2].visible != a;
    profile->rows[0].on = f; profile->rows[1].on = a; profile->rows[2].sel = c; profile->rows[3].sel = b; profile->rows[2].visible = profile->rows[3].visible = a;
    if (ch) { profile->relayout(); kick(); }
}
void ControlsPage::setGpuBusy(const QString &msg) { gpuBusy = msg; rebuildGpuRows(gReason, gDriver, gPower, gA, gS); }
void ControlsPage::setGpu(bool known, bool supported, const QString &reason, const QString &driver, const QString &power, const QString &mode, qint64 a, qint64 s) {
    gpuHaveInfo = known; gpuSupported = supported; gpuModeOn = mode != "off"; gpuRunningOff = driver == "none" || driver.isEmpty(); gpuPending = supported && (!gpuModeOn) != gpuRunningOff;
    gReason = reason; gDriver = driver; gPower = power; gA = a; gS = s; rebuildGpuRows(reason, driver, power, a, s);
}
void ControlsPage::rebuildGpuRows(const QString &reason, const QString &driver, const QString &power, qint64 a, qint64 s) {
    gpu->rows.clear();
    if (!gpuHaveInfo || !gpuSupported) {
        Row t; t.type = Row::Text; t.label = !gpuHaveInfo ? "Looking for a discrete GPU..." : QString("Not available on this laptop: %1.").arg(reason); gpu->rows.push_back(t);
        Row n; n.type = Row::Text; n.label = "MSI's hardware MUX ('discrete graphics mode') is a BIOS setting and cannot be changed from Linux."; gpu->rows.push_back(n); gpu->relayout(); return;
    }
    Row st; st.type = Row::Info; st.label = "State"; st.accent = gpuRunningOff || power.startsWith("D3"); st.value = gpuRunningOff ? QString("OFF · no driver · %1").arg(power) : QString("%1 · %2").arg(power.startsWith("D3") ? "ASLEEP" : "AWAKE", driver); gpu->rows.push_back(st);
    if (!gpuRunningOff && a + s > 0) { Row u; u.type = Row::Info; u.label = "Awake time"; u.value = QString("%1 % since boot").arg(int(100.0 * a / double(a + s) + 0.5)); gpu->rows.push_back(u); }
    Row g; g.type = Row::Gap; gpu->rows.push_back(g);
    Row m; m.type = Row::Segment; m.label = "Boot mode"; m.sub = "Applies after the next restart"; m.opts = {"Hybrid", "Integrated only"}; m.sel = gpuModeOn ? 0 : 1; m.enabled = gpuBusy.isEmpty(); m.pick = [this](int i) { if (onGpuMode) onGpuMode(i == 0); }; gpu->rows.push_back(m);
    if (!gpuBusy.isEmpty()) { Row b; b.type = Row::Text; b.label = gpuBusy; b.accent = true; gpu->rows.push_back(b); }
    else if (gpuPending) { Row b; b.type = Row::Text; b.label = gpuModeOn ? "The discrete GPU turns ON after restart." : "The discrete GPU turns OFF after restart."; b.accent = true; gpu->rows.push_back(b); Row r; r.type = Row::Button; r.label = "Restart now"; r.accent = true; r.click = [this] { if (onReboot) onReboot(); }; gpu->rows.push_back(r); }
    Row c; c.type = Row::Text; c.label = "Integrated only lets the GPU sleep completely: best battery life and quietest fans. External displays wired to the discrete GPU (often HDMI) stop working while it is off. Nothing is ever switched off while the system runs."; gpu->rows.push_back(c);
    gpu->relayout();
}

// ───────────────────────────── about ─────────────────────────────
AboutPage::AboutPage(QWidget *p) : AnimPage(p) {
    auto *root = new QHBoxLayout(this); root->setContentsMargins(18, 16, 18, 16); root->setSpacing(10);
    auto *left = new QVBoxLayout; left->setSpacing(10); auto *right = new QVBoxLayout; right->setSpacing(10);
    credits = new RowsPanel("", "CREDITS", this); system = new RowsPanel("", "THIS SYSTEM", this); links = new RowsPanel("", "PROJECT", this);
    left->addWidget(credits); left->addStretch(1); right->addWidget(system); right->addWidget(links); right->addStretch(1);
    root->addLayout(left, 3); root->addLayout(right, 2); adopt(credits, 0.0); adopt(system, 0.1); adopt(links, 0.2);
    auto text = [&](const QString &t, bool acc = false) { Row r; r.type = Row::Text; r.label = t; r.accent = acc; credits->rows.push_back(r); };
    auto gap = [&] { Row r; r.type = Row::Gap; credits->rows.push_back(r); };
    text("MCONTROLCENTER · OPEN-SOURCE FORK", true);
    text("Maintained by p34t1k (github.com/p34t1k). Adds the telemetry dashboard and redesigned interface, capability-aware UI for any msi-ec laptop, self-healing msi-ec driver setup after kernel updates, discrete-GPU boot mode, KDE Power & Battery integration and the session D-Bus API.");
    gap(); text("ORIGINAL APPLICATION", true);
    text("By Dmitry Serov (dmitry-s93) and contributors: the EC access layer, fan control, power-profile sync and the first versions of every feature here. Licensed GPL-3.0-or-later. This fork keeps the same licence.");
    gap(); text("KERNEL DRIVER", true);
    text("msi-ec by BeardOverflow and contributors (github.com/BeardOverflow/msi-ec) is what makes all of this possible.");
    gap(); text("KEYBOARD LIGHTING PROTOCOL", true);
    text("MSI MysticLight reverse engineering by natanalt, and the community projects mystickeys, msi-mysticlight-controller-linux and OpenRGB, whose published findings this independent implementation builds on.");
    gap(); text("BATTERY GUIDANCE", true);
    text("Charge-limit names, the calibration interval (every 3 months when mostly plugged in, monthly otherwise) and the storage tip follow MSI's own support pages (msi.com: Laptop Battery; Perform Battery Calibration).");
    gap(); text("FONTS", true);
    text("Rajdhani (Indian Type Foundry) and IBM Plex Mono (IBM), both SIL Open Font License 1.1.");
    gap(); text("MSI and Micro-Star International are trademarks of their owners. This project is community-made and not affiliated with or endorsed by MSI.");
    credits->relayout();
    for (const char *l : {"Version", "Qt", "EC firmware", "msi-ec driver", "Kernel", "Desktop"}) { Row r; r.type = Row::Info; r.label = l; system->rows.push_back(r); } system->relayout();
    const char *lb[4] = {"Source code (this fork)", "Original project", "Report a problem / add my laptop", "Copy hardware report"};
    for (int i = 0; i < 4; ++i) { Row r; r.type = Row::Button; r.label = lb[i]; r.accent = i == 3; r.click = [this, i] { if (onLink) onLink(i); }; links->rows.push_back(r); } links->relayout();
}
void AboutPage::setInfo(const QString &v, const QString &qt, const QString &ec, const QString &drv, const QString &k, const QString &de) { const QString x[6] = {v, qt, ec, drv, k, de}; for (int i = 0; i < 6; ++i) system->rows[i].value = x[i]; system->update(); }

// ───────────────────────────── settings ─────────────────────────────
SettingsPage::SettingsPage(QWidget *p) : AnimPage(p) {
    auto *root = new QHBoxLayout(this); root->setContentsMargins(18, 16, 18, 16); root->setSpacing(10);
    auto *l = new QVBoxLayout; l->setSpacing(10); auto *r = new QVBoxLayout; r->setSpacing(10);
    startup = new RowsPanel("", "STARTUP & ALERTS", this); automation = new RowsPanel("", "AUTOMATION", this); data = new RowsPanel("", "DATA", this);
    l->addWidget(startup); l->addStretch(1); r->addWidget(automation); r->addWidget(data); r->addStretch(1);
    root->addLayout(l, 1); root->addLayout(r, 1); adopt(startup, 0.0); adopt(automation, 0.1); adopt(data, 0.2);
    { Row x; x.type = Row::Switch; x.label = "Start with my session"; x.sub = "Runs quietly in the system tray after login"; x.click = [this] { if (onChange) onChange(Autostart, startup->rows[0].on ? 0 : 1); }; startup->rows.push_back(x); }
    { Row x; x.type = Row::Switch; x.label = "Thermal alerts"; x.sub = "Notify me when the CPU stays hot for 10 seconds"; x.click = [this] { if (onChange) onChange(ThermalAlert, startup->rows[1].on ? 0 : 1); }; startup->rows.push_back(x); }
    { Row x; x.type = Row::Segment; x.label = "Alert above"; x.opts = {"85 °C", "90 °C", "95 °C"}; x.visible = false; x.pick = [this](int i) { if (onChange) onChange(ThermalLevel, i); }; startup->rows.push_back(x); }
    { Row x; x.type = Row::Switch; x.label = "Reduce motion"; x.sub = "No entrance or hover animations (calmer, and a little less CPU)"; x.click = [this] { if (onChange) onChange(ReduceMotion, startup->rows[3].on ? 0 : 1); }; startup->rows.push_back(x); }
    { Row x; x.type = Row::Text; x.label = "Launching MControlCenter again from the menu simply raises this window."; startup->rows.push_back(x); }
    startup->relayout();
    { Row x; x.type = Row::Switch; x.label = "Turbo while gaming"; x.sub = "Turbo + Cooler Boost while a game runs (Feral GameMode)"; x.click = [this] { if (onChange) onChange(GameMode, automation->rows[0].on ? 0 : 1); }; automation->rows.push_back(x); }
    { Row x; x.type = Row::Text; x.label = "Needs the 'gamemode' package (many launchers use it automatically). Nothing changes until a game actually requests it."; automation->rows.push_back(x); }
    { Row x; x.type = Row::Switch; x.label = "App profiles"; x.sub = "Switch mode and screen rate while chosen apps run"; x.click = [this] { if (onChange) onChange(AppProfiles, automation->rows[2].on ? 0 : 1); }; automation->rows.push_back(x); }
    { Row x; x.type = Row::Button; x.label = "Edit app profiles"; x.click = [this] { if (onChange) onChange(AppProfilesEdit, 0); }; automation->rows.push_back(x); }
    automation->relayout();
    { Row x; x.type = Row::Button; x.label = "Export last 3 min telemetry (CSV)"; x.click = [this] { if (onChange) onChange(ExportCsv, 0); }; data->rows.push_back(x); }
    { Row x; x.type = Row::Text; x.label = "Temperatures, fan and power samples to a CSV file in your home folder: handy for bug reports or comparing modes."; data->rows.push_back(x); }
    data->relayout();
}
void SettingsPage::setAppProfiles(bool on, int n) { auto &R = automation->rows; const QString l = n ? QString("Edit app profiles (%1)").arg(n) : QString("Add an app profile"); if (R[2].on == on && R[3].label == l) return; R[2].on = on; R[3].label = l; automation->relayout(); automation->update(); kick(); }
void SettingsPage::setReduceMotion(bool on) { auto &R = startup->rows; if (R[3].on == on) return; R[3].on = on; R[3].anim = on ? 1 : 0; startup->update(); kick(); }
void SettingsPage::setState(bool a, bool t, int ti, bool g, bool avail) {
    bool ch = startup->rows[0].on != a || startup->rows[1].on != t || startup->rows[2].sel != ti || startup->rows[2].visible != t || automation->rows[0].on != g || automation->rows[0].enabled != avail;
    startup->rows[0].on = a; startup->rows[1].on = t; startup->rows[2].sel = ti; startup->rows[2].visible = t; automation->rows[0].on = g; automation->rows[0].enabled = true;
    automation->rows[0].sub = avail ? "Turbo + Cooler Boost while a game runs (Feral GameMode)" : "Needs the 'gamemode' package";
    if (ch) { startup->relayout(); automation->relayout(); kick(); }
}

