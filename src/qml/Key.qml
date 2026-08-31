import QtQuick
import QtQuick.Window
import skvirt 1.0

// One key of the macOS-style panel: a rounded cap with a legend that follows
// the modifier state, an optional small word underneath (⌘ command, ⇧ shift,
// …), and the long-press strip for alternate characters.
Item {
    id: root

    property string keyName: ""        // internal key name sent to controller (e.g. "q", "backspace")
    property string label: keyName     // displayed legend (can differ from keyName)
    property string shiftLabel: ""     // legend when shift is active (empty = uppercase of label)
    property string sub: ""            // small word under the glyph ("command", "shift", …)
    property string align: "c"         // where glyph/word sit: "c"entre, "l"eft, "r"ight
    property bool isFuncKey: false     // flatter, darker cap, as on a Mac keyboard
    property bool active: false        // latched (shift, caps lock, ⌃⌥⌘): accent-filled
    property bool capsIndicator: false // draw the caps-lock indicator light
    property var alternates: []        // long-press entries ({t: legend, k: special key name})
    property bool positional: false    // generated-layout key: commit by physical position
    property MacTheme theme            // shared palette

    // Two half-height keys stacked in one slot — the ▲/▼ pair of the Mac
    // arrow cluster. Entries are [keyName, legend]; empty for a normal key.
    property var stacked: []

    // Letters are uppercase when shift XOR caps-lock is active
    readonly property bool upper: KeyboardController.shiftActive !== KeyboardController.capsLock

    // Computed display legend. The Keyboard Viewer shows one legend per key
    // and swaps it with the modifier state, rather than printing both.
    readonly property string displayLabel: {
        if (KeyboardController.shiftActive && shiftLabel !== "")
            return shiftLabel
        if (upper && label.length === 1 && label >= 'a' && label <= 'z')
            return label.toUpperCase()
        return label
    }

    readonly property bool hasAlternates: Settings.longPressSymbols && alternates.length > 0

    width: 60
    height: 50

    property bool pressed: false

    readonly property real radius: Math.max(4, Math.round(height * 0.13))
    readonly property color capColor: pressed || active ? theme.accent
                                    : isFuncKey ? theme.funcKey
                                    : theme.key
    readonly property color capText: pressed || active ? theme.accentText : theme.text

    // ---- cap ---------------------------------------------------------------

    // Mac keys sit a hair proud of the body: a soft edge under the cap, not a
    // drawn border, is what gives them their depth.
    Rectangle {
        visible: root.stacked.length === 0
        anchors.fill: parent
        anchors.topMargin: 1
        y: 1
        radius: root.radius
        color: root.theme.keyShadow
    }

    Rectangle {
        id: cap
        visible: root.stacked.length === 0
        anchors.fill: parent
        anchors.bottomMargin: 1
        radius: root.radius
        color: root.capColor
        border.color: root.theme.keyBorder
        border.width: 1

        Behavior on color {
            ColorAnimation { duration: 70 }
        }

        // Legend area. Modifier caps carry the glyph at the top and the word
        // at the bottom, both flush with the outer edge of the key, the way a
        // Mac keyboard prints ⇧ over "shift"; plain keys just centre theirs.
        // Positioned rather than anchored, so no key ends up with conflicting
        // left/right/centre anchors as `align` changes.
        Item {
            id: content
            anchors.fill: parent
            anchors.margins: Math.round(root.height * 0.13)

            function alignedX(w) {
                if (root.sub === "" || root.align === "c")
                    return Math.round((width - w) / 2)
                return root.align === "l" ? 0 : Math.round(width - w)
            }

            Text {
                id: legend
                visible: root.label !== ""
                text: root.displayLabel
                color: root.capText
                font.family: root.theme.fontFamily
                font.pixelSize: root.sub !== "" ? Math.round(root.height * 0.30)
                              : root.displayLabel.length > 2 ? Math.round(root.height * 0.26)
                              : Math.round(root.height * 0.40)
                renderType: Text.NativeRendering

                x: content.alignedX(width)
                y: root.sub !== "" ? 0 : Math.round((content.height - height) / 2)
            }

            // The word under the glyph, dropped when the cap is too narrow.
            Text {
                id: word
                visible: root.sub !== "" && width <= content.width
                text: root.sub
                color: root.pressed || root.active ? root.theme.accentText : root.theme.subText
                font.family: root.theme.fontFamily
                font.pixelSize: Math.round(root.height * 0.19)
                renderType: Text.NativeRendering

                x: content.alignedX(width)
                y: content.height - height
            }
        }

        // What a long press on this key offers, small and dimmed in the
        // corner — the alternates strip is otherwise invisible until held.
        // Only the first entry is shown: F1 for "1", à for "a", …
        Text {
            visible: root.hasAlternates && root.sub === ""
            text: root.alternates.length > 0 ? root.alternates[0].t : ""
            color: root.pressed || root.active ? root.theme.accentText : root.theme.subText
            opacity: 0.75
            font.family: root.theme.fontFamily
            font.pixelSize: Math.round(root.height * (text.length > 1 ? 0.17 : 0.20))
            renderType: Text.NativeRendering

            anchors.top: parent.top
            anchors.right: parent.right
            anchors.margins: Math.round(root.height * 0.09)
        }

        // Caps-lock indicator light, in the corner of the cap like the LED on
        // an Apple keyboard.
        Rectangle {
            visible: root.capsIndicator
            width: Math.max(4, Math.round(root.height * 0.09))
            height: width
            radius: width / 2
            color: KeyboardController.capsLock ? root.theme.capsLight : "transparent"
            border.color: KeyboardController.capsLock ? "transparent" : root.theme.keyBorder
            border.width: 1
            anchors.top: parent.top
            anchors.right: parent.right
            anchors.margins: Math.round(root.height * 0.13)
        }
    }

    // ---- stacked half-keys (▲ / ▼) -----------------------------------------

    Column {
        visible: root.stacked.length > 0
        anchors.fill: parent
        spacing: 2

        Repeater {
            model: root.stacked

            Rectangle {
                required property var modelData
                required property int index

                width: root.width
                height: Math.floor((root.height - 2) / 2)
                radius: Math.max(3, Math.round(root.radius * 0.7))
                color: halfTap.pressed ? root.theme.accent : root.theme.funcKey
                border.color: root.theme.keyBorder
                border.width: 1

                Text {
                    anchors.centerIn: parent
                    text: parent.modelData[1]
                    color: halfTap.pressed ? root.theme.accentText : root.theme.text
                    font.family: root.theme.fontFamily
                    font.pixelSize: Math.round(root.height * 0.20)
                    renderType: Text.NativeRendering
                }

                MouseArea {
                    id: halfTap
                    anchors.fill: parent
                    onClicked: KeyboardController.sendSpecial(parent.modelData[0])
                }
            }
        }
    }

    // Control keys routed to dedicated handlers; everything else commits its
    // resolved character (case / symbol layer already applied in displayLabel).
    readonly property var specialKeys: [
        "backspace", "enter", "tab", "escape", "delete",
        "space", "up", "down", "left", "right",
        "f1", "f2", "f3", "f4", "f5", "f6",
        "f7", "f8", "f9", "f10", "f11", "f12"
    ]

    // ---- long-press popup ---------------------------------------------------

    property bool longPressFired: false  // release must not commit the base char
    property bool popupVisible: false
    property int popupIndex: -1          // highlighted strip item (-1 = none)
    property bool reserveActive: false   // topReserve was requested for this popup

    // Ask the keyboard panel to grow upward (top-row popups overflow the
    // window top) and to release that space again.
    signal reserveNeeded(real h)
    signal reserveReleased()

    // Popup strip geometry (kept in sync with the Rectangle below).
    readonly property int popupPad: 6
    readonly property int popupSlot: Math.round(root.height * 0.85)
    readonly property int popupSpacing: 3
    readonly property int popupStripW: alternates.length * (popupSlot + popupSpacing)
                                       - popupSpacing + 2 * popupPad
    readonly property int popupStripH: popupSlot + 2 * popupPad

    // Map a point in key coordinates to a strip item index, or -1 when the
    // point is vertically outside the strip (release there cancels).
    function popupIndexAt(mx, my) {
        if (!popupVisible)
            return -1
        if (my < popup.y || my > popup.y + popup.height)
            return -1
        var idx = Math.floor((mx - popup.x - popupPad) / (popupSlot + popupSpacing))
        return Math.max(0, Math.min(alternates.length - 1, idx))
    }

    function closePopup() {
        root.popupVisible = false
        root.popupIndex = -1
        if (root.reserveActive) {
            root.reserveActive = false
            root.reserveReleased()
        }
    }

    // Commit strip entry #idx: a named key (esc, F1, …) or a plain character.
    function commitAlternate(idx) {
        var alt = root.alternates[idx]
        if (alt.k !== undefined && alt.k !== "")
            KeyboardController.sendSpecial(alt.k)
        else
            KeyboardController.commitText(alt.t)
    }

    Rectangle {
        id: popup
        visible: root.popupVisible
        z: 10
        width: root.popupStripW
        height: root.popupStripH
        // Centered on the key, clamped into the window so edge keys (q/p/a/l)
        // don't clip the strip at the sides.
        x: {
            var keyWindowX = root.mapToItem(null, 0, 0).x
            var winW = Window.window ? Window.window.width : root.width
            return Math.max(4 - keyWindowX,
                            Math.min((root.width - width) / 2,
                                     winW - keyWindowX - width - 4))
        }
        y: -height - 8
        radius: 10
        color: root.theme.popup
        border.color: root.theme.bodyBorder
        border.width: 1

        Row {
            x: root.popupPad
            y: root.popupPad
            spacing: root.popupSpacing

            Repeater {
                model: root.alternates.length

                delegate: Rectangle {
                    required property int index

                    width: root.popupSlot
                    height: root.popupSlot
                    radius: Math.max(4, Math.round(root.popupSlot * 0.15))
                    color: index === root.popupIndex ? root.theme.accent : root.theme.popupItem
                    border.color: root.theme.keyBorder
                    border.width: 1

                    Text {
                        anchors.centerIn: parent
                        text: root.alternates[parent.index].t
                        color: parent.index === root.popupIndex ? root.theme.accentText
                                                                : root.theme.text
                        font.family: root.theme.fontFamily
                        font.pixelSize: Math.round(
                            root.height * (root.alternates[parent.index].t.length > 2 ? 0.22 : 0.34))
                        renderType: Text.NativeRendering
                    }
                }
            }
        }
    }

    MouseArea {
        anchors.fill: parent
        enabled: root.stacked.length === 0

        onPressed: {
            root.pressed = true
            root.longPressFired = false
        }
        onPressAndHold: {
            if (root.hasAlternates) {
                root.longPressFired = true
                root.popupIndex = -1
                root.popupVisible = true
                // Would the strip stick out above the window top? Ask the
                // panel to grow upward (layer-shell, bottom-anchored).
                if (root.mapToItem(null, 0, -(root.popupStripH + 8)).y < 0) {
                    root.reserveActive = true
                    root.reserveNeeded(root.popupStripH + 8)
                }
            }
        }
        onPositionChanged: {
            if (root.popupVisible)
                root.popupIndex = root.popupIndexAt(mouse.x, mouse.y)
        }
        onReleased: {
            root.pressed = false
            if (root.longPressFired) {
                var idx = root.popupIndexAt(mouse.x, mouse.y)
                root.closePopup()
                if (idx >= 0)
                    root.commitAlternate(idx)
                return  // released outside the strip: cancel, type nothing
            }
            var n = root.keyName
            if (n === "shift")
                KeyboardController.toggleShift()
            else if (n === "capslock")
                KeyboardController.toggleCapsLock()
            else if (n === "control" || n === "option" || n === "command")
                KeyboardController.toggleModifier(n)
            else if (n === "symbols" || n === "abc")
                KeyboardController.toggleSymbolMode()
            else if (n === "lang")
                KeyboardController.cycleLayout()
            else if (root.specialKeys.indexOf(n) !== -1)
                KeyboardController.sendSpecial(n)
            else if (root.positional)
                KeyboardController.commitKeyAt(root.keyName, root.upper, root.displayLabel)
            else
                KeyboardController.commitText(root.displayLabel)
        }
        onCanceled: {
            root.pressed = false
            root.closePopup()
        }
    }
}
