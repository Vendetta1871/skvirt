#include "kcm_skvirt.h"

#include <KPluginFactory>

KcmSkvirt::KcmSkvirt(QObject *parent, const KPluginMetaData &metaData)
    : KQuickManagedConfigModule(parent, metaData)
    , m_settings(SkvirtSettings::self())
{
    // The settings are a singleton (shared with the daemon), so they are not
    // a child of this module and must be registered manually for the
    // automatic saveNeeded/defaults tracking.
    registerSettings(m_settings);
    setButtons(Apply | Default);
}

K_PLUGIN_CLASS_WITH_JSON(KcmSkvirt, "kcm_skvirt.json")

#include "kcm_skvirt.moc"
