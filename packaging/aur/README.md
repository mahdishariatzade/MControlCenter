# Arch / AUR packaging

Two PKGBUILDs, same files as the .deb/.rpm (they use the project's CMake `install()` rules):

- `mcontrolcenter/` builds the tagged release (bump `pkgver` and the sha256 with `updpkgsums` on each release).
- `mcontrolcenter-git/` builds the latest `main`.

Status: the PKGBUILDs pass `bash -n` and the build + `cmake --install` steps were run into a staging directory on Kali, but **`makepkg`/`namcap` have not been run on a real Arch system** and nothing is published on the AUR yet. Test with `cd packaging/aur/mcontrolcenter-git && makepkg -si`, and run `namcap PKGBUILD *.pkg.tar.zst`.

To publish: create an AUR account, add an SSH key, then `git clone ssh://aur@aur.archlinux.org/mcontrolcenter-git.git`, copy `PKGBUILD`, `mcontrolcenter.install` and a generated `.SRCINFO` (`makepkg --printsrcinfo > .SRCINFO`), commit and push.

The `msi-ec` kernel driver is an optional dependency (`msi-ec-dkms` on the AUR) because newer kernels may ship it; the packaged `mcontrolcenter-doctor` service checks and repairs it at boot.
