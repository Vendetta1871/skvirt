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
        // or disappears so the bar never covers the key rows. The exclusive
        // zone excludes the transient long-press popup reserve so apps are
        // not pushed around by a popup.
        onDesiredHeightChanged: {
            root.updateKeyboardHeight(desiredHeight)
            if (root.visible)
                root.updateExclusiveZone(zoneHeight)
        }

        // The panel is exactly as tall as the keyboard it draws, which the
        // layer-shell window can only learn from QML: push the first value
        // too, in case it never changes afterwards.
        Component.onCompleted: root.updateKeyboardHeight(desiredHeight)
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
