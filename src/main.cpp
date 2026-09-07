#include <QApplication>
#include <QQmlApplicationEngine>
#include <QIcon>
#include <QSystemTrayIcon>

#include <KConfigWatcher>
#include <KSharedConfig>

#include "skvirt.h"  // generated SkvirtSettings (kconfig_add_kcfg_files)

#include "KeyboardWindow.h"
#include "KeyboardController.h"
#include "SettingsBridge.h"

int main(int argc, char *argv[])
{
    qputenv("QT_QPA_PLATFORM", "wayland");

    QApplication app(argc, argv);
    app.setApplicationName("skvirt");
    app.setOrganizationName("skvirt");
    // The tray icon must outlive the last window being hidden/closed.
    app.setQuitOnLastWindowClosed(false);

    qmlRegisterType<KeyboardWindow>("skvirt", 1, 0, "KeyboardWindow");

    // Created here (not by the QML engine) so the tray icon can reach it too.
    auto *controller = new KeyboardController(&app);
    qmlRegisterSingletonInstance("skvirt", 1, 0, "KeyboardController", controller);

    qmlRegisterSingletonType<SettingsBridge>(
        "skvirt", 1, 0, "Settings",
        [](QQmlEngine *, QJSEngine *) -> QObject * {
            return new SettingsBridge();
        });

    QQmlApplicationEngine engine;
    engine.load(QUrl(QStringLiteral("qrc:/skvirt/src/qml/main.qml")));

    if (engine.rootObjects().isEmpty())
        return 1;

    // Visibility toggle in the system tray, with the icon theme's stock
    // keyboard icon. Clicking it shows/hides the panel. The icon itself can
    // be turned off in the settings (showTrayIcon), applied live.
    QSystemTrayIcon tray(QIcon::fromTheme(QStringLiteral("input-keyboard")), &app);
    auto updateTooltip = [&tray, controller] {
        tray.setToolTip(controller->panelVisible()
            ? QStringLiteral("skvirt — click to hide the keyboard")
            : QStringLiteral("skvirt — click to show the keyboard"));
    };
    updateTooltip();
    QObject::connect(controller, &KeyboardController::panelVisibleChanged,
                     &tray, updateTooltip);
    QObject::connect(&tray, &QSystemTrayIcon::activated, controller,
                     [controller](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::Trigger
            || reason == QSystemTrayIcon::MiddleClick)
            controller->togglePanel();
    });

    auto applyTrayVisibility = [&tray] {
        SkvirtSettings::self()->load();
        tray.setVisible(SkvirtSettings::self()->showTrayIcon());
    };
    auto configWatcher = KConfigWatcher::create(KSharedConfig::openConfig(QStringLiteral("skvirtrc")));
    QObject::connect(configWatcher.data(), &KConfigWatcher::configChanged,
                     &tray, [applyTrayVisibility](const KConfigGroup &, const QByteArrayList &) {
        applyTrayVisibility();
    });
    applyTrayVisibility();

    return app.exec();
}
