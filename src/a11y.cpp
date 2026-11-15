#include "a11y.h"
#include "uikit.h"
#include <QAccessibleWidget>
#include <QAccessibleActionInterface>
#include <QAccessibleValueInterface>
#include <QHash>
#include <QPointer>

using namespace kit;

namespace {

class HostAccessible;

// One virtual child (a switch, a button, a slider, an option...). It holds no state of its own: every query re-reads the host, so it is never stale.
class ChildAccessible : public QAccessibleInterface, public QAccessibleActionInterface, public QAccessibleValueInterface {
public:
    ChildAccessible(HostAccessible *par, QWidget *w, int idx) : parentIf(par), widget(w), index(idx) {}
    bool isValid() const override { A11yEntry e; return !widget.isNull() && get(e); }
    QObject *object() const override { return nullptr; }
    QWindow *window() const override { return widget ? widget->windowHandle() : nullptr; }
    QAccessibleInterface *parent() const override;
    QAccessibleInterface *child(int) const override { return nullptr; }
    int childCount() const override { return 0; }
    int indexOfChild(const QAccessibleInterface *) const override { return -1; }
    QAccessibleInterface *childAt(int, int) const override { return nullptr; }
    QAccessibleInterface *focusChild() const override { return nullptr; }
    QRect rect() const override { A11yEntry e; if (!get(e) || !widget) return {}; return QRect(widget->mapToGlobal(e.rect.topLeft()), e.rect.size()); }
    QString text(QAccessible::Text t) const override {
        A11yEntry e; if (!get(e)) return {};
        switch (t) { case QAccessible::Name: return e.name; case QAccessible::Description: return e.desc; case QAccessible::Value: return e.value; default: return {}; }
    }
    void setText(QAccessible::Text, const QString &) override {}
    QAccessible::Role role() const override { A11yEntry e; return get(e) ? e.role : QAccessible::NoRole; }
    QAccessible::State state() const override { A11yEntry e; if (!get(e)) { QAccessible::State s; s.invalid = true; return s; } QAccessible::State s = e.st; s.focusable = e.st.focusable; return s; }
    void *interface_cast(QAccessible::InterfaceType t) override {
        A11yEntry e; if (!get(e)) return nullptr;
        if (t == QAccessible::ActionInterface && e.canAct) return static_cast<QAccessibleActionInterface *>(this);
        if (t == QAccessible::ValueInterface && e.slider) return static_cast<QAccessibleValueInterface *>(this);
        return nullptr;
    }
    // actions
    QStringList actionNames() const override {
        A11yEntry e; if (!get(e)) return {};
        QStringList l; if (e.slider) { l << QAccessibleActionInterface::increaseAction() << QAccessibleActionInterface::decreaseAction(); }
        else if (e.canAct) l << (e.st.checkable && e.role == QAccessible::CheckBox ? QAccessibleActionInterface::toggleAction() : QAccessibleActionInterface::pressAction());
        if (e.st.focusable) l << QAccessibleActionInterface::setFocusAction();
        return l;
    }
    void doAction(const QString &name) override;
    QStringList keyBindingsForAction(const QString &) const override { return {}; }
    // slider value
    QVariant currentValue() const override { A11yEntry e; return get(e) ? e.val : 0; }
    void setCurrentValue(const QVariant &v) override;
    QVariant maximumValue() const override { A11yEntry e; return get(e) ? e.vmax : 0; }
    QVariant minimumValue() const override { A11yEntry e; return get(e) ? e.vmin : 0; }
    QVariant minimumStepSize() const override { A11yEntry e; return get(e) ? e.step : 1; }

    HostAccessible *parentIf; QPointer<QWidget> widget; int index;
private:
    bool get(A11yEntry &out) const {
        auto *h = dynamic_cast<A11yHost *>(widget.data()); if (!h) return false;
        auto v = h->a11yEntries(); if (index < 0 || index >= (int) v.size()) return false; out = v[index]; return true;
    }
};

// The widget itself: a pane named after its title whose children are the host's entries.
class HostAccessible : public QAccessibleWidget {
public:
    explicit HostAccessible(QWidget *w) : QAccessibleWidget(w, QAccessible::Pane) {}
    ~HostAccessible() override { for (QAccessible::Id id : std::as_const(ids)) QAccessible::deleteAccessibleInterface(id); }
    A11yHost *host() const { return dynamic_cast<A11yHost *>(widget()); }
    QString text(QAccessible::Text t) const override {
        A11yHost *h = host(); if (!h) return QAccessibleWidget::text(t);
        switch (t) { case QAccessible::Name: return h->a11yName(); case QAccessible::Description: return h->a11yDesc(); default: return QAccessibleWidget::text(t); }
    }
    QAccessible::State state() const override { QAccessible::State s = QAccessibleWidget::state(); if (A11yHost *h = host(); h && !h->a11yEntries().empty()) s.focusable = widget()->focusPolicy() != Qt::NoFocus; return s; }
    int childCount() const override { A11yHost *h = host(); const int n = h ? (int) h->a11yEntries().size() : 0; return n ? n : QAccessibleWidget::childCount(); }
    QAccessibleInterface *child(int i) const override {
        A11yHost *h = host(); if (!h || h->a11yEntries().empty()) return QAccessibleWidget::child(i);
        if (i < 0 || i >= (int) h->a11yEntries().size()) return nullptr;
        auto it = ids.find(i);
        if (it == ids.end()) { auto *c = new ChildAccessible(const_cast<HostAccessible *>(this), widget(), i); it = ids.insert(i, QAccessible::registerAccessibleInterface(c)); }
        return QAccessible::accessibleInterface(it.value());
    }
    int indexOfChild(const QAccessibleInterface *c) const override {
        if (auto *cc = dynamic_cast<const ChildAccessible *>(c)) return cc->index;
        return QAccessibleWidget::indexOfChild(c);
    }
    QAccessibleInterface *childAt(int x, int y) const override {
        A11yHost *h = host(); if (!h || h->a11yEntries().empty()) return QAccessibleWidget::childAt(x, y);
        auto v = h->a11yEntries(); const QPoint pt = widget()->mapFromGlobal(QPoint(x, y));
        for (int i = (int) v.size() - 1; i >= 0; --i) if (v[i].rect.contains(pt)) return child(i);
        return nullptr;
    }
    QAccessibleInterface *focusChild() const override {
        A11yHost *h = host(); if (!h || !widget()->hasFocus()) return QAccessibleWidget::focusChild();
        int i = h->a11yFocusEntry(); return i >= 0 ? child(i) : const_cast<HostAccessible *>(this);
    }
private:
    mutable QHash<int, QAccessible::Id> ids;
};

QAccessibleInterface *ChildAccessible::parent() const { return parentIf; }
void ChildAccessible::doAction(const QString &name) {
    auto *h = dynamic_cast<A11yHost *>(widget.data()); if (!h) return;
    if (name == QAccessibleActionInterface::setFocusAction()) { widget->setFocus(Qt::OtherFocusReason); return; }
    A11yEntry e; if (!get(e)) return;
    if (e.slider) { if (name == QAccessibleActionInterface::increaseAction()) h->a11ySet(index, e.val + e.step); else if (name == QAccessibleActionInterface::decreaseAction()) h->a11ySet(index, e.val - e.step); return; }
    h->a11yAct(index);
}
void ChildAccessible::setCurrentValue(const QVariant &v) { if (auto *h = dynamic_cast<A11yHost *>(widget.data())) h->a11ySet(index, v.toInt()); }

QAccessibleInterface *factory(const QString &, QObject *o) {
    auto *w = qobject_cast<QWidget *>(o);
    return w && dynamic_cast<A11yHost *>(w) ? new HostAccessible(w) : nullptr;
}

} // namespace

void installAccessibility() { QAccessible::installFactory(factory); }
