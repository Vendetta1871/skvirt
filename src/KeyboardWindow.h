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

    // Map/unmap the keyboard, following input-method activation.
    Q_INVOKABLE void showPanel();
    Q_INVOKABLE void hidePanel();

private:
    void setupLayerShell();
};
