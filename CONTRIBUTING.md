# Contributing

This is an open-source fork of [dmitry-s93/MControlCenter](https://github.com/dmitry-s93/MControlCenter) aimed at **every MSI laptop that the
[msi-ec](https://github.com/BeardOverflow/msi-ec) driver supports** (gaming series first), with **KDE Plasma as the primary desktop** and GNOME / others supported.

## Ground rules
- **Never assume one machine.** No hard-coded device names, PCI addresses, `BAT1`/`ADP1`, GPU or CPU models. Discover things from sysfs (see `src/sensors.cpp`) and hide features the hardware does not report (see `Caps` / `probeCaps()`).
- **Never block the GUI thread.** EC-backed sysfs reads and D-Bus calls to the root helper can take 100+ ms. Read on the sensor thread (`SensorWorker`); the UI only draws. Run with `MCC_FRAMELOG=1` and make sure no `[frame] gap` lines appear.
- **Be cheap.** This runs on daily-driver laptops. Animation only runs while the window is visible; repaint only what changed. Check CPU with `top -H -p $(pgrep -x mcontrolcenter)` while the window is open.
- **Desktop-neutral core, desktop-specific extras.** The app itself is plain Qt6 + a tray icon + the session D-Bus API. KDE bits live in `plasma/`; GNOME / other integrations are welcome as separate directories (e.g. `gnome-shell/`) that talk to the same `/Control` D-Bus API.

## Adding or testing a device
1. Install, run `mcontrolcenter-doctor report`, open a **Hardware report** issue.
2. If msi-ec does not know your EC firmware, report it upstream to msi-ec first; this app picks up new firmware automatically once DKMS rebuilds a newer msi-ec.

## Development
```bash
cmake -S . -B build && cmake --build build -j
MCC_SHOT=/tmp/shot QT_QPA_PLATFORM=xcb xvfb-run -a build/mcontrolcenter   # renders every tab to PNG (applies the saved mode to the EC!)
```
Design language: graphite palette, one mode-following accent, chamfered panels, segmented meters, Rajdhani + IBM Plex Mono (bundled, OFL). Please keep new UI consistent with it.
