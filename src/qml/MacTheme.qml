import QtQuick

// Colour palette for the macOS-style panel. One instance lives in
// SkvirtKeyboard and is handed to every Key, so the whole keyboard repaints
// from a single place when the theme setting (or the system scheme) changes.
QtObject {
    id: theme

    // Settings.theme: 0 = follow the system colour scheme, 1 = light, 2 = dark.
    property int mode: 0

    // Qt reports the desktop's scheme through the platform theme / the
    // freedesktop appearance portal; Unknown (no portal) reads as light,
    // which is what an unthemed Plasma session shows.
    readonly property bool systemDark:
        Application.styleHints.colorScheme === Qt.Dark

    readonly property bool dark: mode === 2 || (mode === 0 && systemDark)

    // Apple's system font when it is installed (it carries ⌘ ⌥ ⌃ ⇧ ⇪ as well);
    // otherwise the closest thing the machine has. The QML font value type
    // takes a single family, so the fallback list is resolved here once.
    readonly property string fontFamily: {
        var installed = Qt.fontFamilies()
        var wanted = ["SF Pro Text", "SF Pro", "Inter", "Noto Sans", "DejaVu Sans"]
        for (var i = 0; i < wanted.length; i++) {
            if (installed.indexOf(wanted[i]) >= 0)
                return wanted[i]
        }
        return ""  // whatever Qt picks by default
    }

    // The keyboard body — a translucent slab floating over the app, like the
    // macOS Keyboard Viewer window.
    readonly property color body:        dark ? "#F22B2B2E" : "#F2E9E9EB"
    readonly property color bodyBorder:  dark ? "#33FFFFFF" : "#26000000"
    readonly property color bodyTopEdge: dark ? "#18FFFFFF" : "#B3FFFFFF"

    // Keys: light-grey caps on macOS light, mid-grey on dark. Function and
    // modifier keys sit one step darker/flatter, as on a real Mac keyboard.
    readonly property color key:        dark ? "#4B4B4F" : "#FFFFFF"
    readonly property color funcKey:    dark ? "#3A3A3D" : "#DCDCDF"
    readonly property color keyBorder:  dark ? "#26000000" : "#1F000000"
    readonly property color keyShadow:  dark ? "#40000000" : "#26000000"

    readonly property color text:      dark ? "#F5F5F7" : "#1D1D1F"
    readonly property color subText:   dark ? "#AEAEB2" : "#6E6E73"
    readonly property color accentText:  "#FFFFFF"

    // macOS system blue, and the green of the caps-lock indicator light.
    readonly property color accent:     dark ? "#0A84FF" : "#007AFF"
    readonly property color capsLight:  dark ? "#30D158" : "#28CD41"

    // Long-press strip and suggestion pills.
    readonly property color popup:       dark ? "#FA3A3A3D" : "#FAF7F7F8"
    readonly property color popupItem:   dark ? "#54545A" : "#FFFFFF"
    readonly property color pill:        dark ? "#48484C" : "#FFFFFF"
    readonly property color pillBorder:  dark ? "#26FFFFFF" : "#1F000000"
}
