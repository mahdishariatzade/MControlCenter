# Changelog

All notable changes to this project will be documented in this file.

## Unreleased

## [0.6.1] 2026-11-15 (fork, p34t1k/MControlCenter)
- **Security: root helper lockdown.** Before this, any local user could call the root helper over D-Bus and write any byte to any EC offset (inherited from upstream). Now: every state-changing call is checked with polkit (action `io.github.dmitry_s93.mcontrolcenter.helper-write`: allowed without a prompt for a local active session, administrator otherwise, refused if polkit is unavailable); raw EC writes are limited to the offsets the app uses, each with a value allowlist (the hottest fan-curve point can never be set below 50 %); sysfs writes accept only plain tokens and range-checked numbers; `quit` and the `ec_sys` module load are gated too; the helper is built with stack protector, FORTIFY and full RELRO/BIND_NOW. Existing installs need the new `.policy` file (install.sh and the packages ship it).
- **Compatibility of the lockdown:** if polkit does not see your session as local and active (bare window manager, no logind, ssh), the dashboard now shows a banner with an Authorize button that lets polkit ask for the administrator password (new helper methods `isAuthorized` / `requestAuthorization`); previously such a setup would have ignored every click silently. On desktops without a system tray the window now opens at start and closing it quits. README has a Requirements section.
- Calibration: remaining-time estimate and MSI-style drain boost (full backlight + light CPU load)
- Panel overdrive toggle, fan dust check (BIOS-gated), firmware card with BIOS age and a community BIOS hint, MSI charge-limit names, unofficial/beta disclaimer
- install.sh restarts the root helper so updates take effect
- Notifications: CPU throttling alert; alert when the battery charge limit is changed by something other than this app
- Optional desktop companion widget (early placeholder)
- Accessibility: every custom-drawn control is now reachable by keyboard (Tab between panels; arrows inside; Space/Enter to press; sliders and the fan curve with arrow keys) with a visible focus ring, and exposed to screen readers (Orca/AT-SPI) as real switches, buttons, radio buttons and sliders; the page tabs are focusable; panels read out their live values

## [0.6.0] 2026-11-15 (fork, p34t1k/MControlCenter)
- Redesigned UI (dashboard, Controls, Battery, Fans, Lighting, Displays, Settings), Plasma Power & Battery applet, session D-Bus API
- Mystic Light keyboard lighting, Displays tab (refresh rate, DDC/CI), discrete-GPU boot mode
- Battery calibration (guided and automatic with charger connected), health history graph, health-drop alert, interrupted-calibration recovery
- Per-app profiles, fan-curve export/import
- Device discovery (no BAT1/ADP1 assumptions), msi-ec doctor, installer with --dry-run, .deb/.rpm packages, CI on 5 distros

## [0.4.1] 2023-08-09
- Fixed UI lags when resizing the window (issue https://github.com/dmitry-s93/MControlCenter/issues/97)
- Updated Norwegian Bokmål (Thanks [msigurdsen](https://github.com/msigurdsen))
- Added support for German language (Thanks [EchterAlsFake](https://github.com/EchterAlsFake))
- Added support for Hungarian language (Thanks [BiRo96](https://github.com/BiRo96))

## [0.4.0] 2023-03-06
- Added fan control support
- Updated Norwegian Bokmål (Thanks [msigurdsen](https://github.com/msigurdsen))
- Updated Spanish translation (Thanks [rottenpants466](https://www.github.com/rottenpants466))
- Updated Italian translation (Thanks [rottenpants466](https://www.github.com/rottenpants466))
- Updated Portuguese translation (Thanks [rottenpants466](https://www.github.com/rottenpants466))

## [0.3.2] 2022-11-28
- Added fan mode display
- Added description on mode tab
- Added "Cooler Boost" item to tray menu
- Added support for [acpi_ec](https://github.com/musikid/acpi_ec) kernel module
- Added support for Italian language (Thanks [marraviglioso](https://github.com/marraviglioso))
- Added support for Portuguese (Brazilian) language (Thanks [rottenpants466](https://www.github.com/rottenpants466))
- Updated Spanish translation (Thanks [rottenpants466](https://www.github.com/rottenpants466))
- Updated Portuguese translation (Thanks [rottenpants466](https://www.github.com/rottenpants466))
- Fixed display of the window when clicking the tray icon if the application was minimized

## [0.3.1] 2022-11-04
- Added support for Norwegian Bokmål (Thanks [msigurdsen](https://github.com/msigurdsen))
- Added support for Turkish language (Thanks [emrgncr](https://github.com/emrgncr))
- Corrected display of the current battery level
- Fixed fan 1 speed display for some devices
- Fixed "FN <-> Super" for some devices (Issue https://github.com/dmitry-s93/MControlCenter/issues/53)

## [0.3.0] 2022-10-28
- Added support for Spanish language (Thanks [ssergio-ll](https://www.github.com/ssergio-ll))
- Ability to set the battery charge limit from 30% to 100% in steps of 1
- Prevent launching more than one instance
- Added application icon to system tray
- Added saving and restoring settings
- Some optimizations
- Fixed turning on/off webcam

## [0.2.0] 2022-10-16
- Added mode change (High Performance, Balanced, Silent, Super Battery)
- Added support for Portuguese language (Thanks [rottenpants466](https://www.github.com/rottenpants466))
- Run helper as D-BUS service
- Some optimizations
- Fixed typos

## [0.1.2] 2022-10-02

- GUI improvements
- Some optimizations
- Added link to bug tracker
- Added "Fully charged" battery status
- Added "Fully charged (Discharging)" battery status

## [0.1.1] 2022-09-21

- Added charging status display
- Added localization support
- Added support for Russian language
- Some optimizations

## [0.1.0 alpha] 2022-09-12

- First release
