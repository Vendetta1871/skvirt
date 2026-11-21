import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

Kirigami.FormLayout {
    id: root

    QQC2.CheckBox {
        id: autoShowOnFocusCheck
        Kirigami.FormData.label: i18n("Behaviour:")
        text: i18n("Show automatically (on text field activation)")
        checked: kcm.settings.autoShowOnFocus
        onToggled: kcm.settings.autoShowOnFocus = checked
    }

    QQC2.Label {
        text: i18n("Mouse clicks and Tab count too. Off: only a finger or pen touch shows it.")
        font: Kirigami.Theme.smallFont
        opacity: 0.7
    }

    QQC2.CheckBox {
        id: tabletModeCheck
        text: i18n("Show only in tablet mode")
        checked: kcm.settings.showOnlyInTabletMode
        onToggled: kcm.settings.showOnlyInTabletMode = checked
    }

    QQC2.CheckBox {
        id: hideOnMouseMoveCheck
        text: i18n("Hide when mouse is moved")
        // With auto-show on focus the panel follows the field instead.
        enabled: !autoShowOnFocusCheck.checked
        checked: kcm.settings.hideOnMouseMove
        onToggled: kcm.settings.hideOnMouseMove = checked
    }

    QQC2.CheckBox {
        id: trayIconCheck
        text: i18n("Show an icon in the system tray")
        checked: kcm.settings.showTrayIcon
        onToggled: kcm.settings.showTrayIcon = checked
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
        id: functionRowCheck
        text: i18n("Function key row (esc, F1–F12)")
        checked: kcm.settings.functionRow
        onToggled: kcm.settings.functionRow = checked
    }

    QQC2.Label {
        // Without the row, its keys stay reachable by holding the key below
        // them — a hint worth spelling out, since nothing on screen shows it.
        text: i18n("Without the row, hold ` or a digit to reach esc and F1–F12.")
        font: Kirigami.Theme.smallFont
        opacity: 0.7
        enabled: !functionRowCheck.checked && longPressCheck.checked
    }

    QQC2.CheckBox {
        id: voiceInputCheck
        Kirigami.FormData.label: i18n("Voice input:")
        text: i18n("Tap Space, then press-and-hold it to dictate")
        checked: kcm.settings.voiceInput
        onToggled: kcm.settings.voiceInput = checked
    }

    QQC2.TextField {
        id: whisperUrlField
        Kirigami.FormData.label: i18n("whisper-server:")
        enabled: voiceInputCheck.checked
        text: kcm.settings.whisperServerUrl
        onEditingFinished: kcm.settings.whisperServerUrl = text
        Layout.fillWidth: true
    }

    QQC2.TextField {
        id: whisperLanguageField
        Kirigami.FormData.label: i18n("Speech language:")
        enabled: voiceInputCheck.checked
        text: kcm.settings.whisperLanguage
        onEditingFinished: kcm.settings.whisperLanguage = text
        Layout.fillWidth: true
    }

    QQC2.Label {
        text: i18n("Requires whisper.cpp's whisper-server (default port 8080). \"auto\" detects the language, a code like \"ru\" pins it; empty uses the server's own -l (\"en\" by default).")
        font: Kirigami.Theme.smallFont
        opacity: 0.7
        enabled: voiceInputCheck.checked
    }

    QQC2.ComboBox {
        id: themeCombo
        Kirigami.FormData.label: i18n("Appearance:")
        // Indices match the Theme enum in skvirt.kcfg (System, Light, Dark).
        model: [i18n("Follow system colour scheme"), i18n("Light"), i18n("Dark")]
        currentIndex: kcm.settings.theme
        onActivated: kcm.settings.theme = currentIndex
    }
}
