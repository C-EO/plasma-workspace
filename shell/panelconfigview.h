/*
    SPDX-FileCopyrightText: 2013 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <Plasma/Theme>
#include <PlasmaQuick/PopupPlasmaWindow>
#include <PlasmaQuick/SharedQmlEngine>

#include "panelview.h"

#include <QJSValue>
#include <QPointer>
#include <QQmlListProperty>
#include <QQuickItem>
#include <QQuickView>
#include <QStandardItemModel>
#include <plasmaquick/plasmawindow.h>
#include <qevent.h>

class PanelView;
class PanelConfigView;

namespace Plasma
{
class Containment;
}
class PanelRulerView : public PlasmaQuick::PlasmaWindow
{
    Q_OBJECT

public:
    PanelRulerView(Plasma::Containment *interface, PanelView *panelView, PanelConfigView *mainConfigView);
    ~PanelRulerView() override;

    void syncPanelLocation();

protected:
    void showEvent(QShowEvent *ev) override;
    void focusOutEvent(QFocusEvent *ev) override;

private:
    Plasma::Containment *const m_containment;
    PanelView *const m_panelView;
    PanelConfigView *const m_mainConfigView;
    LayerShellQt::Window *m_layerWindow = nullptr;
};

class PanelConfigView : public PlasmaQuick::PopupPlasmaWindow
{
    Q_OBJECT
    Q_PROPERTY(PanelRulerView *panelRulerView READ panelRulerView CONSTANT)
    Q_PROPERTY(QRect geometry READ geometry NOTIFY geometryChanged)

public:
    PanelConfigView(Plasma::Containment *interface, PanelView *panelView);
    ~PanelConfigView() override;

    void init();

    PanelRulerView *panelRulerView();
    QRect geometry() const;

    Q_INVOKABLE QScreen *screenFromWindow(QWindow *window) const;

    friend class PanelRulerView;

protected:
    void keyPressEvent(QKeyEvent *ev) override;
    void showEvent(QShowEvent *ev) override;
    void hideEvent(QHideEvent *ev) override;
    void focusInEvent(QFocusEvent *ev) override;
    void moveEvent(QMoveEvent *ev) override;
    void resizeEvent(QResizeEvent *ev) override;

public Q_SLOTS:
    void showAddWidgetDialog();
    void addPanelSpacer();

protected Q_SLOTS:
    void syncGeometry();

Q_SIGNALS:
    void geometryChanged(const QRect &geometry);

private:
    void focusVisibilityCheck(QWindow *focusWindow);

    Plasma::Containment *m_containment;
    QPointer<PanelView> m_panelView;
    QPointer<QWindow> m_focusWindow;
    std::unique_ptr<PanelRulerView> m_panelRulerView;
    KSvg::FrameSvg::EnabledBorders m_enabledBorders = KSvg::FrameSvg::AllBorders;
    Plasma::Theme m_theme;
    QTimer m_screenSyncTimer;
    std::unique_ptr<PlasmaQuick::SharedQmlEngine> m_sharedQmlEngine;
};
