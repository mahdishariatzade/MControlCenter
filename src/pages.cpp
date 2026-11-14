#include "pages.h"
#include "sensors.h"
#include "uikit.h"
#include <QVBoxLayout>
#include <QHBoxLayout>

using namespace kit;

// ───────────────────────────── page animation driver ─────────────────────────────
AnimPage::AnimPage(QWidget *p) : QWidget(p) { setObjectName("page"); timer.setInterval(16); timer.setTimerType(Qt::PreciseTimer); connect(&timer, &QTimer::timeout, this, [this] { tick(); }); }
void AnimPage::adopt(Panel *pn, double delay) { pn->delay = delay; pn->wake = [this] { kick(); }; panels.push_back(pn); }
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
    ChargePanel(QWidget *p) : Panel("01", "CHARGE LIMIT", p) { setMinimumHeight(300); }
    int limit = 100, drag = -1; bool supported = true; std::function<void(int)> onCommit;
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
        const int pv[3] = {60, 80, 100}; const char *nm[3] = {"BEST BATTERY", "BALANCED", "FULL CAPACITY"}; double cw = (b.width() - 12) / 3;
        for (int i = 0; i < 3; ++i) {
            chips[i] = QRectF(b.left() + i * (cw + 6), b.bottom() - 46, cw, 46); bool on = limit == pv[i];
            p.setPen(QPen(on ? gAccent : kLine, 1)); p.setBrush(on ? gAccent : QBrush(Qt::NoBrush)); p.drawPath(chamfer(chips[i], 9));
            p.setFont(fnt(RAJ, 22, QFont::Bold, 1)); p.setPen(on ? kBg : (supported ? kText : kDim)); p.drawText(QRectF(chips[i].left() + 14, chips[i].top() + 4, cw, 24), Qt::AlignLeft | Qt::AlignVCenter, QString("%1 %").arg(pv[i]));
            p.setFont(fnt(MONO, 9, QFont::Normal, 1)); p.setPen(on ? alpha(kBg, 0.75) : kDim); p.drawText(QRectF(chips[i].left() + 14, chips[i].top() + 27, cw, 14), Qt::AlignLeft | Qt::AlignVCenter, nm[i]);
        }
    }
};

BatteryPage::BatteryPage(QWidget *p) : AnimPage(p) {
    auto *root = new QHBoxLayout(this); root->setContentsMargins(18, 16, 18, 16); root->setSpacing(10);
    charge = new ChargePanel(this); info = new RowsPanel("02", "BATTERY", this);
    charge->onCommit = [this](int v) { if (onLimit) onLimit(v); };
    charge->setFixedHeight(300);
    auto *l = new QVBoxLayout; l->addWidget(charge); l->addStretch(1); auto *r = new QVBoxLayout; r->addWidget(info); r->addStretch(1);
    root->addLayout(l, 3); root->addLayout(r, 2); adopt(charge, 0.0); adopt(info, 0.1);
    for (auto [l, k] : std::vector<std::pair<QString, int>>{{"Status", 0}, {"Charge", 1}, {"Health", 2}, {"Cycles", 3}, {"Draw", 4}, {"Time", 5}}) { Row r; r.type = Row::Info; r.label = l; info->rows.push_back(r); (void) k; }
    Row g; g.type = Row::Gap; info->rows.push_back(g);
    Row t; t.type = Row::Text; t.label = "Tip: 60 % is the kindest setting for a laptop that lives on its charger. Choose 100 % before a trip. Changes apply instantly."; info->rows.push_back(t); info->relayout();
}
void BatteryPage::setLimit(int percent, bool supported) { charge->supported = supported; charge->setValue(percent); charge->update(); }
void BatteryPage::setHealth(const QString &status, const QString &health, const QString &cycles, const QString &draw, const QString &eta, int cap) {
    const QString v[6] = {status, QString("%1 %").arg(cap), health, cycles, draw, eta}; for (int i = 0; i < 6; ++i) info->rows[i].value = v[i]; info->update();
}

// ───────────────────────────── fan curve editor ─────────────────────────────
class CurvePanel : public Panel {
public:
    CurvePanel(QWidget *p) : Panel("01", "FAN CURVE", p) { setMinimumHeight(330); }
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
    head = new RowsPanel("01", "FAN CONTROL", this); curve = new CurvePanel(this); curve->idx = "02";
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
    profile = new RowsPanel("01", "POWER PROFILE", this); hardware = new RowsPanel("02", "HARDWARE SWITCHES", this); gpu = new RowsPanel("03", "DISCRETE GPU", this);
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
    credits = new RowsPanel("01", "CREDITS", this); system = new RowsPanel("02", "THIS SYSTEM", this); links = new RowsPanel("03", "PROJECT", this);
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
    startup = new RowsPanel("01", "STARTUP & ALERTS", this); automation = new RowsPanel("02", "AUTOMATION", this); data = new RowsPanel("03", "DATA", this);
    l->addWidget(startup); l->addStretch(1); r->addWidget(automation); r->addWidget(data); r->addStretch(1);
    root->addLayout(l, 1); root->addLayout(r, 1); adopt(startup, 0.0); adopt(automation, 0.1); adopt(data, 0.2);
    { Row x; x.type = Row::Switch; x.label = "Start with my session"; x.sub = "Runs quietly in the system tray after login"; x.click = [this] { if (onChange) onChange(Autostart, startup->rows[0].on ? 0 : 1); }; startup->rows.push_back(x); }
    { Row x; x.type = Row::Switch; x.label = "Thermal alerts"; x.sub = "Notify me when the CPU stays hot for 10 seconds"; x.click = [this] { if (onChange) onChange(ThermalAlert, startup->rows[1].on ? 0 : 1); }; startup->rows.push_back(x); }
    { Row x; x.type = Row::Segment; x.label = "Alert above"; x.opts = {"85 °C", "90 °C", "95 °C"}; x.visible = false; x.pick = [this](int i) { if (onChange) onChange(ThermalLevel, i); }; startup->rows.push_back(x); }
    { Row x; x.type = Row::Text; x.label = "Launching MControlCenter again from the menu simply raises this window."; startup->rows.push_back(x); }
    startup->relayout();
    { Row x; x.type = Row::Switch; x.label = "Turbo while gaming"; x.sub = "Turbo + Cooler Boost while a game runs (Feral GameMode)"; x.click = [this] { if (onChange) onChange(GameMode, automation->rows[0].on ? 0 : 1); }; automation->rows.push_back(x); }
    { Row x; x.type = Row::Text; x.label = "Needs the 'gamemode' package (many launchers use it automatically). Nothing changes until a game actually requests it."; automation->rows.push_back(x); }
    automation->relayout();
    { Row x; x.type = Row::Button; x.label = "Export last 3 min telemetry (CSV)"; x.click = [this] { if (onChange) onChange(ExportCsv, 0); }; data->rows.push_back(x); }
    { Row x; x.type = Row::Text; x.label = "Temperatures, fan and power samples to a CSV file in your home folder: handy for bug reports or comparing modes."; data->rows.push_back(x); }
    data->relayout();
}
void SettingsPage::setState(bool a, bool t, int ti, bool g, bool avail) {
    bool ch = startup->rows[0].on != a || startup->rows[1].on != t || startup->rows[2].sel != ti || startup->rows[2].visible != t || automation->rows[0].on != g || automation->rows[0].enabled != avail;
    startup->rows[0].on = a; startup->rows[1].on = t; startup->rows[2].sel = ti; startup->rows[2].visible = t; automation->rows[0].on = g; automation->rows[0].enabled = true;
    automation->rows[0].sub = avail ? "Turbo + Cooler Boost while a game runs (Feral GameMode)" : "Needs the 'gamemode' package";
    if (ch) { startup->relayout(); automation->relayout(); kick(); }
}

// ───────────────────────────── lighting ─────────────────────────────
namespace {
const QColor kQuick[10] = {QColor(255, 40, 10), QColor(255, 120, 0), QColor(255, 200, 0), QColor(120, 255, 40), QColor(0, 255, 120), QColor(0, 220, 255), QColor(40, 110, 255), QColor(150, 60, 255), QColor(255, 60, 200), QColor(255, 255, 255)};
QColor effectiveZoneColor(const RgbProfile &p, int z) {
    switch (p.mode) { case RgbProfile::Off: return QColor(30, 31, 34); case RgbProfile::Static: return p.color; case RgbProfile::Zones: return p.zones.value(z, p.color);
        default: { int n = qMax(1, int(p.palette.size())); return p.palette[(z / 4 + (z % 4)) % n]; } }       // effects: a stand-in preview from the palette
}
}

// 24-zone map of the keyboard: 6 rows x 4 zones. Click / drag to select, then colour them.
class ZoneMatrix : public Panel {
public:
    ZoneMatrix(QWidget *p) : Panel("01", "KEYBOARD", p) { setMinimumHeight(330); }
    RgbProfile *prof = nullptr; int zones = 24; bool present = false; quint32 sel = 0xFFFFFFu; std::function<void()> onSelect;
    void selectAllOnce() { sel = zones == 24 ? 0xFFFFFFu : 0xFu; update(); }
protected:
    QRectF cells[24], rowLabel[6]; QRectF bAll; int dragStart = -1; bool dragAdd = true;
    int rows() const { return qMax(1, zones / 4); }
    int cellAt(QPointF pt) const { for (int i = 0; i < zones; ++i) if (cells[i].contains(pt)) return i; return -1; }
    void mousePressEvent(QMouseEvent *e) override {
        if (bAll.contains(e->position())) { sel = zones == 24 ? 0xFFFFFFu : 0xFu; if (onSelect) onSelect(); update(); return; }
        for (int r = 0; r < rows(); ++r) if (rowLabel[r].contains(e->position())) { sel = 0xFu << (r * 4); if (onSelect) onSelect(); update(); return; }
        int c = cellAt(e->position()); if (c < 0) return; dragStart = c; dragAdd = !(sel & (1u << c)) || !(e->modifiers() & Qt::ControlModifier);
        if (e->modifiers() & Qt::ControlModifier) sel ^= (1u << c); else sel = 1u << c; if (onSelect) onSelect(); update();
    }
    void mouseMoveEvent(QMouseEvent *e) override {
        int c = cellAt(e->position()); setCursor(c >= 0 || bAll.contains(e->position()) ? Qt::PointingHandCursor : Qt::ArrowCursor);
        if (dragStart >= 0 && c >= 0 && !(sel & (1u << c))) { sel |= 1u << c; if (onSelect) onSelect(); update(); }
    }
    void mouseReleaseEvent(QMouseEvent *) override { dragStart = -1; }
    void paintBody(QPainter &p, const QRectF &b) override {
        static const char *lab[6] = {"ESC", "TAB-1", "TAB", "CAPS", "SHIFT", "CTRL"};
        bAll = QRectF(b.right() - 96, b.top() - 30, 90, 22); p.setPen(QPen(kLine, 1)); p.setBrush(QBrush(Qt::NoBrush)); p.drawPath(chamfer(bAll, 5)); p.setFont(fnt(RAJ, 13, QFont::Bold, 1.5)); p.setPen(kText); p.drawText(bAll, Qt::AlignCenter, "SELECT ALL");
        const int R = rows(); const double lw = 54, gap = 5, ch = (b.height() - gap * (R - 1) - 26) / R, cw = (b.width() - lw - gap * 4) / 4;
        for (int r = 0; r < R; ++r) {
            double y = b.top() + r * (ch + gap); rowLabel[r] = QRectF(b.left(), y, lw - 6, ch);
            p.setFont(fnt(MONO, 9, QFont::Medium, 1)); p.setPen(kDim); p.drawText(rowLabel[r], Qt::AlignLeft | Qt::AlignVCenter, zones == 24 ? QString(lab[r]).replace("TAB-1", "`") : "ZONES");
            for (int c = 0; c < 4; ++c) {
                int z = r * 4 + c; QRectF cr(b.left() + lw + c * (cw + gap), y, cw, ch); cells[z] = cr; bool on = sel & (1u << z);
                QColor col = prof ? effectiveZoneColor(*prof, z) : kDim; if (prof && prof->brightness < 100) col = QColor(col.red() * (30 + prof->brightness * 70 / 100) / 100, col.green() * (30 + prof->brightness * 70 / 100) / 100, col.blue() * (30 + prof->brightness * 70 / 100) / 100);
                p.setPen(QPen(on ? kText : kLine, on ? 2 : 1)); p.setBrush(col); p.drawPath(chamfer(cr, 7));
                if (on) { p.setPen(Qt::NoPen); p.setBrush(kText); p.drawRect(QRectF(cr.right() - 14, cr.top() + 6, 6, 6)); }
            }
        }
        p.setFont(fnt(MONO, 10)); p.setPen(present ? kDim : gAccent);
        p.drawText(QRectF(b.left(), b.bottom() - 18, b.width(), 18), Qt::AlignLeft | Qt::AlignVCenter, present ? "Click or drag to select, then pick a colour." : "No keyboard controller connected.");
    }
};

// HSV colour picker: saturation/value square, hue bar, hex read-out, quick swatches and (for effects) a palette of up to 4 colours.
class ColorPanel : public Panel {
public:
    ColorPanel(QWidget *p) : Panel("02", "COLOUR", p) { setMinimumHeight(330); }
    QColor color = QColor(255, 75, 31); QVector<QColor> palette; int palSel = 0; bool showPalette = false; std::function<void(const QColor &)> onColor; std::function<void(int)> onPalSelect; std::function<void(bool)> onPalEdit;
    int h = 15, s = 255, v = 255;
    void setColor(const QColor &c) { color = c; int hh, ss, vv; c.getHsv(&hh, &ss, &vv); if (hh >= 0) h = hh; s = ss; v = vv; update(); }
protected:
    QRectF sv, hue, quick[10], pal[4], bAdd, bDel; int drag = 0;
    void setFromSV(QPointF pt) { s = qBound(0.0, (pt.x() - sv.left()) / sv.width(), 1.0) * 255; v = (1 - qBound(0.0, (pt.y() - sv.top()) / sv.height(), 1.0)) * 255; color = QColor::fromHsv(h, s, v); if (onColor) onColor(color); update(); }
    void setFromHue(QPointF pt) { h = qBound(0.0, (pt.x() - hue.left()) / hue.width(), 0.999) * 360; color = QColor::fromHsv(h, s, v); if (onColor) onColor(color); update(); }
    void mousePressEvent(QMouseEvent *e) override {
        QPointF pt = e->position(); if (sv.contains(pt)) { drag = 1; setFromSV(pt); return; } if (hue.adjusted(0, -6, 0, 6).contains(pt)) { drag = 2; setFromHue(pt); return; }
        for (int i = 0; i < 10; ++i) if (quick[i].contains(pt)) { setColor(kQuick[i]); if (onColor) onColor(color); return; }
        if (showPalette) { for (int i = 0; i < palette.size(); ++i) if (pal[i].contains(pt) && onPalSelect) { onPalSelect(i); return; } if (bAdd.contains(pt) && palette.size() < 4 && onPalEdit) onPalEdit(true); if (bDel.contains(pt) && palette.size() > 1 && onPalEdit) onPalEdit(false); }
    }
    void mouseMoveEvent(QMouseEvent *e) override { if (drag == 1) setFromSV(e->position()); else if (drag == 2) setFromHue(e->position()); }
    void mouseReleaseEvent(QMouseEvent *) override { drag = 0; }
    void paintBody(QPainter &p, const QRectF &b) override {
        double pw = showPalette ? 52 : 0; sv = QRectF(b.left(), b.top(), b.width(), b.height() - 150 - pw); hue = QRectF(b.left(), sv.bottom() + 16, b.width(), 14);
        QLinearGradient gh(sv.left(), 0, sv.right(), 0); gh.setColorAt(0, Qt::white); gh.setColorAt(1, QColor::fromHsv(h, 255, 255)); p.setPen(Qt::NoPen); p.setBrush(gh); p.drawRect(sv);
        QLinearGradient gv(0, sv.top(), 0, sv.bottom()); gv.setColorAt(0, QColor(0, 0, 0, 0)); gv.setColorAt(1, Qt::black); p.setBrush(gv); p.drawRect(sv);
        QPointF hp(sv.left() + s / 255.0 * sv.width(), sv.top() + (1 - v / 255.0) * sv.height()); p.setBrush(QBrush(Qt::NoBrush)); p.setPen(QPen(Qt::white, 2)); p.drawEllipse(hp, 7, 7); p.setPen(QPen(Qt::black, 1)); p.drawEllipse(hp, 9, 9);
        QLinearGradient hg(hue.left(), 0, hue.right(), 0); for (int i = 0; i <= 6; ++i) hg.setColorAt(i / 6.0, QColor::fromHsv(i == 6 ? 359 : i * 60, 255, 255)); p.setPen(Qt::NoPen); p.setBrush(hg); p.drawRect(hue);
        double hx = hue.left() + h / 360.0 * hue.width(); p.setPen(QPen(kText, 3)); p.drawLine(QPointF(hx, hue.top() - 5), QPointF(hx, hue.bottom() + 5));
        double y = hue.bottom() + 16; double qw = (b.width() - 9 * 4) / 10; for (int i = 0; i < 10; ++i) { quick[i] = QRectF(b.left() + i * (qw + 4), y, qw, 20); p.setPen(QPen(kLine, 1)); p.setBrush(kQuick[i]); p.drawRect(quick[i]); }
        y += 30; p.setFont(fnt(MONO, 12, QFont::Medium)); p.setPen(kText); p.drawText(QRectF(b.left(), y, 140, 22), Qt::AlignLeft | Qt::AlignVCenter, color.name().toUpper());
        p.setPen(Qt::NoPen); p.setBrush(color); p.drawRect(QRectF(b.right() - 60, y, 60, 22));
        p.setFont(fnt(MONO, 10)); p.setPen(kDim); p.drawText(QRectF(b.left() + 100, y, b.width() - 170, 22), Qt::AlignLeft | Qt::AlignVCenter, QString("H %1  S %2  V %3").arg(h).arg(s * 100 / 255).arg(v * 100 / 255));
        if (showPalette) {
            y += 34; p.setFont(fnt(MONO, 9, QFont::Medium, 1)); p.setPen(kDim); p.drawText(QRectF(b.left(), y - 14, 200, 12), Qt::AlignLeft, "EFFECT COLOURS");
            for (int i = 0; i < palette.size() && i < 4; ++i) { pal[i] = QRectF(b.left() + i * 40, y, 34, 22); p.setPen(QPen(i == palSel ? gAccent : kLine, i == palSel ? 2 : 1)); p.setBrush(palette[i]); p.drawRect(pal[i]); }
            bAdd = QRectF(b.right() - 90, y, 42, 22); bDel = QRectF(b.right() - 44, y, 42, 22);
            for (auto *r : {&bAdd, &bDel}) { p.setPen(QPen(kLine, 1)); p.setBrush(QBrush(Qt::NoBrush)); p.drawPath(chamfer(*r, 5)); }
            p.setFont(fnt(RAJ, 16, QFont::Bold)); p.setPen(palette.size() < 4 ? kText : alpha(kDim, 0.5)); p.drawText(bAdd, Qt::AlignCenter, "+"); p.setPen(palette.size() > 1 ? kText : alpha(kDim, 0.5)); p.drawText(bDel, Qt::AlignCenter, "–");
        }
    }
};

// A grid of one-click looks, each drawn as a swatch strip of what it does.
class PresetPanel : public Panel {
public:
    struct Preset { QString name; RgbProfile p; };
    PresetPanel(QWidget *p) : Panel("03", "PRESETS", p) { setMinimumHeight(250); }
    std::vector<Preset> items; std::function<void(const RgbProfile &)> onPick;
protected:
    QRectF tiles[12]; int hot = -1;
    void mousePressEvent(QMouseEvent *e) override { for (size_t i = 0; i < items.size(); ++i) if (tiles[i].contains(e->position()) && onPick) onPick(items[i].p); }
    void mouseMoveEvent(QMouseEvent *e) override { int h = -1; for (size_t i = 0; i < items.size(); ++i) if (tiles[i].contains(e->position())) h = int(i); if (h != hot) { hot = h; setCursor(h >= 0 ? Qt::PointingHandCursor : Qt::ArrowCursor); update(); } }
    void paintBody(QPainter &p, const QRectF &b) override {
        const int cols = 2; double gap = 6, tw = (b.width() - gap) / cols, th = 44; int n = int(items.size());
        for (int i = 0; i < n; ++i) {
            QRectF t(b.left() + (i % cols) * (tw + gap), b.top() + (i / cols) * (th + gap), tw, th); tiles[i] = t; bool h = i == hot;
            p.setPen(QPen(h ? gAccent : kLine, 1)); p.setBrush(h ? alpha(gAccent, 0.10) : QBrush(Qt::NoBrush)); p.drawPath(chamfer(t, 8));
            p.setFont(fnt(RAJ, 15, QFont::Bold, 1.2)); p.setPen(kText); p.drawText(QRectF(t.left() + 10, t.top() + 3, tw - 20, 20), Qt::AlignLeft | Qt::AlignVCenter, items[i].name.toUpper());
            const RgbProfile &pp = items[i].p; int cnt = pp.mode == RgbProfile::Zones ? 6 : pp.mode == RgbProfile::Static ? 1 : int(pp.palette.size());
            double sx = t.left() + 10, sw = (tw - 20) / qMax(cnt, 1); QLinearGradient g(sx, 0, sx + sw * cnt, 0);
            for (int k = 0; k < cnt; ++k) { QColor c = pp.mode == RgbProfile::Zones ? pp.zones[k * 4] : pp.mode == RgbProfile::Static ? pp.color : pp.palette[k]; if (cnt == 1) { g.setColorAt(0, c); g.setColorAt(1, c); } else g.setColorAt(k / double(cnt - 1), c); }
            p.setPen(Qt::NoPen); p.setBrush(pp.mode == RgbProfile::Zones ? QBrush(Qt::NoBrush) : QBrush(g)); if (pp.mode != RgbProfile::Zones) p.drawRect(QRectF(sx, t.top() + 27, sw * cnt, 8));
            else for (int k = 0; k < 6; ++k) { p.setBrush(pp.zones[k * 4]); p.drawRect(QRectF(sx + k * sw + 1, t.top() + 27, sw - 2, 8)); }
        }
    }
};

LightingPage::LightingPage(QWidget *p) : AnimPage(p) {
    auto *root = new QHBoxLayout(this); root->setContentsMargins(18, 16, 18, 16); root->setSpacing(10);
    auto *c1 = new QVBoxLayout; c1->setSpacing(10); auto *c2 = new QVBoxLayout; c2->setSpacing(10); auto *c3 = new QVBoxLayout; c3->setSpacing(10);
    matrix = new ZoneMatrix(this); presets = new PresetPanel(this); effect = new RowsPanel("04", "EFFECT", this); colour = new ColorPanel(this); follow = new RowsPanel("05", "FOLLOW THE LAPTOP", this);
    matrix->idx = "01"; colour->idx = "03"; effect->idx = "02"; presets->idx = "04"; follow->idx = "05";
    c1->addWidget(matrix, 3); c1->addWidget(presets, 2); c2->addWidget(effect); c2->addWidget(colour, 1); c3->addWidget(follow); c3->addStretch(1);
    root->addLayout(c1, 4); root->addLayout(c2, 4); root->addLayout(c3, 4);
    adopt(matrix, 0.0); adopt(effect, 0.08); adopt(follow, 0.16); adopt(colour, 0.12); adopt(presets, 0.2);
    matrix->prof = &prof;
    matrix->onSelect = [this] { if (prof.mode != RgbProfile::Zones && prof.mode != RgbProfile::Static) {} update(); };

    auto setMode = [this](RgbProfile::Mode m) { prof.mode = m; if (m == RgbProfile::Zones) { for (auto &z : prof.zones) if (!z.isValid()) z = prof.color; } syncRows(); changed(); };
    { Row r; r.type = Row::Segment; r.label = "Effect"; r.opts = {"Static", "Zones", "Off"}; r.pick = [setMode](int i) { setMode(i == 0 ? RgbProfile::Static : i == 1 ? RgbProfile::Zones : RgbProfile::Off); }; effect->rows.push_back(r); }
    { Row r; r.type = Row::Segment; r.opts = {"Breathing", "Cycle", "Wave"}; r.sel = -1; r.pick = [setMode](int i) { setMode(i == 0 ? RgbProfile::Breathing : i == 1 ? RgbProfile::Cycle : RgbProfile::Wave); }; effect->rows.push_back(r); }
    { Row r; r.type = Row::Segment; r.label = "Brightness"; r.opts = {"25%", "50%", "75%", "100%"}; r.pick = [this](int i) { prof.brightness = 25 * (i + 1); syncRows(); changed(); }; effect->rows.push_back(r); }
    { Row r; r.type = Row::Segment; r.label = "Speed"; r.opts = {"Slow", "Medium", "Fast"}; r.pick = [this](int i) { prof.speed = i; syncRows(); changed(); }; effect->rows.push_back(r); }
    { Row r; r.type = Row::Segment; r.label = "Wave direction"; r.opts = {"Right to left", "Left to right"}; r.pick = [this](int i) { prof.direction = i; syncRows(); changed(); }; effect->rows.push_back(r); }
    effect->relayout();

    colour->onColor = [this](const QColor &c) { applyColor(c); };
    colour->onPalSelect = [this](int i) { palSel = i; colour->palSel = i; colour->setColor(prof.palette.value(i, prof.color)); };
    colour->onPalEdit = [this](bool add) { if (add && prof.palette.size() < 4) prof.palette << colour->color; else if (!add && prof.palette.size() > 1) prof.palette.removeLast(); palSel = qMin(palSel, int(prof.palette.size()) - 1); syncRows(); changed(); };

    // presets
    auto mk = [](const char *n, RgbProfile::Mode m, QColor c, QVector<QColor> pal = {}, int speed = 1, int br = 100) { RgbProfile x; x.mode = m; x.color = c; x.zones = QVector<QColor>(24, c); if (!pal.isEmpty()) x.palette = pal; x.speed = speed; x.brightness = br; return PresetPanel::Preset{n, x}; };
    presets->items.push_back(mk("MSI Dragon", RgbProfile::Static, QColor(255, 40, 10)));
    presets->items.push_back(mk("Ice", RgbProfile::Static, QColor(110, 190, 255)));
    presets->items.push_back(mk("Matrix", RgbProfile::Static, QColor(0, 255, 80)));
    presets->items.push_back(mk("Stealth", RgbProfile::Static, QColor(255, 255, 255), {}, 1, 25));
    presets->items.push_back(mk("Sunset", RgbProfile::Wave, QColor(255, 80, 0), {QColor(255, 90, 0), QColor(255, 0, 110), QColor(120, 0, 255)}, 1));
    presets->items.push_back(mk("Aurora", RgbProfile::Cycle, QColor(0, 255, 140), {QColor(0, 255, 140), QColor(0, 200, 255), QColor(160, 60, 255)}, 0));
    presets->items.push_back(mk("Heartbeat", RgbProfile::Breathing, QColor(255, 20, 20), {QColor(255, 20, 20)}, 1));
    { auto g = mk("Horizon", RgbProfile::Zones, QColor(255, 0, 0)); const QColor rowc[6] = {QColor(255, 30, 30), QColor(255, 130, 0), QColor(255, 220, 0), QColor(60, 255, 90), QColor(0, 200, 255), QColor(120, 70, 255)}; for (int z = 0; z < 24; ++z) g.p.zones[z] = rowc[z / 4]; presets->items.push_back(g); }
    presets->onPick = [this](const RgbProfile &pr) { int b = prof.brightness; prof = pr; if (pr.brightness == 100) prof.brightness = b; syncRows(); changed(); };

    // follow the laptop
    const char *fl[4] = {"Follow power mode", "Flash on overheat", "Pulse on low battery", "Off when locked"};
    const char *fs[4] = {"Orange / white / blue / green by mode", "Red flashes when the CPU stays hot", "Slow red breathing under 15 %", "Dark while locked, back on unlock"};
    for (int i = 0; i < 4; ++i) { Row r; r.type = Row::Switch; r.label = fl[i]; r.sub = fs[i]; r.click = [this, i] { if (onFollow) onFollow(i, !follow->rows[i].on); }; follow->rows.push_back(r); }
    { Row r; r.type = Row::Gap; follow->rows.push_back(r); }
    { Row r; r.type = Row::Button; r.label = "Restore stock lighting"; r.click = [this] { if (onRestoreFirmware) onRestoreFirmware(); }; follow->rows.push_back(r); }
    { Row r; r.type = Row::Text; r.label = "Lighting is never written to the keyboard's memory. MControlCenter re-applies it at login and after sleep; a reboot without it restores the factory look."; follow->rows.push_back(r); }
    follow->relayout();
}
void LightingPage::setDevice(bool pr, int z, const QString &name, const QString &error) {
    const int old = matrix->zones; present = pr; zones = z; matrix->present = pr; matrix->zones = pr ? z : 24; (void) name; (void) error;
    if (matrix->zones != old) matrix->selectAllOnce(); matrix->update(); kick();
}
void LightingPage::setProfile(const RgbProfile &p) { prof = p; syncRows(); }
void LightingPage::setFollow(bool m, bool t, bool l, bool k) { bool v[4] = {m, t, l, k}; for (int i = 0; i < 4; ++i) follow->rows[i].on = v[i]; follow->update(); kick(); }
void LightingPage::syncRows() {
    const RgbProfile::Mode m = prof.mode; effect->rows[0].sel = m == RgbProfile::Static ? 0 : m == RgbProfile::Zones ? 1 : m == RgbProfile::Off ? 2 : -1;
    effect->rows[1].sel = m == RgbProfile::Breathing ? 0 : m == RgbProfile::Cycle ? 1 : m == RgbProfile::Wave ? 2 : -1;
    effect->rows[2].sel = qBound(0, prof.brightness / 25 - 1, 3); effect->rows[3].sel = prof.speed; effect->rows[3].visible = m >= RgbProfile::Breathing && m <= RgbProfile::Wave;
    effect->rows[4].sel = prof.direction; effect->rows[4].visible = m == RgbProfile::Wave; effect->relayout();
    const bool fx = m >= RgbProfile::Breathing && m <= RgbProfile::Wave; colour->showPalette = fx; colour->palette = prof.palette; colour->palSel = qMin(palSel, int(prof.palette.size()) - 1);
    colour->setColor(fx ? prof.palette.value(colour->palSel, prof.color) : m == RgbProfile::Zones ? prof.zones.value(qMax(0, int(qCountTrailingZeroBits(matrix->sel | (1u << 31)))), prof.color) : prof.color);
    matrix->update(); colour->update(); kick();
}
void LightingPage::applyColor(const QColor &c) {
    switch (prof.mode) {
        case RgbProfile::Zones: for (int z = 0; z < 24; ++z) if (matrix->sel & (1u << z)) prof.zones[z] = c; break;
        case RgbProfile::Breathing: case RgbProfile::Cycle: case RgbProfile::Wave: if (palSel < prof.palette.size()) prof.palette[palSel] = c; break;
        case RgbProfile::Off: prof.mode = RgbProfile::Static; prof.color = c; syncRows(); break;
        default: prof.color = c; break;
    }
    colour->palette = prof.palette; changed();
}
void LightingPage::changed() { matrix->update(); colour->update(); if (onProfile) onProfile(prof); }
