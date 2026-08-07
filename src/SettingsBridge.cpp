#include "SettingsBridge.h"

#include <KConfigWatcher>
#include <KSharedConfig>

#include "skvirt.h"

SettingsBridge::SettingsBridge(QObject *parent)
    : QObject(parent)
{
    // Live-reload when the config file changes (e.g. edits from System Settings).
    m_watcher = KConfigWatcher::create(KSharedConfig::openConfig(QStringLiteral("skvirtrc")));
    connect(m_watcher.data(), &KConfigWatcher::configChanged, this, [this](const KConfigGroup &group, const QByteArrayList &names) {
        Q_UNUSED(group);
        Q_UNUSED(names);
        reload();
    });
}

bool SettingsBridge::showOnlyInTabletMode() const
{
    return SkvirtSettings::self()->showOnlyInTabletMode();
}

bool SettingsBridge::hideOnMouseMove() const
{
    return SkvirtSettings::self()->hideOnMouseMove();
}

bool SettingsBridge::showAutosuggestions() const
{
    return SkvirtSettings::self()->showAutosuggestions();
}

bool SettingsBridge::longPressSymbols() const
{
    return SkvirtSettings::self()->longPressSymbols();
}

bool SettingsBridge::numbersRow() const
{
    return SkvirtSettings::self()->numbersRow();
}

void SettingsBridge::reload()
{
    auto *settings = SkvirtSettings::self();

    const bool oldTabletMode = settings->showOnlyInTabletMode();
    const bool oldHideOnMouse = settings->hideOnMouseMove();
    const bool oldAutosuggestions = settings->showAutosuggestions();
    const bool oldLongPress = settings->longPressSymbols();
    const bool oldNumbersRow = settings->numbersRow();

    settings->load();

    if (settings->showOnlyInTabletMode() != oldTabletMode)
        emit showOnlyInTabletModeChanged();
    if (settings->hideOnMouseMove() != oldHideOnMouse)
        emit hideOnMouseMoveChanged();
    if (settings->showAutosuggestions() != oldAutosuggestions)
        emit showAutosuggestionsChanged();
    if (settings->longPressSymbols() != oldLongPress)
        emit longPressSymbolsChanged();
    if (settings->numbersRow() != oldNumbersRow)
        emit numbersRowChanged();
}
