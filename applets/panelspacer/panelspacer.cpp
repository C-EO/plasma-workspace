/*
    SPDX-FileCopyrightText: 2020 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "panelspacer.h"

#include <QDebug>
#include <QProcess>

#include <Plasma/Containment>
#include <Plasma/Corona>

PanelSpacer::PanelSpacer(QObject *parent, const KPluginMetaData &data, const QVariantList &args)
    : Plasma::Applet(parent, data, args)
{
}

PlasmaQuick::AppletQuickItem *PanelSpacer::containmentGraphicObject() const
{
    if (!containment())
        return nullptr; // Return nothing if there is no containment to prevent a Segmentation Fault
    return PlasmaQuick::AppletQuickItem::itemForApplet(containment());
}

K_PLUGIN_CLASS_WITH_JSON(PanelSpacer, "metadata.json")

#include "panelspacer.moc"

#include "moc_panelspacer.cpp"
