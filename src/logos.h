// Neon marks for the UI: vendor badges (MSI / Intel / NVIDIA / AMD) and small line icons for every subsystem.
// Included from uikit.h INSIDE namespace kit (it uses the palette helpers defined there). Everything is vector drawing in the app's own accent
// colours, so it follows the power mode. The vendor badges are original stylised marks (emblem + wordmark), not the companies' official logos.
// All product names are trademarks of their owners; this project is not affiliated with or endorsed by them.

enum Icon { IcNone, IcCpu, IcGpu, IcRam, IcFan, IcBattery, IcThermal, IcPulse, IcMonitor, IcKeyboard, IcBolt, IcGear, IcInfo, IcSliders, IcBulb, IcLink, IcCalib, IcRefresh };
enum Brand { BrNone, BrMsi, BrIntel, BrNvidia, BrAmd };

inline Brand brandOf(const QString &s) {
    const QString l = s.toLower();
    if (l.contains("nvidia") || l.contains("geforce") || l.contains("rtx") || l.contains("quadro")) return BrNvidia;
    if (l.contains("amd") || l.contains("ryzen") || l.contains("radeon") || l.contains("advanced micro")) return BrAmd;
    if (l.contains("intel") || l.contains("core(tm)") || l.contains("core ultra") || l.contains("arc ")) return BrIntel;
    if (l.contains("msi") || l.contains("micro-star")) return BrMsi;
    return BrNone;
}

// three passes = soft glow without any blur filter (cheap, resolution independent)
inline void neon(QPainter &p, const QPainterPath &path, const QColor &c, double w, bool glow = true) {
    p.setBrush(Qt::NoBrush);
    if (glow) { p.setPen(QPen(alpha(c, 0.10), w * 4.2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin)); p.drawPath(path);
                p.setPen(QPen(alpha(c, 0.22), w * 2.2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin)); p.drawPath(path); }
    p.setPen(QPen(c, w, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin)); p.drawPath(path);
}

inline void paintIcon(QPainter &p, const QRectF &box, int id, const QColor &c) {
    if (id == IcNone) return;
    const double s = qMin(box.width(), box.height()); const QPointF o(box.center().x() - s / 2, box.center().y() - s / 2);
    auto P = [&](double x, double y) { return QPointF(o.x() + x * s, o.y() + y * s); };
    auto R = [&](double x, double y, double w, double h) { return QRectF(P(x, y), QSizeF(w * s, h * s)); };
    const double w = qMax(1.2, s / 14); QPainterPath a;
    p.save(); p.setRenderHint(QPainter::Antialiasing);
    switch (id) {
        case IcCpu:
            a.addRoundedRect(R(.24, .24, .52, .52), s * .06, s * .06); a.addRect(R(.38, .38, .24, .24));
            for (double t : {.36, .5, .64}) { a.moveTo(P(t, .24)); a.lineTo(P(t, .08)); a.moveTo(P(t, .76)); a.lineTo(P(t, .92)); a.moveTo(P(.24, t)); a.lineTo(P(.08, t)); a.moveTo(P(.76, t)); a.lineTo(P(.92, t)); } break;
        case IcGpu:
            a.addRoundedRect(R(.06, .24, .88, .50), s * .05, s * .05); a.addEllipse(P(.36, .49), s * .16, s * .16); a.addEllipse(P(.36, .49), s * .05, s * .05);
            a.moveTo(P(.62, .36)); a.lineTo(P(.84, .36)); a.moveTo(P(.62, .50)); a.lineTo(P(.84, .50)); a.moveTo(P(.62, .62)); a.lineTo(P(.76, .62)); a.moveTo(P(.12, .74)); a.lineTo(P(.12, .86)); a.lineTo(P(.5, .86)); a.lineTo(P(.5, .74)); break;
        case IcRam:
            a.addRoundedRect(R(.06, .28, .88, .40), s * .04, s * .04);
            for (double x : {.16, .36, .56, .76}) a.addRect(R(x, .38, .12, .2));
            for (double x : {.14, .26, .38, .50, .62, .74, .86}) { a.moveTo(P(x, .68)); a.lineTo(P(x, .8)); } break;
        case IcFan: {
            a.addEllipse(P(.5, .5), s * .42, s * .42); a.addEllipse(P(.5, .5), s * .07, s * .07);
            for (int k = 0; k < 3; ++k) { const double ang = k * 2 * M_PI / 3 - M_PI / 2, ca = cos(ang), sa = sin(ang);
                QPointF c0 = P(.5 + .07 * ca, .5 + .07 * sa), tip = P(.5 + .36 * ca - .16 * sa, .5 + .36 * sa + .16 * ca), tip2 = P(.5 + .30 * ca + .13 * sa, .5 + .30 * sa - .13 * ca);
                a.moveTo(c0); a.quadTo(tip, tip2); a.lineTo(c0); }
            break; }
        case IcBattery:
            a.addRoundedRect(R(.06, .30, .78, .40), s * .06, s * .06); a.addRect(R(.84, .42, .08, .16));
            a.moveTo(P(.50, .36)); a.lineTo(P(.38, .52)); a.lineTo(P(.50, .52)); a.lineTo(P(.40, .66)); break;
        case IcThermal:
            a.addRoundedRect(R(.40, .08, .20, .56), s * .10, s * .10); a.addEllipse(P(.5, .76), s * .18, s * .18); a.moveTo(P(.5, .30)); a.lineTo(P(.5, .72));
            for (double y : {.18, .28, .38}) { a.moveTo(P(.66, y)); a.lineTo(P(.78, y)); } break;
        case IcPulse:
            a.moveTo(P(.04, .52)); a.lineTo(P(.28, .52)); a.lineTo(P(.38, .22)); a.lineTo(P(.52, .80)); a.lineTo(P(.62, .40)); a.lineTo(P(.68, .52)); a.lineTo(P(.96, .52)); break;
        case IcMonitor:
            a.addRoundedRect(R(.06, .14, .88, .56), s * .05, s * .05); a.moveTo(P(.5, .70)); a.lineTo(P(.5, .84)); a.moveTo(P(.28, .86)); a.lineTo(P(.72, .86)); break;
        case IcKeyboard:
            a.addRoundedRect(R(.04, .24, .92, .52), s * .06, s * .06);
            for (int r = 0; r < 3; ++r) for (int k = 0; k < (r == 2 ? 1 : 5); ++k) { if (r == 2) a.addRect(R(.24, .60, .52, .06)); else a.addRect(R(.12 + k * .16, .34 + r * .12, .08, .05)); } break;
        case IcBolt:
            a.moveTo(P(.58, .06)); a.lineTo(P(.22, .56)); a.lineTo(P(.48, .56)); a.lineTo(P(.38, .94)); a.lineTo(P(.78, .38)); a.lineTo(P(.52, .38)); a.closeSubpath(); break;
        case IcGear:
            a.addEllipse(P(.5, .5), s * .20, s * .20); a.addEllipse(P(.5, .5), s * .32, s * .32);
            for (int k = 0; k < 8; ++k) { const double ang = k * M_PI / 4, ca = cos(ang), sa = sin(ang); a.moveTo(P(.5 + .32 * ca, .5 + .32 * sa)); a.lineTo(P(.5 + .44 * ca, .5 + .44 * sa)); } break;
        case IcInfo:
            a.addEllipse(P(.5, .5), s * .42, s * .42); a.moveTo(P(.5, .46)); a.lineTo(P(.5, .72)); a.moveTo(P(.5, .30)); a.lineTo(P(.5, .31)); break;
        case IcSliders:
            for (double y : {.24, .5, .76}) { a.moveTo(P(.08, y)); a.lineTo(P(.92, y)); }
            a.addEllipse(P(.34, .24), s * .08, s * .08); a.addEllipse(P(.68, .5), s * .08, s * .08); a.addEllipse(P(.44, .76), s * .08, s * .08); break;
        case IcBulb:
            a.addEllipse(P(.5, .38), s * .26, s * .26); a.moveTo(P(.38, .66)); a.lineTo(P(.38, .78)); a.lineTo(P(.62, .78)); a.lineTo(P(.62, .66)); a.moveTo(P(.42, .88)); a.lineTo(P(.58, .88)); break;
        case IcLink:
            a.addRoundedRect(R(.06, .36, .46, .28), s * .14, s * .14); a.addRoundedRect(R(.48, .36, .46, .28), s * .14, s * .14); break;
        case IcCalib:
            a.addEllipse(P(.5, .5), s * .40, s * .40); a.moveTo(P(.5, .50)); a.lineTo(P(.5, .22)); a.moveTo(P(.5, .50)); a.lineTo(P(.70, .62));
            for (int k = 0; k < 12; k += 3) { const double ang = k * M_PI / 6, ca = cos(ang), sa = sin(ang); a.moveTo(P(.5 + .34 * ca, .5 + .34 * sa)); a.lineTo(P(.5 + .40 * ca, .5 + .40 * sa)); } break;
        case IcRefresh:
            a.arcMoveTo(R(.14, .14, .72, .72), 40); a.arcTo(R(.14, .14, .72, .72), 40, 250); a.moveTo(P(.78, .10)); a.lineTo(P(.80, .34)); a.lineTo(P(.58, .30)); break;
    }
    neon(p, a, c, w); p.restore();
}

// ── vendor badge: chamfered frame + emblem + wordmark, drawn in the given colour ──
inline QString brandText(int b) { return b == BrMsi ? "MSI" : b == BrIntel ? "intel" : b == BrNvidia ? "NVIDIA" : b == BrAmd ? "AMD" : QString(); }
inline double brandWidth(int b, double h) {
    if (b == BrNone) return 0; QFont f = fnt(RAJ, int(h * 0.50), QFont::Bold, h * 0.07);
    return h * 0.62 + QFontMetricsF(f).horizontalAdvance(brandText(b)) + h * 0.55;
}
inline void paintBrand(QPainter &p, const QRectF &r, int b, const QColor &c) {
    if (b == BrNone) return;
    p.save(); p.setRenderHint(QPainter::Antialiasing);
    const double h = r.height(); p.setBrush(alpha(c, 0.07)); p.setPen(QPen(alpha(c, 0.55), 1)); p.drawPath(chamfer(r.adjusted(.5, .5, -.5, -.5), h * 0.28));
    const QRectF em(r.left() + h * 0.18, r.top() + h * 0.18, h * 0.64, h * 0.64); QPainterPath a; const QPointF m = em.center(); const double e = em.width();
    auto Q = [&](double x, double y) { return QPointF(em.left() + x * e, em.top() + y * e); };
    switch (b) {
        case BrMsi:    // two blades forming an angular crest
            a.moveTo(Q(.04, .90)); a.lineTo(Q(.04, .14)); a.lineTo(Q(.34, .52)); a.lineTo(Q(.50, .14)); a.lineTo(Q(.66, .52)); a.lineTo(Q(.96, .14)); a.lineTo(Q(.96, .90)); break;
        case BrIntel:  // ring with the dot of an "i"
            a.addEllipse(m, e * .46, e * .46); a.moveTo(Q(.50, .40)); a.lineTo(Q(.50, .76)); a.moveTo(Q(.50, .24)); a.lineTo(Q(.50, .25)); break;
        case BrNvidia: // an eye: lens outline + pupil
            a.moveTo(Q(.02, .5)); a.quadTo(Q(.5, -.22), Q(.98, .5)); a.quadTo(Q(.5, 1.22), Q(.02, .5)); a.addEllipse(m, e * .17, e * .17); break;
        case BrAmd:    // the corner arrow: an open triangle with a notch
            a.moveTo(Q(.08, .92)); a.lineTo(Q(.08, .08)); a.lineTo(Q(.92, .08)); a.moveTo(Q(.08, .92)); a.lineTo(Q(.92, .08)); a.moveTo(Q(.52, .92)); a.lineTo(Q(.92, .92)); a.lineTo(Q(.92, .50)); break;
    }
    neon(p, a, c, qMax(1.3, h / 15));
    p.setFont(fnt(RAJ, int(h * 0.50), QFont::Bold, h * 0.07)); p.setPen(c);
    p.drawText(QRectF(r.left() + h * 0.88, r.top(), r.width() - h * 0.9, h), Qt::AlignLeft | Qt::AlignVCenter, brandText(b));
    p.restore();
}

// every panel gets the icon that matches its title, so new panels need no extra code
inline int iconForTitle(const QString &t) {
    const QString u = t.toUpper();
    struct K { const char *k; int ic; };
    static const K map[] = {{"CALIBRATION", IcCalib}, {"THERMAL", IcThermal}, {"COOLING", IcFan}, {"FAN", IcFan}, {"POWER PROFILE", IcBolt}, {"POWER", IcBattery}, {"BATTERY", IcBattery}, {"CHARGE", IcBattery},
        {"TELEMETRY", IcPulse}, {"DATA", IcPulse}, {"GPU", IcGpu}, {"GRAPHICS", IcGpu}, {"MONITOR", IcMonitor}, {"DISPLAY", IcMonitor}, {"SCREEN", IcMonitor}, {"REFRESH", IcRefresh},
        {"KEYBOARD", IcKeyboard}, {"KEY GROUPS", IcKeyboard}, {"COLOUR", IcBulb}, {"PRESETS", IcBulb}, {"PROFILES", IcBulb}, {"EFFECT", IcBulb}, {"FOLLOW", IcLink}, {"PAINT", IcSliders},
        {"HARDWARE", IcSliders}, {"AUTOMATION", IcRefresh}, {"STARTUP", IcGear}, {"SYSTEM", IcCpu}, {"CPU", IcCpu}, {"MEMORY", IcRam}, {"CREDITS", IcInfo}, {"PROJECT", IcLink}};
    for (const K &k : map) if (u.contains(QLatin1String(k.k))) return k.ic;
    return IcNone;
}
