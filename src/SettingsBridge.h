#pragma once

#include <KConfigWatcher>

#include <QObject>

// QML singleton ("Settings" in module skvirt 1.0) exposing SkvirtSettings values.
// Watches the config file with KConfigWatcher so edits made from System Settings
// are applied live without restarting the app.
class SettingsBridge : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool showOnlyInTabletMode READ showOnlyInTabletMode NOTIFY showOnlyInTabletModeChanged)
    Q_PROPERTY(bool hideOnMouseMove READ hideOnMouseMove NOTIFY hideOnMouseMoveChanged)
    Q_PROPERTY(bool showAutosuggestions READ showAutosuggestions NOTIFY showAutosuggestionsChanged)
    Q_PROPERTY(bool longPressSymbols READ longPressSymbols NOTIFY longPressSymbolsChanged)
    Q_PROPERTY(bool numbersRow READ numbersRow NOTIFY numbersRowChanged)

public:
    explicit SettingsBridge(QObject *parent = nullptr);

    bool showOnlyInTabletMode() const;
    bool hideOnMouseMove() const;
    bool showAutosuggestions() const;
    bool longPressSymbols() const;
    bool numbersRow() const;

signals:
    void showOnlyInTabletModeChanged();
    void hideOnMouseMoveChanged();
    void showAutosuggestionsChanged();
    void longPressSymbolsChanged();
    void numbersRowChanged();

private:
    // Reload SkvirtSettings from disk and emit changed signals for entries
    // whose values actually differ from the snapshot taken before reload.
    void reload();

    KConfigWatcher::Ptr m_watcher;
};
