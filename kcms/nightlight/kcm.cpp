/*
    SPDX-FileCopyrightText: 2017 Roman Gilg <subdiff@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "kcm.h"
#include "daynighttimings.h"

#include <QDBusConnection>
#include <QDBusMessage>
#include <QIcon>
#include <QStandardPaths>

#include <KLocalizedString>
#include <KPluginFactory>

#include "nightlightdata.h"

using namespace Qt::StringLiterals;

namespace ColorCorrect
{
K_PLUGIN_FACTORY_WITH_JSON(KCMNightLightFactory, "kcm_nightlight.json", registerPlugin<KCMNightLight>(); registerPlugin<NightLightData>();)

KCMNightLight::KCMNightLight(QObject *parent, const KPluginMetaData &data)
    : KQuickManagedConfigModule(parent, data)
    , m_data(new NightLightData(this))
{
    const auto uri = "org.kde.private.kcms.nightlight";
    qmlRegisterAnonymousType<NightLightSettings>(uri, 1);
    qmlRegisterType<DayNightTimings>(uri, 1, 0, "DayNightTimings");
    qmlRegisterUncreatableMetaObject(ColorCorrect::staticMetaObject, uri, 1, 0, "NightLightMode", QStringLiteral("Error: only enums"));

    minDayTemp = nightLightSettings()->findItem(u"DayTemperature"_s)->minValue().toInt();
    maxDayTemp = nightLightSettings()->findItem(u"DayTemperature"_s)->maxValue().toInt();
    minNightTemp = nightLightSettings()->findItem(u"NightTemperature"_s)->minValue().toInt();
    maxNightTemp = nightLightSettings()->findItem(u"NightTemperature"_s)->maxValue().toInt();

    setButtons(Apply | Default);
}

NightLightSettings *KCMNightLight::nightLightSettings() const
{
    return m_data->settings();
}

void KCMNightLight::preview(uint temperature)
{
    auto message = QDBusMessage::createMethodCall(u"org.kde.KWin.NightLight"_s, u"/org/kde/KWin/NightLight"_s, u"org.kde.KWin.NightLight"_s, u"preview"_s);
    message.setArguments({temperature});
    QDBusConnection::sessionBus().send(message);
}

void KCMNightLight::stopPreview()
{
    auto message = QDBusMessage::createMethodCall(u"org.kde.KWin.NightLight"_s, u"/org/kde/KWin/NightLight"_s, u"org.kde.KWin.NightLight"_s, u"stopPreview"_s);
    QDBusConnection::sessionBus().send(message);
}
}
#include "kcm.moc"

#include "moc_kcm.cpp"
