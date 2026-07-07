# skvirt

A touch-driven on-screen keyboard for **KDE Plasma / KWin (Wayland)** that works
**with** fcitx5 instead of replacing it.

Unlike input-method-based on-screen keyboards, skvirt does not take over the
Wayland input-method slot and does not put fcitx5 into its virtual-keyboard
mode. It types by creating a **kernel virtual keyboard via `/dev/uinput`**, so
key taps travel the normal input path — libinput → KWin → fcitx5 — exactly like
a physical keyboard. Your active fcitx5 layout and engine (e.g. `keyboard-ru`,
Pinyin) compose the input and show candidates in fcitx5's usual window, and the
fcitx5 tray icon keeps working. fcitx5 never even knows skvirt exists.

## Features

- **Touch-driven, Windows-like:** touching a text field with a finger/pen pops
  the keyboard up; reaching for the mouse/touchpad/trackpoint, losing the field,
  or pressing the ▼ button hides it. A mouse click never summons it.
- **Coexists with fcitx5:** input goes through fcitx5, so layouts, composing and
  CJK candidate selection all work normally.
- **Wayland layer-shell** panel docked at the bottom; never steals input focus
  from the application you are typing into.

## Requirements

- KDE Plasma 6 / KWin on Wayland
- Qt 6 (Core, Gui, Quick, Qml, DBus)
- LayerShellQt
- fcitx5 as the active input method
- The user must be in the **`input`** group (to read the touchscreen via evdev
  and to write to `/dev/uinput`):

  ```sh
  sudo usermod -aG input "$USER"
  ```

  Log out and back in for the group change to take effect. (For a quick test in
  the current session without re-login: `sg input -c '.../build/skvirt'`.)

## Build

```sh
cmake -B build
cmake --build build
```

## Run

```sh
./build/skvirt
```

Then touch a text field — the keyboard appears. To start it automatically at
login, copy `skvirt.desktop` (adjust the `Exec` path) to
`~/.config/autostart/`, or add the binary in *System Settings → Autostart*.

## How it works

- `UinputKeyboard` — creates the `/dev/uinput` virtual keyboard and injects
  US-position evdev keycodes (with Shift as needed). Output follows the active
  fcitx5 layout/IM, per window.
- `InputMonitor` — reads `/dev/input/event*` and classifies devices by kernel
  properties (`INPUT_PROP_DIRECT` = touchscreen vs pointer devices) to tell
  finger/pen from mouse.
- `KWinVk` — asks KWin (`org.kde.kwin.VirtualKeyboard`) whether the focused
  window accepts text input, to know when there is a field to type into.
- `KeyboardWindow` — the layer-shell panel; marked
  `Qt::WindowDoesNotAcceptFocus` so mapping it never pulls focus off the app.

## Limitations

- Input devices are enumerated at startup; hot-plugged keyboards/mice/touchscreens
  are not picked up until restart.
- Key labels are a fixed Latin QWERTY reference; the actual character produced
  depends on the active fcitx5 layout (as with a physical keyboard).
- If focus moves to another text-capable window while in touch mode, the
  keyboard may stay up until you use the mouse.

## License

GPLv3 — see [LICENSE](LICENSE).
