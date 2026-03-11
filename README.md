# dwl-custom

This is a customized build of [dwl](https://codeberg.org/dwl/dwl), a compact [Wayland] compositor based on [wlroots].

## Features

- **XWayland**: Native support enabled by default.
- **Vanity Gaps**: Configurable inner and outer gaps for layouts.
- **Opacity Control**: Real-time control of active/inactive window transparency.
- **IPC Protocol**: Integrated inter-process communication for status bars and automation.
- **WIP: Unified dwlctl**: A single CLI tool for window management, opacity control, and state querying.

## Patches

Most of my own modifications build upon these patches.

- client-opacity-focus
- customfloat
- ipc
- riverctl
- scenefx
- vanitygaps
- warpcursor

## Dependencies

- libinput, wayland, wlroots (0.18), xkbcommon
- libxcb, libxcb-wm (for XWayland)
- wayland-protocols, pkg-config (build-time)

## Building and Installation

```bash
make
sudo make install
```

## dwlctl Usage

`dwlctl` is the single source of truth for interacting with the compositor.
This is very much work-in-progress for now, and not all commands work yet.

- **Manage**: `dwlctl setmfact 0.5`, `dwlctl tag 1`, `dwlctl set-opacity-focus 0.8`
- **Query**: `dwlctl get clients`, `dwlctl query display eDP-1`, `dwlctl get master`
- **Monitor**: `dwlctl -m` (continuously stream status events to stdout)
- **Help**: `dwlctl --help` provides a full list of supported commands and arguments.

## Configuration

All configuration is done by editing `config.h` and recompiling. Changes take effect upon restarting the display server.
Note that you can use `dwlctl` to change many options on the fly.

## Links

- [Official dwl Repository](https://codeberg.org/dwl/dwl)
- [Official Wiki](https://codeberg.org/dwl/dwl/wiki)
- [dwl-patches](https://codeberg.org/dwl/dwl-patches)
- [wlroots](https://gitlab.freedesktop.org/wlroots/wlroots)

[Wayland]: https://wayland.freedesktop.org/
