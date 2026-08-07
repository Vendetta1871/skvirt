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

![skvirt keyboard with pinyin suggestions](screenshots/keyboard.png)

## Features

- **Touch-driven, Windows-like:** touching a text field with a finger/pen pops
  the keyboard up; reaching for the mouse/touchpad/trackpoint, losing the field,
  or pressing the ▼ button hides it. A mouse click never summons it.
- **Coexists with fcitx5:** input goes through fcitx5, so layouts, composing and
  CJK candidate selection all work normally.
- **Autosuggestion bar:** word completions above the key rows — Hunspell
  dictionaries for Latin/Cyrillic layouts, and **libime pinyin candidates**
  (the same engine and data as fcitx5-pinyin) when a Chinese IM is active.
  Tapping a pinyin candidate commits the hanzi directly.
- **Layout generator:** the on-screen rows are generated at runtime from the
  active IM's xkb symbols (`/usr/share/X11/xkb/symbols/*`), so any
  `keyboard-XX` layout (de, fr, …) shows the right legends. Hardcoded
  QWERTY/ЙЦУКЕН remain as fallback.
- **Long-press symbols:** holding a letter opens an alternates strip
  (digits, accented variants, ё/й, shifted punctuation); slide to pick,
  release to type.
- **Optional numbers row** that shrinks the keys instead of growing the panel.
- **Tablet-mode aware:** can auto-show only when the convertible is folded
  into tablet mode (via the `SW_TABLET_MODE` evdev switch).
- **KDE settings module:** *System Settings → skvirt* (KCM) toggles all of
  the above live, no restart needed.
- **Wayland layer-shell** panel docked at the bottom; never steals input focus
  from the application you are typing into.

## Requirements

- KDE Plasma 6 / KWin on Wayland
- Qt 6 (Core, Gui, Quick, Qml, DBus)
- LayerShellQt
- KF6: Config, CoreAddons, I18n, KCMUtils (+ Kirigami at runtime, for the
  settings module)
- **libime** (pinyin suggestions) and **hunspell** (Latin/Cyrillic
  suggestions; dictionaries `hunspell-en_us`, `hunspell-ru`, … install
  separately — without them only pinyin suggestions work)
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

## Settings

Install (`sudo cmake --install build`) to get the **skvirt** page in System
Settings (or run it from the build tree:
`QT_PLUGIN_PATH=build/bin kcmshell6 kcm_skvirt`). It edits
`~/.config/skvirtrc`; a running skvirt picks changes up live:

- **Show only in tablet mode** — auto-show only while the convertible's
  `SW_TABLET_MODE` switch is engaged.
- **Hide when mouse is moved** — reaching for a pointer device dismisses the
  panel (default on).
- **Show autosuggestions** — completion/candidate bar above the keys.
- **Show symbols on long press** — alternates popup on held letters.
- **Numbers row** — extra digit row; keys shrink, panel height is unchanged.

![skvirt settings page in System Settings](screenshots/settings.png)

## How it works

- `UinputKeyboard` — creates the `/dev/uinput` virtual keyboard and injects
  US-position evdev keycodes (with Shift as needed). Output follows the active
  fcitx5 layout/IM, per window. Arbitrary Unicode (hanzi candidates, accented
  letters) goes through fcitx5's unicode addon direct mode (Ctrl+Shift+U,
  hex, Enter).
- `InputMonitor` — reads `/dev/input/event*` and classifies devices by kernel
  properties (`INPUT_PROP_DIRECT` = touchscreen vs pointer devices) to tell
  finger/pen from mouse; also tracks the `SW_TABLET_MODE` switch.
- `KWinVk` — asks KWin (`org.kde.kwin.VirtualKeyboard`) whether the focused
  window accepts text input, to know when there is a field to type into.
- `KeyboardWindow` — the layer-shell panel; marked
  `Qt::WindowDoesNotAcceptFocus` so mapping it never pulls focus off the app.
- `SuggestionEngine` — word completions: Hunspell dictionaries for keyboard
  layouts, libime (`PinyinIME` + `sc.dict` + `zh_CN.lm`) for pinyin IMs.
- `LayoutGenerator` — parses the active layout's xkb symbols into QML rows.
- `SettingsBridge` + `kcm/` — KConfigXT settings (`skvirtrc`) shared with a
  KQuickManagedConfigModule KCM; `KConfigWatcher` reloads them live.

## Limitations

- Input devices are enumerated at startup; hot-plugged keyboards/mice/touchscreens
  are not picked up until restart.
- Generated layouts cover the three letter rows (group 1 of the `basic` xkb
  section); exotic variants, dead keys and multi-group layouts fall back to
  the hardcoded QWERTY/ЙЦУКЕН rows.
- Pinyin candidates committed from the suggestion bar do not teach fcitx5's
  history (the candidate is injected as Unicode after cancelling the preedit).
- If focus moves to another text-capable window while in touch mode, the
  keyboard may stay up until you use the mouse.

## License

GPLv3 — see [LICENSE](LICENSE).
