/*
    kcmformats.cpp
    SPDX-FileCopyrightText: 2014 Sebastian Kügler <sebas@kde.org>
    SPDX-FileCopyrightText: 2021 Han Young <hanyoung@protonmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <QCollator>

#include <KAboutData>
#include <KLocalizedString>
#include <KPluginFactory>
#include <KSharedConfig>

#include "exampleutility.cpp"
#include "formatssettings.h"
#include "kcmformats.h"
#include "localelistmodel.h"

K_PLUGIN_CLASS_WITH_JSON(KCMFormats, "metadata.json")

KCMFormats::KCMFormats(QObject *parent, const QVariantList &args)
    : KQuickAddons::ManagedConfigModule(parent, args)
    , m_settings(new FormatsSettings(this))
{
    KAboutData *aboutData = new KAboutData(QStringLiteral("kcm_formats"),
                                           i18nc("@title", "Formats"),
                                           QStringLiteral("0.1"),
                                           QLatin1String(""),
                                           KAboutLicense::LicenseKey::GPL_V2,
                                           i18nc("@info:credit", "Copyright Year Author"));

    aboutData->addAuthor(i18nc("@info:credit", "Author"), i18nc("@info:credit", "Author"), QStringLiteral("author@domain.com"));

    setAboutData(aboutData);
    setButtons(Help | Apply | Default);
    setQuickHelp(
        i18n("<h1>Formats</h1>"
             "You can configure the formats used for time, dates, money and other numbers here."));

    qmlRegisterAnonymousType<FormatsSettings>("kcmformats", 1);
    qmlRegisterType<LocaleListModel>("LocaleListModel", 1, 0, "LocaleListModel");
    connect(m_settings, &FormatsSettings::langChanged, this, &KCMFormats::handleLangChange);
    connect(m_settings, &FormatsSettings::numericChanged, this, &KCMFormats::numericExampleChanged);
    connect(m_settings, &FormatsSettings::timeChanged, this, &KCMFormats::timeExampleChanged);
    connect(m_settings, &FormatsSettings::monetaryChanged, this, &KCMFormats::monetaryExampleChanged);
    connect(m_settings, &FormatsSettings::measurementChanged, this, &KCMFormats::measurementExampleChanged);
    connect(m_settings, &FormatsSettings::collateChanged, this, &KCMFormats::collateExampleChanged);
}

void KCMFormats::handleLangChange()
{
    auto defaultVal = i18n("Default");
    if (m_settings->numeric() == defaultVal)
        Q_EMIT numericExampleChanged();
    if (m_settings->time() == defaultVal)
        Q_EMIT timeExampleChanged();
    if (m_settings->measurement() == defaultVal)
        Q_EMIT measurementExampleChanged();
    if (m_settings->monetary() == defaultVal)
        Q_EMIT monetaryExampleChanged();
    if (m_settings->collate() == defaultVal)
        Q_EMIT collateExampleChanged();
}

QString KCMFormats::numberExample() const
{
    return Utility::numericExample(QLocale(m_settings->numeric()));
}
QString KCMFormats::timeExample() const
{
    return Utility::timeExample(QLocale(m_settings->time()));
}
QString KCMFormats::currencyExample() const
{
    return Utility::monetaryExample(QLocale(m_settings->monetary()));
}
QString KCMFormats::measurementExample() const
{
    return Utility::measurementExample(QLocale(m_settings->measurement()));
}
QString KCMFormats::collateExample() const
{
    return Utility::collateExample(m_settings->collate());
}
FormatsSettings *KCMFormats::settings() const
{
    return m_settings;
}

QQuickItem *KCMFormats::getSubPage(int index) const
{
    return subPage(index);
}
#include "kcmformats.moc"
