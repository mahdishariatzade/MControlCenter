// Neon-dark theme for MControlCenter (matches the Kali / Dr460nized desktop). Applied as a stylesheet on the main window only.
#ifndef THEME_H
#define THEME_H
static const char *kThemeQss = R"QSS(
QMainWindow, QTabWidget, QTabBar, QWidget#centralwidget, QWidget#dashboard, QWidget#overviewTab, QWidget#batteryTab, QWidget#fanControlTab, QWidget#keyboardTab, QWidget#aboutTab, QWidget#debugTab, QTabWidget QStackedWidget > QWidget { background: #0a0e17; }
QWidget { color: #d7e3ff; font-size: 13px; }
QTabWidget::pane { border: none; background: transparent; }
QTabBar { background: transparent; }
QTabBar::tab { background: transparent; color: #7f92b8; padding: 9px 18px; margin: 4px 3px 6px 3px; border-radius: 10px; border: 1px solid transparent; font-weight: 600; letter-spacing: 1px; }
QTabBar::tab:hover { color: #d7e3ff; background: #121a2b; }
QTabBar::tab:selected { color: #19e3ff; background: #121a2b; border: 1px solid #19e3ff; }
QLabel#dashTitle { font-size: 26px; font-weight: 800; letter-spacing: 3px; color: #ffffff; }
QLabel#dashSub { color: #7f92b8; font-size: 12px; letter-spacing: 1px; }
QLabel#section { color: #19e3ff; font-size: 11px; font-weight: 700; letter-spacing: 2px; padding-top: 2px; }
QLabel#chip { background: #121a2b; border: 1px solid #233252; border-radius: 12px; padding: 5px 14px; font-weight: 700; font-size: 11px; letter-spacing: 1px; }
QLabel#chip[tone="ok"] { color: #3dff9a; border-color: #1d6b49; }
QLabel#chip[tone="warn"] { color: #ffb020; border-color: #7a5410; }
QFrame#card { background: qlineargradient(x1:0,y1:0,x2:1,y2:1, stop:0 #162036, stop:1 #0d1321); border: 1px solid #233252; border-radius: 14px; }
QLabel#statKey { color: #7f92b8; background: transparent; }
QLabel#statVal { color: #ffffff; font-weight: 700; background: transparent; }
QGroupBox { border: 1px solid #233252; border-radius: 12px; margin-top: 14px; padding: 14px 10px 10px 10px; background: #0e1424; font-weight: 700; }
QGroupBox::title { subcontrol-origin: margin; left: 14px; padding: 0 8px; color: #19e3ff; letter-spacing: 1px; }
QPushButton { background: #121a2b; border: 1px solid #233252; border-radius: 9px; padding: 7px 16px; font-weight: 600; }
QPushButton:hover { border-color: #19e3ff; color: #19e3ff; }
QPushButton:pressed { background: #19e3ff; color: #0a0e17; }
QPushButton:disabled { color: #4a5a7a; border-color: #1a2440; }
QCheckBox, QRadioButton { spacing: 9px; background: transparent; }
QCheckBox::indicator { width: 18px; height: 18px; border-radius: 5px; border: 1px solid #34466f; background: #0e1424; }
QCheckBox::indicator:hover, QRadioButton::indicator:hover { border-color: #19e3ff; }
QCheckBox::indicator:checked { background: #19e3ff; border-color: #19e3ff; }
QRadioButton::indicator { width: 18px; height: 18px; border-radius: 10px; border: 1px solid #34466f; background: #0e1424; }
QRadioButton::indicator:checked { background: qradialgradient(cx:.5,cy:.5,radius:.5,fx:.5,fy:.5,stop:0 #19e3ff,stop:.55 #19e3ff,stop:.6 #0e1424,stop:1 #0e1424); border-color: #19e3ff; }
QCheckBox:disabled, QRadioButton:disabled { color: #4a5a7a; }
QSlider::groove:horizontal { height: 6px; background: #1c2740; border-radius: 3px; }
QSlider::sub-page:horizontal { background: qlineargradient(x1:0,y1:0,x2:1,y2:0, stop:0 #19e3ff, stop:1 #ff2bd6); border-radius: 3px; }
QSlider::handle:horizontal { width: 16px; height: 16px; margin: -6px 0; border-radius: 8px; background: #ffffff; border: 2px solid #19e3ff; }
QSlider::groove:vertical { width: 6px; background: #1c2740; border-radius: 3px; }
QSlider::add-page:vertical { background: qlineargradient(x1:0,y1:1,x2:0,y2:0, stop:0 #19e3ff, stop:1 #ff2bd6); border-radius: 3px; }
QSlider::sub-page:vertical { background: #1c2740; border-radius: 3px; }
QSlider::handle:vertical { height: 16px; width: 16px; margin: 0 -6px; border-radius: 8px; background: #ffffff; border: 2px solid #19e3ff; }
QSpinBox { min-width: 74px; }
QTabBar::tab:focus { outline: none; }
QSpinBox, QComboBox, QLineEdit, QDoubleSpinBox { background: #0e1424; border: 1px solid #233252; border-radius: 8px; padding: 5px 9px; selection-background-color: #19e3ff; selection-color: #0a0e17; }
QSpinBox:focus, QComboBox:focus, QLineEdit:focus { border-color: #19e3ff; }
QComboBox::drop-down { border: none; width: 22px; }
QComboBox QAbstractItemView { background: #0e1424; border: 1px solid #19e3ff; selection-background-color: #19e3ff; selection-color: #0a0e17; outline: none; }
QScrollBar:vertical { background: transparent; width: 10px; } QScrollBar::handle:vertical { background: #233252; border-radius: 5px; min-height: 30px; }
QScrollBar::add-line, QScrollBar::sub-line { height: 0; width: 0; } QScrollBar:horizontal { background: transparent; height: 10px; } QScrollBar::handle:horizontal { background: #233252; border-radius: 5px; }
QToolTip { background: #121a2b; color: #d7e3ff; border: 1px solid #19e3ff; padding: 5px; }
QMenu { background: #0e1424; border: 1px solid #233252; padding: 6px; } QMenu::item { padding: 7px 22px; border-radius: 6px; } QMenu::item:selected { background: #19e3ff; color: #0a0e17; }
)QSS";
#endif
