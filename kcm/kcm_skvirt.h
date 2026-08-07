#pragma once

#include <KQuickManagedConfigModule>

#include "skvirt.h"

// System Settings module for the skvirt on-screen keyboard.
// Exposes the shared KConfigXT settings (SkvirtSettings) to QML as "kcm.settings".
class KcmSkvirt : public KQuickManagedConfigModule
{
    Q_OBJECT

    Q_PROPERTY(SkvirtSettings *settings READ settings CONSTANT)

public:
    explicit KcmSkvirt(QObject *parent, const KPluginMetaData &metaData);

    SkvirtSettings *settings() const
    {
        return m_settings;
    }

private:
    SkvirtSettings *const m_settings;
};
