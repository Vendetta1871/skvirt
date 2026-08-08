import QtQuick
import QtQuick.Window
import skvirt 1.0

Rectangle {
    id: root

    property string keyName: ""        // internal key name sent to controller (e.g. "q", "backspace")
    property string label: keyName     // displayed label (can differ from keyName)
    property string shiftLabel: ""     // label when shift is active (empty = uppercase of label)
    property bool isFuncKey: false     // darker background for function keys
    property string alternates: ""     // long-press popup chars ("" = no long-press)
    property string corner: ""         // small hint glyph in the key's top-right corner
    property bool positional: false    // generated-layout key: commit by physical position

    // Letters are uppercase when shift XOR caps-lock is active
    readonly property bool upper: KeyboardController.shiftActive !== KeyboardController.capsLock

    // Computed display label
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
    radius: 6

    color: pressed ? "#3daee9"
         : isFuncKey ? "#2a2e32"
         : "#31363b"

    border.color: "#1a1d20"
    border.width: 1

    property bool pressed: false

    Behavior on color {
        ColorAnimation { duration: 80 }
    }

    Text {
        anchors.centerIn: parent
        text: root.displayLabel
        color: "#eff0f1"
        font.pixelSize: root.height * 0.38
        font.family: "Noto Sans"
        renderType: Text.NativeRendering
    }

    // Maliit-style corner hint: the long-press symbol, small and dimmed.
    Text {
        visible: root.hasAlternates && root.corner.length > 0
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.margins: Math.round(root.height * 0.08)
        text: root.corner
        color: "#eff0f1"
        opacity: 0.55
        font.pixelSize: root.height * 0.22
        font.family: "Noto Sans"
        renderType: Text.NativeRendering
    }

    // Control keys routed to dedicated handlers; everything else commits its
    // resolved character (case / symbol layer already applied in displayLabel).
    readonly property var specialKeys: [
        "backspace", "enter", "tab", "escape", "delete",
        "space", "up", "down", "left", "right"
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
    readonly property int popupSlot: Math.round(root.height * 0.8)
    readonly property int popupSpacing: 2
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
        y: -height - 6
        radius: 8
        color: "#31363b"
        border.color: "#1a1d20"
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
                    radius: 6
                    color: index === root.popupIndex ? "#3daee9" : "#232629"

                    Text {
                        anchors.centerIn: parent
                        text: root.alternates.charAt(index)
                        color: "#eff0f1"
                        font.pixelSize: root.height * 0.34
                        font.family: "Noto Sans"
                        renderType: Text.NativeRendering
                    }
                }
            }
        }
    }

    MouseArea {
        anchors.fill: parent

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
                if (root.mapToItem(null, 0, -(root.popupStripH + 6)).y < 0) {
                    root.reserveActive = true
                    root.reserveNeeded(root.popupStripH + 6)
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
                    KeyboardController.commitText(root.alternates.charAt(idx))
                return  // released outside the strip: cancel, type nothing
            }
            var n = root.keyName
            if (n === "shift")
                KeyboardController.toggleShift()
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
