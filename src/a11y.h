// Accessibility for the custom-painted widgets: exposes each kit::A11yHost (panels, tile strips, row lists) and its virtual children to screen readers.
#ifndef A11Y_H
#define A11Y_H
void installAccessibility();   // call once after QApplication is created
#endif
