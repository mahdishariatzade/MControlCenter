#include "companion.h"
#include "uikit.h"
#include "settings.h"
#include <QPainter>
#include <QMouseEvent>
#include <QMenu>
#include <QApplication>
#include <QScreen>
#include <QRandomGenerator>

using namespace kit;

namespace {
const int kSize = 86;
// A handful of short, honest tips about the app itself -- nothing about MSI's own software.
const char *kTips[] = {
    "Tip: the four power modes also have M1-M4 shortcuts on the dashboard.",
    "Tip: Battery -> calibration runs on its own; you only need to keep the charger connected when it says so.",
    "Tip: App profiles (Settings) can switch mode and refresh rate automatically per app.",
    "Tip: right-click me to put me away. The app works exactly the same either way.",
    "Tip: the Fans page lets you draw your own fan curve, then export it to reuse on another install.",
    "Tip: Displays supports DDC/CI for some external monitors over the same cable as the laptop screen.",
};
}

CompanionWidget::CompanionWidget(QWidget *parent) : QWidget(parent) {
    setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::NoDropShadowWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_ShowWithoutActivating);
    setFixedSize(kSize, kSize + 30);               // a little extra height below for a speech bubble
    setCursor(Qt::OpenHandCursor);
    setToolTip(tr("MControlCenter's little companion -- purely decorative. Right-click to hide it."));
    clock.start();
    connect(&anim, &QTimer::timeout, this, &CompanionWidget::tick);
    anim.start(1000 / 15);                          // 15 fps is plenty for a small idle sprite and costs very little
}

void CompanionWidget::placeRemembered() {
    QScreen *scr = QGuiApplication::primaryScreen(); if (!scr) return;
    const QRect avail = scr->availableGeometry();
    QPoint p(Settings().getValue("Companion/x").toInt(), Settings().getValue("Companion/y").toInt());
    if (p.isNull() || !avail.adjusted(-kSize, -kSize, kSize, kSize).contains(p))
        p = QPoint(avail.right() - kSize - 24, avail.bottom() - kSize - 48);
    move(p);
}

void CompanionWidget::setMood(Mood m, const QString &d) { if (mood != m || detail != d) { mood = m; detail = d; update(); } }

void CompanionWidget::sayTip() { bubble = QString::fromUtf8(kTips[QRandomGenerator::global()->bounded(int(sizeof(kTips) / sizeof(kTips[0])))]); bubbleT = 4.0; update(); }

void CompanionWidget::tick() {
    const double now = clock.elapsed() / 1000.0; static double last = 0; const double dt = qBound(0.001, now - last, 0.1); last = now;
    blink = fmod(now, 4.2); bob = sin(now * (mood == Boosted ? 5.0 : mood == Hot ? 3.0 : 1.4)) * (mood == Idle ? 3.0 : 5.0);
    mouth += ((mood == Happy || mood == Boosted ? 1.0 : mood == Hot || mood == Low ? -0.6 : 0.0) - mouth) * ease(dt, 0.3);
    if (bubbleT > 0) { bubbleT -= dt; if (bubbleT <= 0) bubble.clear(); }
    update();
}

void CompanionWidget::paintEvent(QPaintEvent *) {
    QPainter p(this); p.setRenderHint(QPainter::Antialiasing);
    const QColor mood_c = mood == Hot ? kHot : mood == Boosted ? gAccent : mood == Low ? kDim : mood == Happy ? QColor(120, 220, 140) : gAccent;
    const double cy = kSize / 2.0 + bob;
    // speech bubble, drawn first so it sits behind nothing
    if (!bubble.isEmpty()) {
        QFontMetrics fm(fnt(MONO, 10)); const int w = qMin(220, fm.horizontalAdvance(bubble) + 20), h = fm.height() * (1 + bubble.size() / 34) + 16;
        QRectF b(kSize / 2.0 - w, -h - 8, w, h); const double a = qMin(1.0, bubbleT);
        p.setOpacity(a); p.setPen(QPen(alpha(gAccent, 0.6), 1)); p.setBrush(alpha(kBg, 0.92)); p.drawPath(chamfer(b, 8));
        p.setFont(fnt(MONO, 10)); p.setPen(kText); p.drawText(b.adjusted(10, 6, -10, -6), Qt::TextWordWrap, bubble); p.setOpacity(1);
    }
    // body: a simple rounded diamond "core", like a small floating reactor -- our own shape, not anyone else's character
    QPainterPath body; const double r = 30;
    body.moveTo(kSize / 2.0, cy - r); body.quadTo(kSize / 2.0 + r, cy, kSize / 2.0, cy + r); body.quadTo(kSize / 2.0 - r, cy, kSize / 2.0, cy - r); body.closeSubpath();
    QRadialGradient g(QPointF(kSize / 2.0, cy), r * 1.4); g.setColorAt(0, alpha(mood_c, 0.30)); g.setColorAt(1, alpha(mood_c, 0));
    p.setPen(Qt::NoPen); p.setBrush(g); p.drawEllipse(QPointF(kSize / 2.0, cy), r * 1.4, r * 1.4);
    p.setPen(QPen(mood_c, 2)); p.setBrush(alpha(kPanel, 0.92)); p.drawPath(body);
    // eyes: two small bars, closing briefly every few seconds; a gentle curved mouth that reacts to mood
    const bool shut = blink > 3.9; const double ew = 7, eh = shut ? 1 : 9, ex = 12;
    p.setPen(Qt::NoPen); p.setBrush(kText);
    p.drawRoundedRect(QRectF(kSize / 2.0 - ex - ew / 2, cy - 10 - eh / 2, ew, eh), 3, 3);
    p.drawRoundedRect(QRectF(kSize / 2.0 + ex - ew / 2, cy - 10 - eh / 2, ew, eh), 3, 3);
    QPainterPath m; m.moveTo(kSize / 2.0 - 10, cy + 6 - mouth * 4); m.quadTo(kSize / 2.0, cy + 6 + mouth * 10, kSize / 2.0 + 10, cy + 6 - mouth * 4);
    p.setPen(QPen(mood_c, 2)); p.drawPath(m);
}

void CompanionWidget::mousePressEvent(QMouseEvent *e) {
    if (e->button() != Qt::LeftButton) return;
    dragging = true; moved = false; dragFrom = e->globalPosition().toPoint(); windowFrom = pos(); setCursor(Qt::ClosedHandCursor);
}
void CompanionWidget::mouseMoveEvent(QMouseEvent *e) {
    if (!dragging) return;
    const QPoint d = e->globalPosition().toPoint() - dragFrom; if (d.manhattanLength() > 4) moved = true;
    move(windowFrom + d);
}
void CompanionWidget::mouseReleaseEvent(QMouseEvent *) {
    dragging = false; setCursor(Qt::OpenHandCursor);
    if (moved) { Settings::setValue("Companion/x", x()); Settings::setValue("Companion/y", y()); }
    else sayTip();          // a plain click (no drag) asks for a tip
}
void CompanionWidget::contextMenuEvent(QContextMenuEvent *e) {
    QMenu menu; QAction *hide = menu.addAction(tr("Hide companion (Settings -> Automation to bring it back)"));
    if (menu.exec(e->globalPos()) == hide) emit QWidget::destroyed(), this->close();   // the owner (MainWindow) also turns the Settings switch off via its onHide hook
}
