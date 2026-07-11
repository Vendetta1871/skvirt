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

    readonly property var currentRows: KeyboardController.symbolMode
        ? rowsSymbol
        : (KeyboardController.layout === "keyboard-ru" ? rowsRussian : rowsQwerty)

    // Key sizing
    readonly property real keyH: Math.round(root.height / 4.6)
    readonly property real keySpacing: 4
    readonly property real rowSpacing: 4
    readonly property real sidePad: 6

    Column {
        id: keyColumn
        anchors.centerIn: parent
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

                Repeater {
                    model: parent.modelData

                    Key {
                        required property var modelData

                        keyName:    modelData[0]
                        label:      modelData[1]
                        shiftLabel: modelData[2]
                        width:      Math.round(parent.unitW * modelData[3])
                        height:     root.keyH
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
