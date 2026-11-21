#pragma once

#include <QQuickWindow>

class KeyboardWindow : public QQuickWindow
{
    Q_OBJECT
    QML_ELEMENT

public:
    explicit KeyboardWindow(QWindow *parent = nullptr);

    Q_INVOKABLE void updateExclusiveZone(int heightPx);
    Q_INVOKABLE void updateKeyboardHeight(int heightPx);

    // Map/unmap the keyboard. screenName is the output (QScreen::name(),
    // e.g. "eDP-1") to map it on; empty or unknown leaves the choice to the
    // compositor (its active output).
    Q_INVOKABLE void showPanel(const QString &screenName = {});
    Q_INVOKABLE void hidePanel();

private:
    void setupLayerShell();
    void placeOnScreen(const QString &screenName);
};
