// A small, optional, purely-for-fun desktop companion. This is our OWN original drawing (geometric, in the
// app's neon theme via kit::), NOT MSI's "MSI Companion" mascot art: that is MSI's copyrighted sprite work and
// this project never copies or redistributes it. Reacts loosely to the laptop's state; never reads or writes
// anything itself (MainWindow feeds it a mood string). Off by default: it's a toy, not a feature.
#ifndef COMPANION_H
#define COMPANION_H
#include <QWidget>
#include <QTimer>
#include <QElapsedTimer>
#include <QPoint>
#include <functional>

class CompanionWidget : public QWidget {
    Q_OBJECT
public:
    explicit CompanionWidget(QWidget *parent = nullptr);
    enum Mood { Idle, Happy, Hot, Boosted, Low };
    void setMood(Mood m, const QString &detail = QString());
    void notify(const QString &text, double seconds = 6.0);   // a one-off message pushed in from outside (e.g. a BIOS-update hint), bypassing the mood cooldown
    void placeRemembered();         // restores the saved screen position, or a sensible default
    std::function<void()> onActivate;   // double-click: MainWindow wires this to raise the main window
protected:
    void paintEvent(QPaintEvent *) override;
    void mousePressEvent(QMouseEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;
    void mouseDoubleClickEvent(QMouseEvent *e) override;
    void contextMenuEvent(QContextMenuEvent *e) override;
private:
    void tick();
    QTimer anim; QElapsedTimer clock;
    QPoint dragFrom; QPoint windowFrom; bool dragging = false, moved = false;
    Mood mood = Idle; QString detail; double blink = 0, bob = 0, mouth = 0, squashX = 1, squashY = 1;
    QString bubble; double bubbleT = 0;
    double announcedAt[5] = {-1e9, -1e9, -1e9, -1e9, -1e9};   // per-Mood cooldown clock for the automatic "heads up" bubbles; starts far in the past so the first real transition always announces
    void sayTip(); void announceMoodChange(Mood from, Mood to);
};
#endif
