import QtQuick
import skvirt 1.0

KeyboardWindow {
    id: root
    visible: false
    color: "transparent"

    SkvirtKeyboard {
        id: keyboard
        anchors.fill: parent

        // Grow/shrink the layer-shell window when the suggestion bar appears
        // or disappears so the bar never covers the key rows.
        onDesiredHeightChanged: {
            root.updateKeyboardHeight(desiredHeight)
            if (root.visible)
                root.updateExclusiveZone(desiredHeight)
        }
    }

    // Visibility is driven by fcitx5 (Show/HideVirtualKeyboard) via the controller.
    Connections {
        target: KeyboardController
        function onPanelVisibleChanged() {
            if (KeyboardController.panelVisible)
                root.showPanel()
            else
                root.hidePanel()
        }
    }
}
