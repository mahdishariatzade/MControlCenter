#!/bin/bash
# MControlCenter installer - builds the app, installs it, and sets up the msi-ec driver so it keeps working after kernel updates.
#   ./install.sh               install / upgrade
#   ./install.sh --no-driver   skip the msi-ec kernel driver step
#   ./install.sh --no-deps     do not install build dependencies (packagers / already installed)
#   ./install.sh --no-build    reuse an existing ./build (packagers)
#   ./install.sh --uninstall   remove the app, helper and doctor (leaves the msi-ec driver alone)
# Tested on Debian-family (Kali). The dnf / pacman / zypper paths are written but not yet tested on real machines.
set -euo pipefail
cd "$(dirname "$(readlink -f "$0")")"
SUDO=""; [ "$(id -u)" = 0 ] || SUDO="sudo"
DRIVER=1; UNINSTALL=0; DEPS=1; BUILD=1
for a in "$@"; do case "$a" in --no-driver) DRIVER=0 ;; --uninstall) UNINSTALL=1 ;; --no-deps) DEPS=0 ;; --no-build) BUILD=0 ;; -h|--help) sed -n 2,6p "$0"; exit 0 ;; esac; done
say() { printf '\033[1;38;5;208m==>\033[0m %s\n' "$*"; }
. /etc/os-release 2>/dev/null || true
FAMILY="${ID:-} ${ID_LIKE:-}"

if [ "$UNINSTALL" = 1 ]; then
    say "removing MControlCenter"
    $SUDO systemctl disable --now mcontrolcenter-doctor.service 2>/dev/null || true
    $SUDO rm -vf /usr/bin/mcontrolcenter /usr/libexec/mcontrolcenter-helper /usr/share/applications/mcontrolcenter.desktop /etc/xdg/autostart/mcontrolcenter.desktop \
        /usr/share/icons/hicolor/scalable/apps/mcontrolcenter.svg /usr/share/dbus-1/system.d/mcontrolcenter-helper.conf /usr/share/dbus-1/system-services/mcontrolcenter.helper.service \
        /etc/systemd/system/mcontrolcenter-doctor.service /usr/share/polkit-1/actions/io.github.dmitry_s93.mcontrolcenter.doctor.policy /usr/share/polkit-1/actions/io.github.dmitry_s93.mcontrolcenter.gpu.policy
    $SUDO rm -rvf /usr/libexec/mcontrolcenter
    say "done (the msi-ec driver was left in place; remove it with: sudo dkms remove msi-ec --all)"; exit 0
fi

[ "$DEPS" = 1 ] && { say "installing build dependencies"
case "$FAMILY" in
    *debian*|*ubuntu*|*kali*|*mint*|*pop*) $SUDO apt-get update -qq; $SUDO apt-get install -y cmake build-essential git qt6-base-dev qt6-tools-dev qt6-tools-dev-tools qt6-l10n-tools pkexec polkitd dbus ;;
    *fedora*|*rhel*|*centos*|*rocky*|*alma*) $SUDO dnf install -y cmake gcc-c++ make git qt6-qtbase-devel qt6-qttools-devel polkit ;;
    *arch*|*manjaro*|*endeavouros*|*garuda*|*cachyos*) $SUDO pacman -S --needed --noconfirm cmake gcc make git qt6-base qt6-tools polkit ;;
    *suse*) $SUDO zypper -n install cmake gcc-c++ make git qt6-base-devel qt6-tools-devel qt6-linguist-devel polkit ;;
    *) echo "Unknown distro '$FAMILY'. Install: cmake, a C++ compiler, git, Qt6 (base + tools dev packages), polkit - then re-run with --no-deps." >&2; exit 1 ;;
esac; }

[ "$BUILD" = 1 ] && { say "building"
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release >/dev/null
cmake --build build --parallel "$(nproc)"; }

say "installing"
$SUDO install -Dm755 build/mcontrolcenter /usr/bin/mcontrolcenter
$SUDO install -Dm755 build/helper/mcontrolcenter-helper /usr/libexec/mcontrolcenter-helper
$SUDO install -Dm644 resources/mcontrolcenter.desktop /usr/share/applications/mcontrolcenter.desktop
$SUDO install -Dm644 resources/mcontrolcenter.svg /usr/share/icons/hicolor/scalable/apps/mcontrolcenter.svg
$SUDO install -Dm644 src/helper/mcontrolcenter-helper.conf /usr/share/dbus-1/system.d/mcontrolcenter-helper.conf
$SUDO install -Dm644 src/helper/mcontrolcenter.helper.service /usr/share/dbus-1/system-services/mcontrolcenter.helper.service
$SUDO install -Dm755 scripts/mcontrolcenter-doctor /usr/libexec/mcontrolcenter/mcontrolcenter-doctor
$SUDO install -Dm755 scripts/mcontrolcenter-gpu /usr/libexec/mcontrolcenter/mcontrolcenter-gpu
$SUDO install -Dm644 packaging/io.github.dmitry_s93.mcontrolcenter.gpu.policy /usr/share/polkit-1/actions/io.github.dmitry_s93.mcontrolcenter.gpu.policy
$SUDO install -Dm644 packaging/io.github.dmitry_s93.mcontrolcenter.doctor.policy /usr/share/polkit-1/actions/io.github.dmitry_s93.mcontrolcenter.doctor.policy
$SUDO install -Dm644 packaging/mcontrolcenter-doctor.service /etc/systemd/system/mcontrolcenter-doctor.service
$SUDO install -Dm644 packaging/mcontrolcenter-autostart.desktop /etc/xdg/autostart/mcontrolcenter.desktop
$SUDO systemctl daemon-reload
$SUDO systemctl reload dbus 2>/dev/null || true
$SUDO systemctl enable mcontrolcenter-doctor.service >/dev/null 2>&1 || true   # boot-time self-heal after kernel updates

if [ "$DRIVER" = 1 ]; then
    say "checking the msi-ec driver"
    state="$(/usr/libexec/mcontrolcenter/mcontrolcenter-doctor status | sed -n 's/^state=//p')"
    case "$state" in
        ok) say "msi-ec is already loaded" ;;
        not-msi) say "this does not look like an MSI laptop - the app will still run in read-only fallback mode" ;;
        *) say "msi-ec needs setup (state: $state) - building it for kernel $(uname -r)"; $SUDO /usr/libexec/mcontrolcenter/mcontrolcenter-doctor fix || say "driver setup did not finish - open the app, the banner on the Dashboard will explain what is missing" ;;
    esac
fi
say "installed. Start it from the application menu (it lives in the system tray) or run: mcontrolcenter"
