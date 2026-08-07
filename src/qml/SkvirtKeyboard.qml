import QtQuick
import skvirt 1.0

Rectangle {
    id: root
    color: "#232629"

    // Keyboard rows data: [keyName, label, shiftLabel, widthFactor]
    // widthFactor: 1.0 = normal key, 1.5 = wide, 2.0 = extra wide
    readonly property var rowsQwerty: [
        // Row 0: Q-P
        [
            ["q","q","Q",1], ["w","w","W",1], ["e","e","E",1], ["r","r","R",1],
            ["t","t","T",1], ["y","y","Y",1], ["u","u","U",1], ["i","i","I",1],
            ["o","o","O",1], ["p","p","P",1]
        ],
        // Row 1: A-L
        [
            ["a","a","A",1], ["s","s","S",1], ["d","d","D",1], ["f","f","F",1],
            ["g","g","G",1], ["h","h","H",1], ["j","j","J",1], ["k","k","K",1],
            ["l","l","L",1]
        ],
        // Row 2: Shift + Z-M + Backspace
        [
            ["shift", KeyboardController.capsLock ? "⇪" : KeyboardController.shiftActive ? "⇧" : "⇧", "", 1.5],
            ["z","z","Z",1], ["x","x","X",1], ["c","c","C",1], ["v","v","V",1],
            ["b","b","B",1], ["n","n","N",1], ["m","m","M",1],
            ["backspace","⌫","",1.5]
        ],
        // Row 3: Lang / Symbols / Space / Enter
        [
            ["lang", KeyboardController.layoutLabel.toUpperCase(), "", 1],
            ["symbols","?123","",1.3],
            [",",",","<",1], ["space","  space  ","",3], [".",".","!",1],
            ["enter","↵","",1.3]
        ]
    ]

    // Russian (ЙЦУКЕН): same physical keys as rowsQwerty, Cyrillic legends.
    readonly property var rowsRussian: [
        // Row 0: Ё Й-Ъ
        [
            ["yo","ё","Ё",0.8],
            ["q","й","Й",1], ["w","ц","Ц",1], ["e","у","У",1], ["r","к","К",1],
            ["t","е","Е",1], ["y","н","Н",1], ["u","г","Г",1], ["i","ш","Ш",1],
            ["o","щ","Щ",1], ["p","з","З",1], ["[","х","Х",1], ["]","ъ","Ъ",1]
        ],
        // Row 1: Ф-Э
        [
            ["a","ф","Ф",1], ["s","ы","Ы",1], ["d","в","В",1], ["f","а","А",1],
            ["g","п","П",1], ["h","р","Р",1], ["j","о","О",1], ["k","л","Л",1],
            ["l","д","Д",1], [";","ж","Ж",1], ["'","э","Э",1]
        ],
        // Row 2: Shift + Я-Ю + Backspace
        [
            ["shift", KeyboardController.capsLock ? "⇪" : KeyboardController.shiftActive ? "⇧" : "⇧", "", 1.5],
            ["z","я","Я",1], ["x","ч","Ч",1], ["c","с","С",1], ["v","м","М",1],
            ["b","и","И",1], ["n","т","Т",1], ["m","ь","Ь",1],
            [",","б","Б",1], [".","ю","Ю",1],
            ["backspace","⌫","",1.5]
        ],
        // Row 3: Lang / Symbols / Space / Enter
        [
            ["lang", KeyboardController.layoutLabel.toUpperCase(), "", 1],
            ["symbols","?123","",1.3],
            ["space","  space  ","",5],
            ["enter","↵","",1.3]
        ]
    ]

    // Optional numbers row, prepended to letter rows when Settings.numbersRow
    // is enabled (symbol mode already has digits, so no extra row there).
    readonly property var numbersRowData: [
        ["1","1","!",1], ["2","2","@",1], ["3","3","#",1], ["4","4","$",1],
        ["5","5","%",1], ["6","6","^",1], ["7","7","&",1], ["8","8","*",1],
        ["9","9","(",1], ["0","0",")",1]
    ]

    // Long-press data. cornerSymbols (maliit-style, keyed by PHYSICAL key
    // name so they work for every layout) are drawn as a small glyph in the
    // key's corner and always come first in the popup strip; accented
    // variants of the letter (keyed by label) follow in the popup only.
    readonly property var cornerSymbols: ({
        "q": "1", "w": "2", "e": "3", "r": "4", "t": "5",
        "y": "6", "u": "7", "i": "8", "o": "9", "p": "0",
        "a": "@", "s": "#", "d": "$", "f": "-", "g": "&",
        "h": "_", "j": "+", "k": "(", "l": ")",
        "z": "`", "x": "\"", "c": ".", "v": ":", "b": ";",
        "n": "!", "m": "?",
        "[": "{", "]": "}", ";": ":", "'": "\"", ",": "<", ".": ">",
        "/": "?", "-": "_", "=": "+", "\\": "|", "`": "~"
    })
    readonly property var accentChars: ({
        "a": "àáâä", "e": "èéêë", "i": "ìíîï", "o": "òóôö", "u": "ùúûü",
        "c": "ç", "n": "ñ", "s": "ß",
        "е": "ё", "ё": "е", "и": "й", "й": "и"
    })

    function cornerFor(keyName) {
        var c = cornerSymbols[keyName]
        return c === undefined ? "" : c
    }

    function altFor(keyName, label) {
        var s = cornerFor(keyName)
        var a = accentChars[label]
        return a === undefined ? s : s + a
    }

    readonly property var rowsSymbol: [
        [
            ["1","1","!",1], ["2","2","@",1], ["3","3","#",1], ["4","4","$",1],
            ["5","5","%",1], ["6","6","^",1], ["7","7","&",1], ["8","8","*",1],
            ["9","9","(",1], ["0","0",")",1]
        ],
        [
            ["-","-","_",1], ["=","=","+",1], [";",";",":",1], ["'","'","\"",1],
            [",",",","<",1], [".",".","!",1], ["/","/","?",1], ["[","[","{",1],
            ["]","]","}",1]
        ],
        [
            ["shift", KeyboardController.shiftActive ? "⇧" : "⇧", "", 1.5],
            ["`","`","~",1], ["\\","\\","|",1], ["space","  space  ","",3],
            ["backspace","⌫","",1.5]
        ],
        [
            ["abc","ABC","",1.5],
            ["enter","↵","",4]
        ]
    ]

    readonly property var currentRows: {
        if (KeyboardController.symbolMode)
            return rowsSymbol
        var gen = KeyboardController.generatedRows
        var letterRows
        if (gen.length >= 3) {
            // Generated letter rows (xkb layout of the active IM). They carry
            // no function keys, so shift and backspace bookend the bottom
            // letter row and the fixed function row closes the panel, exactly
            // like the hardcoded rowsQwerty structure.
            letterRows = [
                gen[0],
                gen[1],
                [rowsQwerty[2][0]].concat(gen[2], [rowsQwerty[2][rowsQwerty[2].length - 1]]),
                rowsQwerty[3]
            ]
        } else {
            letterRows = KeyboardController.layout === "keyboard-ru" ? rowsRussian : rowsQwerty
        }
        if (Settings.numbersRow)
            return [numbersRowData].concat(letterRows)
        return letterRows
    }

    // Generated-layout rows are positional (committed by physical key, not by
    // character). They start at row 0, or row 1 when the numbers row is on.
    readonly property bool generatedActive: !KeyboardController.symbolMode
                                         && KeyboardController.generatedRows.length >= 3
    readonly property int posRowStart: generatedActive && Settings.numbersRow ? 1 : 0

    // Suggestion bar: when visible the panel grows by barArea (main.qml pushes
    // desiredHeight to the window) and keyH is computed from the remaining
    // height, so the keys keep exactly the size they have without the bar.
    readonly property bool barVisible: Settings.showAutosuggestions
                                    && KeyboardController.suggestions.length > 0
    readonly property real barH: 36
    readonly property real barArea: barVisible ? barH + rowSpacing : 0
    // Height the window should have; the base matches KeyboardWindow's
    // initial 38%-of-screen sizing in setupLayerShell().
    readonly property int desiredHeight: Math.round(Screen.height * 0.38) + Math.round(barArea)

    // Key sizing: total panel height is fixed, keys shrink to fit the extra row
    readonly property real keyH: Math.round((root.height - barArea) / (currentRows.length + 0.6))
    readonly property real keySpacing: 4
    readonly property real rowSpacing: 4
    readonly property real sidePad: 6

    Column {
        id: panelColumn
        anchors.centerIn: parent
        width: root.width
        spacing: root.rowSpacing

        // Autosuggestion bar above the key rows
        Rectangle {
            id: suggestionBar
            visible: root.barVisible
            width: parent.width
            height: root.barH
            color: "transparent"

            Row {
                anchors.centerIn: parent
                spacing: 8

                Repeater {
                    model: KeyboardController.suggestions

                    Rectangle {
                        required property string modelData
                        required property int index

                        width: pillLabel.implicitWidth + 24
                        height: suggestionBar.height - 6
                        radius: height / 2
                        color: pillTap.pressed ? "#3daee9" : "#31363b"

                        Text {
                            id: pillLabel
                            anchors.centerIn: parent
                            text: parent.modelData
                            color: "#eff0f1"
                            font.family: "Noto Sans"
                            font.pixelSize: 16
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

        Column {
            id: keyColumn
            width: parent.width
            spacing: root.rowSpacing

            Repeater {
                model: root.currentRows

                // One row
                Row {
                    required property var modelData
                    required property int index

                    spacing: root.keySpacing
                    anchors.horizontalCenter: parent.horizontalCenter

                    // Compute total widthFactors for this row to derive unit width
                    readonly property real totalFactor: {
                        var sum = 0
                        for (var i = 0; i < modelData.length; i++)
                            sum += modelData[i][3]
                        return sum
                    }
                    readonly property real unitW: {
                        var totalSpacing = (modelData.length - 1) * root.keySpacing
                        return (root.width - 2 * root.sidePad - totalSpacing) / totalFactor
                    }
                    // Generated-layout letter rows commit by physical position.
                    readonly property bool positionalRow: root.generatedActive
                                                       && index >= root.posRowStart
                                                       && index <= root.posRowStart + 2

                    Repeater {
                        model: parent.modelData

                        Key {
                            required property var modelData

                            keyName:    modelData[0]
                            label:      modelData[1]
                            shiftLabel: modelData[2]
                            alternates: root.altFor(modelData[0], modelData[1])
                            corner:     root.cornerFor(modelData[0])
                            width:      Math.round(parent.unitW * modelData[3])
                            height:     root.keyH
                            positional: parent.positionalRow
                            isFuncKey:  modelData[0] === "shift"
                                     || modelData[0] === "backspace"
                                     || modelData[0] === "enter"
                                     || modelData[0] === "symbols"
                                     || modelData[0] === "abc"
                                     || modelData[0] === "lang"

                            // Highlight shift key when active
                            color: {
                                if (modelData[0] === "shift" && (KeyboardController.shiftActive || KeyboardController.capsLock))
                                    return "#3daee9"
                                return pressed ? "#3daee9"
                                     : isFuncKey ? "#2a2e32"
                                     : "#31363b"
                            }
                        }
                    }
                }
            }
        }
    }

    // Manual hide button (top-right corner)
    Rectangle {
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.margins: 4
        width: 32
        height: 20
        radius: 4
        color: hideArea.pressed ? "#3daee9" : "#1a1d20"
        opacity: 0.7

        Text {
            anchors.centerIn: parent
            text: "▼"
            color: "#eff0f1"
            font.pixelSize: 11
        }

        MouseArea {
            id: hideArea
            anchors.fill: parent
            onClicked: KeyboardController.hidePanel()
        }
    }
}
