import QtQuick
import skvirt 1.0

KeyboardWindow {
    id: root
    visible: false
    color: "transparent"

    SkvirtKeyboard {
        anchors.fill: parent
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
