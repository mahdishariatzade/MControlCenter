// Caller authorisation for the root helper. Every method that changes hardware state asks polkit first:
// the action is allowed without a prompt for a local, active session; anything else (ssh, a session logind does not
// track, a bare window manager, another seat) needs an administrator. The app can ask for that explicitly through
// requestAuthorization(), which lets polkit show its normal password dialog. Fails closed: no polkit = refused.
#pragma once
#include <QtCore/QObject>

namespace mccauth {
// Pass the exported object (`this`); must be called from inside a D-Bus slot.
bool authorized(const QObject *exported, bool quiet = false);
// Interactive variant: asks polkit with user interaction allowed and answers the D-Bus call later (bool reply).
void requestInteractive(QObject *exported);
}
