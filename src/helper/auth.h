// Caller authorisation for the root helper. Every method that changes hardware state asks polkit first:
// the action is allowed without a prompt for a local, active session, and needs an administrator otherwise
// (ssh, inactive sessions, other seats, services). Fails closed: no polkit / no answer = refused.
#pragma once
#include <QtCore/QObject>
#include <QtDBus/QDBusContext>

namespace mccauth {
// Qt sets the caller context on the *registered object*, not on its adaptor, so the registered objects carry it.
struct CtxObject : QObject, QDBusContext {};
// Pass the adaptor (`this`); must be called from inside a D-Bus slot. False if there is no caller context.
bool authorized(const QObject *adaptor);
}
