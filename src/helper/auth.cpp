#include "auth.h"
#include <QtDBus/QDBusArgument>
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusConnectionInterface>
#include <QtDBus/QDBusMessage>
#include <QtDBus/QDBusVariant>
#include <QElapsedTimer>
#include <QHash>
#include <QMutex>
#include <cstdio>

namespace {
const char kAction[] = "io.github.dmitry_s93.mcontrolcenter.helper-write";

struct Subject { QString kind; QVariantMap details; };
QDBusArgument &operator<<(QDBusArgument &a, const Subject &s) { a.beginStructure(); a << s.kind << s.details; a.endStructure(); return a; }
}  // namespace

bool mccauth::authorized(const QObject *adaptor) {
    const auto *ctxp = adaptor ? dynamic_cast<const QDBusContext *>(adaptor) : nullptr;      // the exported object itself carries the context
    if (!ctxp) { fprintf(stderr, "refused: no caller context object\n"); return false; }
    const QDBusContext &ctx = *ctxp;
    if (!ctx.calledFromDBus()) { fprintf(stderr, "refused: no D-Bus caller context\n"); return false; }   // fail closed
    const QString who = ctx.message().service();                             // unique bus name of the caller
    static QMutex mu; static QHash<QString, qint64> okUntil; static QElapsedTimer clock;
    if (!clock.isValid()) clock.start();
    const qint64 now = clock.elapsed();
    {   QMutexLocker l(&mu);                                                 // a fan-curve apply is ~30 calls: ask polkit once per caller per 5 s
        const auto it = okUntil.constFind(who);
        if (it != okUntil.constEnd() && *it > now) return true;
        if (okUntil.size() > 64) okUntil.clear(); }
    QDBusMessage m = QDBusMessage::createMethodCall("org.freedesktop.PolicyKit1", "/org/freedesktop/PolicyKit1/Authority",
                                                    "org.freedesktop.PolicyKit1.Authority", "CheckAuthorization");
    Subject subj{"system-bus-name", {{"name", who}}};
    QDBusArgument sa; sa.beginStructure(); sa << subj.kind;
    sa.beginMap(QMetaType::fromType<QString>(), QMetaType::fromType<QDBusVariant>());
    for (auto i = subj.details.constBegin(); i != subj.details.constEnd(); ++i) { sa.beginMapEntry(); sa << i.key() << QDBusVariant(i.value()); sa.endMapEntry(); }
    sa.endMap(); sa.endStructure();
    QDBusArgument det; det.beginMap(QMetaType::fromType<QString>(), QMetaType::fromType<QString>()); det.endMap();
    m << QVariant::fromValue(sa) << QString(kAction) << QVariant::fromValue(det) << uint(0) << QString();   // flags 0 = never prompt
    const QDBusMessage r = QDBusConnection::systemBus().call(m, QDBus::Block, 3000);
    bool ok = false;
    if (r.type() == QDBusMessage::ReplyMessage && !r.arguments().isEmpty()) {
        const QDBusArgument ra = r.arguments().first().value<QDBusArgument>();
        ra.beginStructure(); bool isAuth = false, isChall = false; ra >> isAuth >> isChall; ra.endStructure(); ok = isAuth;
    } else fprintf(stderr, "polkit check failed: %s\n", qPrintable(r.errorMessage()));
    if (ok) { QMutexLocker l(&mu); okUntil.insert(who, now + 5000); }
    else fprintf(stderr, "refused call from %s (not authorised for %s)\n", qPrintable(who), kAction);
    return ok;
}
