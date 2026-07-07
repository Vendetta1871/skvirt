#include "KeyboardWindow.h"
#include <LayerShellQt/Window>
#include <QDebug>
#include <QScreen>
#include <QGuiApplication>

KeyboardWindow::KeyboardWindow(QWindow *parent)
    : QQuickWindow(parent)
{
    // Never take input focus: an on-screen keyboard must not become the
    // text-input target, or mapping it steals focus from the app being typed
    // into — fcitx then deactivates the input context and flip-flops the
    // keyboard show/hide. This flag stops Qt requesting activation on show;
    // it complements layer-shell KeyboardInteractivityNone/ActivateOnShow(false).
    setFlags(flags() | Qt::WindowDoesNotAcceptFocus);

    // Layer shell must be configured before the window is shown.
    // QQuickWindow constructor creates the native surface, so we can
    // call LayerShellQt::Window::get() immediately.
    setupLayerShell();
}

void KeyboardWindow::setupLayerShell()
{
    auto *lsw = LayerShellQt::Window::get(this);
    if (!lsw) {
        qWarning() << "skvirt: LayerShellQt not available — is the Wayland compositor running?";
        return;
    }

    // Full-width bar anchored to the bottom edge
    LayerShellQt::Window::Anchors anchors(LayerShellQt::Window::AnchorBottom);
    anchors |= LayerShellQt::Window::AnchorLeft;
    anchors |= LayerShellQt::Window::AnchorRight;
    lsw->setAnchors(anchors);

    // LayerTop: above normal windows, below system overlays (notifications, etc.)
    lsw->setLayer(LayerShellQt::Window::LayerTop);

    // CRITICAL: do not steal keyboard focus — the text input target must stay
    // in the application behind us so key events are committed there.
    lsw->setKeyboardInteractivity(LayerShellQt::Window::KeyboardInteractivityNone);

    // Reserve space at the bottom so maximised windows don't go under the keyboard.
    // Updated dynamically when InputPanel.active changes.
    lsw->setExclusiveZone(0);

    lsw->setScope("skvirt.keyboard");

    // Do not steal activation when shown
    lsw->setActivateOnShow(false);

    // Layer shell requires an explicit height when not anchored to both top and bottom.
    // Use ~38% of screen height as the initial size; QML will report exact height later.
    int screenH = 1080;
    if (auto *screen = QGuiApplication::primaryScreen())
        screenH = screen->size().height();
    int initH = qRound(screenH * 0.38);
    lsw->setDesiredSize(QSize(0, initH));  // 0 width = stretch to anchors
    setHeight(initH);
}

void KeyboardWindow::updateKeyboardHeight(int heightPx)
{
    if (heightPx <= 0)
        return;
    auto *lsw = LayerShellQt::Window::get(this);
    if (lsw)
        lsw->setDesiredSize(QSize(0, heightPx));
    setHeight(heightPx);
}

void KeyboardWindow::updateExclusiveZone(int heightPx)
{
    auto *lsw = LayerShellQt::Window::get(this);
    if (lsw)
        lsw->setExclusiveZone(heightPx);
}

void KeyboardWindow::showPanel()
{
    if (isVisible())
        return;
    setVisible(true);
    updateExclusiveZone(height());
}

void KeyboardWindow::hidePanel()
{
    updateExclusiveZone(0);
    setVisible(false);
}
