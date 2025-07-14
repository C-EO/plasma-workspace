/*
    SPDX-FileCopyrightText: 2020 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <Plasma/Applet>
#include <PlasmaQuick/AppletQuickItem>

class PanelSpacer : public Plasma::Applet
{
    Q_OBJECT
    Q_PROPERTY(PlasmaQuick::AppletQuickItem *containmentItem READ containmentGraphicObject CONSTANT)

public:
    PanelSpacer(QObject *parent, const KPluginMetaData &data, const QVariantList &args);

    PlasmaQuick::AppletQuickItem *containmentGraphicObject() const;
};
