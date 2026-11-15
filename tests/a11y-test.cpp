// Keyboard + accessibility test for the custom-painted widgets (RowsPanel, TileStrip) and the accessible tree built from them.
// Build: cmake --build build --target a11y_test && QT_QPA_PLATFORM=offscreen build/a11y_test
#include <QApplication>
#include <QTest>
#include <QVBoxLayout>
#include <QPushButton>
#include <QAccessible>
#include <QAccessibleActionInterface>
#include <QAccessibleValueInterface>
#include "../src/a11y.h"
#include "../src/uikit.h"
using namespace kit;

class A11yTest : public QObject {
    Q_OBJECT
private slots:
    void initTestCase() { installAccessibility(); QAccessible::setActive(true); }
    void rowsPanelKeyboard() {
        RowsPanel p("", "TEST", nullptr); int toggled = 0, pressed = 0, picked = -1, slid = -1;
        Row sw; sw.type = Row::Switch; sw.label = "Switch one"; sw.click = [&] { ++toggled; }; p.rows.push_back(sw);
        Row seg; seg.type = Row::Segment; seg.label = "Choice"; seg.opts = {"A", "B", "C"}; seg.sel = 0; seg.pick = [&](int i) { picked = i; }; p.rows.push_back(seg);
        Row sl; sl.type = Row::Slider; sl.label = "Level"; sl.vmax = 100; sl.val = 50; sl.pick = [&](int v) { slid = v; }; p.rows.push_back(sl);
        Row bt; bt.type = Row::Button; bt.label = "Go"; bt.click = [&] { ++pressed; }; p.rows.push_back(bt);
        Row info; info.type = Row::Info; info.label = "Status"; info.value = "OK"; p.rows.push_back(info);
        p.resize(500, 500); p.show(); QVERIFY(QTest::qWaitForWindowExposed(&p)); p.setFocus(); QTRY_VERIFY(p.hasFocus());
        QTest::keyClick(&p, Qt::Key_Space); QCOMPARE(toggled, 1);                                    // first stop is the switch
        QTest::keyClick(&p, Qt::Key_Down); QTest::keyClick(&p, Qt::Key_Right); QTest::keyClick(&p, Qt::Key_Right); QCOMPARE(picked, -1);   // moving the cursor must not change the choice
        QTest::keyClick(&p, Qt::Key_Return); QCOMPARE(picked, 2);                                   // Enter selects option C
        QTest::keyClick(&p, Qt::Key_Down); QTest::keyClick(&p, Qt::Key_Right); QTest::keyClick(&p, Qt::Key_Right, Qt::ShiftModifier); QCOMPARE(p.rows[2].val, 61);
        QCOMPARE(slid, -1); QTRY_COMPARE_WITH_TIMEOUT(slid, 61, 2000);                                  // committed once, after the key stopped
        QTest::keyClick(&p, Qt::Key_Down); QTest::keyClick(&p, Qt::Key_Return); QCOMPARE(pressed, 1);
        QTest::keyClick(&p, Qt::Key_Down); QCOMPARE(pressed, 1);                                       // the info row is not a stop: Down stays on the button
        QTest::keyClick(&p, Qt::Key_Return); QCOMPARE(pressed, 2);
    }
    void rowsPanelAccessibleTree() {
        RowsPanel p("", "TEST PANEL", nullptr); bool on = false;
        Row sw; sw.type = Row::Switch; sw.label = "Switch one"; sw.on = false; sw.click = [&] { on = !on; p.rows[0].on = on; }; p.rows.push_back(sw);
        Row seg; seg.type = Row::Segment; seg.label = "Choice"; seg.opts = {"A", "B"}; seg.sel = 1; seg.pick = [](int) {}; p.rows.push_back(seg);
        Row sl; sl.type = Row::Slider; sl.label = "Level"; sl.vmax = 40; sl.val = 10; sl.pick = [](int) {}; p.rows.push_back(sl);
        Row info; info.type = Row::Info; info.label = "Status"; info.value = "Idle"; p.rows.push_back(info);
        p.resize(500, 400); p.show(); QVERIFY(QTest::qWaitForWindowExposed(&p));
        QAccessibleInterface *a = QAccessible::queryAccessibleInterface(&p); QVERIFY(a);
        QCOMPARE(a->text(QAccessible::Name), QString("TEST PANEL"));
        QCOMPARE(a->childCount(), 5);                                                                 // switch + 2 options + slider + info
        QAccessibleInterface *c0 = a->child(0); QCOMPARE(c0->role(), QAccessible::CheckBox); QCOMPARE(c0->text(QAccessible::Name), QString("Switch one")); QVERIFY(!c0->state().checked);
        auto *act = c0->actionInterface(); QVERIFY(act); QVERIFY(act->actionNames().contains(QAccessibleActionInterface::toggleAction()));
        act->doAction(QAccessibleActionInterface::toggleAction()); QVERIFY(on); QVERIFY(c0->state().checked);
        QAccessibleInterface *c2 = a->child(2); QCOMPARE(c2->role(), QAccessible::RadioButton); QVERIFY(c2->state().checked); QCOMPARE(c2->text(QAccessible::Name), QString("Choice: B"));
        QAccessibleInterface *c3 = a->child(3); QCOMPARE(c3->role(), QAccessible::Slider); auto *v = c3->valueInterface(); QVERIFY(v); QCOMPARE(v->currentValue().toInt(), 10); QCOMPARE(v->maximumValue().toInt(), 40);
        v->setCurrentValue(25); QCOMPARE(p.rows[2].val, 25);
        QAccessibleInterface *c4 = a->child(4); QCOMPARE(c4->text(QAccessible::Name), QString("Status: Idle"));
        QCOMPARE(a->indexOfChild(c3), 3); QCOMPARE(c3->parent(), a);
        QVERIFY(c3->rect().isValid());
    }
    void tileStrip() {
        TileStrip t(true, nullptr); int clicked = -1; t.controls = true; t.onClick = [&](int i) { clicked = i; t.checked = i; };
        t.setItems({{"Turbo", "max", Qt::red}, {"Balanced", "mid", Qt::white}, {"Silent", "low", Qt::blue}}); t.checked = 0; t.reveal = 1;
        t.resize(600, 60); t.show(); QVERIFY(QTest::qWaitForWindowExposed(&t)); t.setFocus(); QTRY_VERIFY(t.hasFocus());
        QTest::keyClick(&t, Qt::Key_Right); QCOMPARE(clicked, -1); QTest::keyClick(&t, Qt::Key_Right); QTest::keyClick(&t, Qt::Key_Space); QCOMPARE(clicked, 2);
        QAccessibleInterface *a = QAccessible::queryAccessibleInterface(&t); QVERIFY(a); QCOMPARE(a->childCount(), 3);
        QCOMPARE(a->child(2)->role(), QAccessible::RadioButton); QVERIFY(a->child(2)->state().checked); QVERIFY(!a->child(0)->state().checked);
        QCOMPARE(a->child(0)->text(QAccessible::Name), QString("Turbo"));
    }
    void noStopsDoesNotTrapFocus() {
        QWidget w; auto *l = new QVBoxLayout(&w); auto *info = new RowsPanel("", "INFO ONLY", &w); Row r; r.type = Row::Info; r.label = "x"; r.value = "y"; info->rows.push_back(r);
        auto *after = new QPushButton("after", &w); l->addWidget(info); l->addWidget(after); w.show(); QVERIFY(QTest::qWaitForWindowExposed(&w));
        info->setFocus(Qt::TabFocusReason); QTRY_VERIFY(after->hasFocus());                           // a panel with nothing to operate hands focus on
    }
};
QTEST_MAIN(A11yTest)
#include "a11y-test.moc"
