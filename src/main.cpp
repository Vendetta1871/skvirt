#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include "KeyboardWindow.h"
#include "KeyboardController.h"
#include "SettingsBridge.h"

int main(int argc, char *argv[])
{
    qputenv("QT_QPA_PLATFORM", "wayland");

    QGuiApplication app(argc, argv);
    app.setApplicationName("skvirt");
    app.setOrganizationName("skvirt");

    qmlRegisterType<KeyboardWindow>("skvirt", 1, 0, "KeyboardWindow");
    qmlRegisterSingletonType<KeyboardController>(
        "skvirt", 1, 0, "KeyboardController",
        [](QQmlEngine *, QJSEngine *) -> QObject * {
            return new KeyboardController();
        });

    qmlRegisterSingletonType<SettingsBridge>(
        "skvirt", 1, 0, "Settings",
        [](QQmlEngine *, QJSEngine *) -> QObject * {
            return new SettingsBridge();
        });

    QQmlApplicationEngine engine;
    engine.load(QUrl(QStringLiteral("qrc:/skvirt/src/qml/main.qml")));

    if (engine.rootObjects().isEmpty())
        return 1;

    return app.exec();
}
