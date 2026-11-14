# Power & Battery popup with MSI controls (`local.battery-msi`)

A fork of KDE's Power & Battery applet (powerdevil v6.7.4, LGPL-2.0-or-later / GPL-2.0+, see SPDX headers in the QML) with an extra
`MsiSection.qml` that shows CPU/GPU temperatures, fan speeds, performance mode, Cooler Boost and charge limit inside the stock popup.
It talks to the running MControlCenter tray app over its session-bus API.

Install: `cp -r local.battery-msi ~/.local/share/plasma/plasmoids/` and use it in the system tray instead of the stock applet
(Plasma ignores same-id overrides of the compiled-in applet, hence the new id). Only tested on Plasma 6.7.4 / Kali.
