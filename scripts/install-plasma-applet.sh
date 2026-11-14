#!/bin/bash
# Installs the optional KDE Plasma 6 panel applet (the Power & Battery popup with the MSI controls built in) for the CURRENT user. No root needed.
#   ./scripts/install-plasma-applet.sh            install / update
#   ./scripts/install-plasma-applet.sh --remove   remove
# After installing: right-click the panel > Add Widgets > "Power and Battery (MSI)". Plasma 6 only.
set -eu
SRC="$(cd "$(dirname "$0")/.." && pwd)/plasma/local.battery-msi"
DST="${XDG_DATA_HOME:-$HOME/.local/share}/plasma/plasmoids/local.battery-msi"
[ "$(id -u)" = 0 ] && { echo "run this as your normal user, not root" >&2; exit 1; }
if [ "${1:-}" = "--remove" ]; then rm -rf "$DST"; echo "removed $DST"; exit 0; fi
command -v plasmashell >/dev/null 2>&1 || { echo "plasmashell not found: this applet is only for KDE Plasma 6" >&2; exit 1; }
[ -d "$SRC" ] || { echo "applet sources not found at $SRC" >&2; exit 1; }
rm -rf "$DST"; mkdir -p "$(dirname "$DST")"; cp -r "$SRC" "$DST"
echo "installed $DST"
echo "Restart the panel to load it:  kquitapp6 plasmashell; kstart plasmashell    (or log out and in)"
