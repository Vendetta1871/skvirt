import QtQuick
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami

Kirigami.FormLayout {
    id: root

    QQC2.CheckBox {
        id: tabletModeCheck
        Kirigami.FormData.label: i18n("Behaviour:")
        text: i18n("Show only in tablet mode")
        checked: kcm.settings.showOnlyInTabletMode
        onToggled: kcm.settings.showOnlyInTabletMode = checked
    }

    QQC2.CheckBox {
        id: hideOnMouseMoveCheck
        text: i18n("Hide when mouse is moved")
        checked: kcm.settings.hideOnMouseMove
        onToggled: kcm.settings.hideOnMouseMove = checked
    }

    QQC2.CheckBox {
        id: autosuggestionsCheck
        text: i18n("Show autosuggestions")
        checked: kcm.settings.showAutosuggestions
        onToggled: kcm.settings.showAutosuggestions = checked
    }

    QQC2.CheckBox {
        id: longPressCheck
        Kirigami.FormData.label: i18n("Keys:")
        text: i18n("Show symbols on long press")
        checked: kcm.settings.longPressSymbols
        onToggled: kcm.settings.longPressSymbols = checked
    }

    QQC2.CheckBox {
        id: numbersRowCheck
        text: i18n("Numbers row")
        checked: kcm.settings.numbersRow
        onToggled: kcm.settings.numbersRow = checked
    }
}
