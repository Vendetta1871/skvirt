import QtQuick
import skvirt 1.0

// The panel: a macOS Keyboard Viewer-style slab floating at the bottom of the
// screen. The key set is a full ANSI/Mac keyboard — number row, the three
// letter rows framed by tab/caps lock/shift/return/delete, and a bottom row of
// 🌐 ⌃ ⌥ ⌘ space ⌘ ⌥ plus the inverted-T arrow cluster — with an optional
// esc/F1–F12 row on top.
Rectangle {
    id: root
    color: "transparent"

    MacTheme {
        id: pal
        mode: Settings.theme
    }

    // Row data: [keyName, label, shiftLabel, widthFactor, subLabel, align].
    // Every row adds up to 15 units, so the rows line up like a real keyboard
    // instead of each stretching to the panel width.

    readonly property var kDelete: ["backspace", "⌫", "", 2, "delete", "r"]
    readonly property var kTab:    ["tab", "⇥", "", 1.5, "tab", "l"]
    readonly property var kCaps:   ["capslock", "⇪", "", 1.75, "caps lock", "l"]
    readonly property var kReturn: ["enter", "↩", "", 2.25, "return", "r"]
    readonly property var kLShift: ["shift", "⇧", "", 2.25, "shift", "l"]
    readonly property var kRShift: ["shift", "⇧", "", 2.75, "shift", "r"]

    // esc + F1–F12: 1.5 + 12 × 1.125 = 15 units.
    readonly property var functionRowData: [
        ["escape","esc","",1.5,"",  "c"],
        ["f1","F1","",1.125,"","c"],   ["f2","F2","",1.125,"","c"],
        ["f3","F3","",1.125,"","c"],   ["f4","F4","",1.125,"","c"],
        ["f5","F5","",1.125,"","c"],   ["f6","F6","",1.125,"","c"],
        ["f7","F7","",1.125,"","c"],   ["f8","F8","",1.125,"","c"],
        ["f9","F9","",1.125,"","c"],   ["f10","F10","",1.125,"","c"],
        ["f11","F11","",1.125,"","c"], ["f12","F12","",1.125,"","c"]
    ]

    // The Mac bottom row. 🌐 is the input-source key: it cycles the fcitx IM,
    // so its small label is the active layout code.
    readonly property var bottomRowData: [
        ["lang", "🌐", "", 1, KeyboardController.layoutLabel, "c"],
        ["control", "⌃", "", 1, "control", "l"],
        ["option", "⌥", "", 1, "option", "l"],
        ["command", "⌘", "", 1.25, "command", "l"],
        ["space", "", "", 5.5, "", "c"],
        ["command", "⌘", "", 1.25, "command", "r"],
        ["option", "⌥", "", 1, "option", "r"],
        ["left", "◀", "", 1, "", "c"],
        ["updown", "", "", 1, "", "c"],
        ["right", "▶", "", 1, "", "c"]
    ]

    // Fallback typing rows, used when the active IM is not a keyboard-* one
    // (pinyin and friends compose from latin) or xkb parsing failed. Same
    // four-row shape LayoutGenerator produces: number row + AD/AC/AB.
    readonly property var rowsQwerty: [
        [
            ["`","`","~",1], ["1","1","!",1], ["2","2","@",1], ["3","3","#",1],
            ["4","4","$",1], ["5","5","%",1], ["6","6","^",1], ["7","7","&",1],
            ["8","8","*",1], ["9","9","(",1], ["0","0",")",1], ["-","-","_",1],
            ["=","=","+",1]
        ],
        [
            ["q","q","Q",1], ["w","w","W",1], ["e","e","E",1], ["r","r","R",1],
            ["t","t","T",1], ["y","y","Y",1], ["u","u","U",1], ["i","i","I",1],
            ["o","o","O",1], ["p","p","P",1], ["[","[","{",1], ["]","]","}",1],
            ["\\","\\","|",1.5]
        ],
        [
            ["a","a","A",1], ["s","s","S",1], ["d","d","D",1], ["f","f","F",1],
            ["g","g","G",1], ["h","h","H",1], ["j","j","J",1], ["k","k","K",1],
            ["l","l","L",1], [";",";",":",1], ["'","'","\"",1]
        ],
        [
            ["z","z","Z",1], ["x","x","X",1], ["c","c","C",1], ["v","v","V",1],
            ["b","b","B",1], ["n","n","N",1], ["m","m","M",1], [",",",","<",1],
            [".",".",">",1], ["/","/","?",1]
        ]
    ]

    // Russian (ЙЦУКЕН): same physical keys, Cyrillic legends.
    readonly property var rowsRussian: [
        [
            ["`","ё","Ё",1], ["1","1","!",1], ["2","2","\"",1], ["3","3","№",1],
            ["4","4",";",1], ["5","5","%",1], ["6","6",":",1], ["7","7","?",1],
            ["8","8","*",1], ["9","9","(",1], ["0","0",")",1], ["-","-","_",1],
            ["=","=","+",1]
        ],
        [
            ["q","й","Й",1], ["w","ц","Ц",1], ["e","у","У",1], ["r","к","К",1],
            ["t","е","Е",1], ["y","н","Н",1], ["u","г","Г",1], ["i","ш","Ш",1],
            ["o","щ","Щ",1], ["p","з","З",1], ["[","х","Х",1], ["]","ъ","Ъ",1],
            ["\\","\\","/",1.5]
        ],
        [
            ["a","ф","Ф",1], ["s","ы","Ы",1], ["d","в","В",1], ["f","а","А",1],
            ["g","п","П",1], ["h","р","Р",1], ["j","о","О",1], ["k","л","Л",1],
            ["l","д","Д",1], [";","ж","Ж",1], ["'","э","Э",1]
        ],
        [
            ["z","я","Я",1], ["x","ч","Ч",1], ["c","с","С",1], ["v","м","М",1],
            ["b","и","И",1], ["n","т","Т",1], ["m","ь","Ь",1], [",","б","Б",1],
            [".","ю","Ю",1], ["/",".",",",1]
        ]
    ]

    // Long press. Every key that hides something prints it small in its own
    // corner (Gboard-style) and offers it first in the strip.
    //
    // Letters and punctuation carry the digit/symbol of their physical
    // position — keyed by PHYSICAL key name, so they hold for every layout —
    // followed by the accented variants of the letter actually on the cap.
    readonly property var cornerSymbols: ({
        "q": "1", "w": "2", "e": "3", "r": "4", "t": "5",
        "y": "6", "u": "7", "i": "8", "o": "9", "p": "0",
        "a": "@", "s": "#", "d": "$", "f": "-", "g": "&",
        "h": "_", "j": "+", "k": "(", "l": ")",
        "z": "`", "x": "\"", "c": ".", "v": ":", "b": ";",
        "n": "!", "m": "?",
        "[": "{", "]": "}", ";": ":", "'": "\"", ",": "<", ".": ">",
        "/": "?", "\\": "|"
    })

    // The number row instead hides the function row, whenever that row is off:
    // hold ` for esc, 1 for F1, … = for F12. Its shifted symbols need no
    // corner hint — they are one Shift away on a full keyboard.
    readonly property var functionAlternates: ({
        "`": {t: "esc", k: "escape"},
        "1": {t: "F1",  k: "f1"},  "2": {t: "F2",  k: "f2"},
        "3": {t: "F3",  k: "f3"},  "4": {t: "F4",  k: "f4"},
        "5": {t: "F5",  k: "f5"},  "6": {t: "F6",  k: "f6"},
        "7": {t: "F7",  k: "f7"},  "8": {t: "F8",  k: "f8"},
        "9": {t: "F9",  k: "f9"},  "0": {t: "F10", k: "f10"},
        "-": {t: "F11", k: "f11"}, "=": {t: "F12", k: "f12"}
    })
    readonly property var accentChars: ({
        "a": "àáâäãå", "e": "èéêë", "i": "ìíîï", "o": "òóôöõ", "u": "ùúûü",
        "y": "ÿ", "c": "ç", "n": "ñ", "s": "ß",
        "е": "ё", "ё": "е", "и": "й", "й": "и"
    })

    function altsFor(keyName, label) {
        var out = []
        var fn = functionAlternates[keyName]
        if (fn !== undefined) {
            // Number row: the function key, or nothing once the row is shown.
            if (!Settings.functionRow)
                out.push(fn)
            return out
        }
        var sym = cornerSymbols[keyName]
        if (sym !== undefined)
            out.push({t: sym, k: ""})
        var acc = accentChars[label]
        if (acc !== undefined) {
            for (var i = 0; i < acc.length; i++)
                out.push({t: acc.charAt(i), k: ""})
        }
        return out
    }

    // The four typing rows: generated from the active IM's xkb layout when
    // that worked, hardcoded otherwise.
    readonly property bool generatedActive: KeyboardController.generatedRows.length >= 4
    readonly property var typingRows: {
        if (generatedActive)
            return KeyboardController.generatedRows
        return KeyboardController.layout === "keyboard-ru" ? rowsRussian : rowsQwerty
    }

    readonly property var currentRows: {
        var rows = []
        if (Settings.functionRow)
            rows.push(functionRowData)
        rows.push(typingRows[0].concat([kDelete]))
        rows.push([kTab].concat(typingRows[1]))
        rows.push([kCaps].concat(typingRows[2], [kReturn]))
        rows.push([kLShift].concat(typingRows[3], [kRShift]))
        rows.push(bottomRowData)
        return rows
    }

    // Generated-layout rows are positional (committed by physical key, not by
    // character): the four typing rows, after the optional function row.
    readonly property int posRowStart: Settings.functionRow ? 1 : 0

    // ---- geometry ----------------------------------------------------------

    readonly property real keySpacing: 5
    readonly property real rowSpacing: 5
    readonly property real bodyPad: 10
    readonly property real sidePad: 8

    // Widest row in key units — every row is laid out against this one, so
    // they share a unit and line up like the rows of a real keyboard.
    readonly property real maxFactor: {
        var m = 0
        for (var r = 0; r < currentRows.length; r++) {
            var sum = 0
            for (var i = 0; i < currentRows[r].length; i++)
                sum += currentRows[r][i][3]
            if (sum > m)
                m = sum
        }
        return m > 0 ? m : 15
    }

    // Header strip: the hide button, and the suggestion pills when there are
    // any. Sized off the screen rather than off the keys, so the key unit
    // below can be derived from it without a binding loop.
    readonly property bool barVisible: Settings.showAutosuggestions
                                    && KeyboardController.suggestions.length > 0
    readonly property real barH: Math.max(30, Math.round(Screen.height * 0.033))
    readonly property real headerH: Math.max(20, Math.round(Screen.height * 0.022))
    readonly property real barArea: barVisible ? barH + rowSpacing : 0

    // Key unit. The keys are square, as on a Mac keyboard, so the panel takes
    // whichever of the two limits is tighter: the width it may occupy, or the
    // height budget (the same ~38% of the screen the panel always used).
    // barArea is deliberately left out: when the suggestion bar appears the
    // panel grows by it instead of the keys shrinking under the user's finger.
    readonly property real heightBudget: Math.round(Screen.height * 0.38)
    readonly property int rowCount: currentRows.length
    readonly property real unitByWidth:
        (root.width - 2 * sidePad - 2 * bodyPad + keySpacing) / maxFactor
    readonly property real unitByHeight:
        (heightBudget - headerH - rowSpacing - 2 * bodyPad
         - (rowCount - 1) * rowSpacing) / rowCount
    readonly property real unit: Math.max(20, Math.min(unitByWidth, unitByHeight))
    readonly property real keyH: unit

    readonly property int bodyW: Math.round(maxFactor * unit - keySpacing + 2 * bodyPad)
    readonly property int bodyH: Math.round(2 * bodyPad + headerH + rowSpacing + barArea
                                            + rowCount * unit + (rowCount - 1) * rowSpacing)

    // Long-press popup reserve: when a popup would stick out above the panel
    // top, the window temporarily grows upward by topReserve (transparent
    // strip — the popup renders there, over the app behind). zoneHeight
    // excludes it so the exclusive zone never jumps with popups.
    property real topReserve: 0
    readonly property int desiredHeight: bodyH + Math.round(topReserve)
    readonly property int zoneHeight: bodyH

    // ---- the keyboard body -------------------------------------------------

    Rectangle {
        id: body
        // Bottom-anchored: when the window grows upward for a long-press
        // popup (topReserve), the free space lands above the keys and the
        // keys stay exactly where they were.
        anchors.bottom: parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        width: root.bodyW
        height: root.bodyH

        color: pal.body
        // Docked at the screen edge: rounded at the top, flush at the bottom.
        topLeftRadius: 14
        topRightRadius: 14
        bottomLeftRadius: 0
        bottomRightRadius: 0
        border.color: pal.bodyBorder
        border.width: 1

        // Faint highlight along the top edge, the way macOS lights the rim of
        // a floating panel.
        Rectangle {
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.margins: 1
            anchors.topMargin: 1
            height: 1
            color: pal.bodyTopEdge
        }

        Column {
            anchors.fill: parent
            anchors.margins: root.bodyPad
            spacing: root.rowSpacing

            // Header: suggestions in the middle, hide button on the right.
            Item {
                width: parent.width
                height: root.headerH

                Text {
                    anchors.left: parent.left
                    anchors.verticalCenter: parent.verticalCenter
                    visible: !root.barVisible
                    text: KeyboardController.layoutLabel.toUpperCase()
                    color: pal.subText
                    opacity: 0.8
                    font.family: pal.fontFamily
                    font.pixelSize: Math.round(root.headerH * 0.6)
                    renderType: Text.NativeRendering
                }

                Rectangle {
                    id: hideButton
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    width: Math.round(root.headerH * 1.6)
                    height: Math.round(root.headerH * 0.85)
                    radius: height / 2
                    color: hideArea.pressed ? pal.accent : pal.pill
                    border.color: pal.pillBorder
                    border.width: 1

                    Text {
                        anchors.centerIn: parent
                        text: "▾"
                        color: hideArea.pressed ? pal.accentText : pal.subText
                        font.family: pal.fontFamily
                        font.pixelSize: Math.round(parent.height * 0.7)
                        renderType: Text.NativeRendering
                    }

                    MouseArea {
                        id: hideArea
                        anchors.fill: parent
                        onClicked: KeyboardController.hidePanel()
                    }
                }
            }

            // Autosuggestion bar
            Item {
                id: suggestionBar
                visible: root.barVisible
                width: parent.width
                height: root.barH

                Row {
                    anchors.centerIn: parent
                    spacing: 8

                    Repeater {
                        model: KeyboardController.suggestions

                        Rectangle {
                            required property string modelData
                            required property int index

                            width: pillLabel.implicitWidth + 26
                            height: Math.round(suggestionBar.height - 6)
                            radius: height / 2
                            color: pillTap.pressed ? pal.accent : pal.pill
                            border.color: pal.pillBorder
                            border.width: 1

                            Text {
                                id: pillLabel
                                anchors.centerIn: parent
                                text: parent.modelData
                                color: pillTap.pressed ? pal.accentText : pal.text
                                font.family: pal.fontFamily
                                font.pixelSize: Math.round(suggestionBar.height * 0.46)
                                renderType: Text.NativeRendering
                            }

                            MouseArea {
                                id: pillTap
                                anchors.fill: parent
                                onClicked: KeyboardController.commitSuggestion(parent.index)
                            }
                        }
                    }
                }
            }

            // Key rows
            Repeater {
                model: root.currentRows

                Row {
                    required property var modelData
                    required property int index

                    spacing: root.keySpacing
                    anchors.horizontalCenter: parent.horizontalCenter

                    // Generated-layout typing rows commit by physical position.
                    readonly property bool positionalRow: root.generatedActive
                                                       && index >= root.posRowStart
                                                       && index <= root.posRowStart + 3

                    Repeater {
                        model: parent.modelData

                        Key {
                            required property var modelData

                            theme:      pal
                            keyName:    modelData[0]
                            label:      modelData[1]
                            shiftLabel: modelData[2]
                            sub:        modelData.length > 4 ? modelData[4] : ""
                            align:      modelData.length > 5 ? modelData[5] : "c"
                            alternates: root.altsFor(modelData[0], modelData[1])
                            width:      Math.round(modelData[3] * root.unit - root.keySpacing)
                            height:     Math.round(root.keyH)
                            positional: parent.positionalRow
                            stacked:    modelData[0] === "updown"
                                        ? [["up", "▲"], ["down", "▼"]] : []

                            // Everything that is not a character key gets the
                            // flatter Mac modifier cap; the space bar keeps
                            // the plain one.
                            isFuncKey: modelData[0].length > 1 && modelData[0] !== "space"

                            capsIndicator: modelData[0] === "capslock"
                            active: {
                                var n = modelData[0]
                                if (n === "shift")
                                    return KeyboardController.shiftActive
                                if (n === "capslock")
                                    return KeyboardController.capsLock
                                if (n === "control")
                                    return KeyboardController.controlActive
                                if (n === "option")
                                    return KeyboardController.optionActive
                                if (n === "command")
                                    return KeyboardController.commandActive
                                return false
                            }

                            // Long-press popup overflow: reserve transparent
                            // space above the panel while the popup is open.
                            onReserveNeeded: function(h) { root.topReserve = h }
                            onReserveReleased: root.topReserve = 0
                        }
                    }
                }
            }
        }
    }
}
