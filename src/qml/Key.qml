import QtQuick
import skvirt 1.0

Rectangle {
    id: root

    property string keyName: ""        // internal key name sent to controller (e.g. "q", "backspace")
    property string label: keyName     // displayed label (can differ from keyName)
    property string shiftLabel: ""     // label when shift is active (empty = uppercase of label)
    property bool isFuncKey: false     // darker background for function keys

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

    // Control keys routed to dedicated handlers; everything else commits its
    // resolved character (case / symbol layer already applied in displayLabel).
    readonly property var specialKeys: [
        "backspace", "enter", "tab", "escape", "delete",
        "space", "up", "down", "left", "right"
    ]

    MouseArea {
        anchors.fill: parent
        onPressed: root.pressed = true
        onReleased: {
            root.pressed = false
            var n = root.keyName
            if (n === "shift")
                KeyboardController.toggleShift()
            else if (n === "symbols" || n === "abc")
                KeyboardController.toggleSymbolMode()
            else if (root.specialKeys.indexOf(n) !== -1)
                KeyboardController.sendSpecial(n)
            else
                KeyboardController.commitText(root.displayLabel)
        }
        onCanceled: root.pressed = false
    }
}
