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

#include "formatssettings.h"
#include "kcmformats.h"
#include "localelistmodel.h"
#include "optionsmodel.h"

K_PLUGIN_CLASS_WITH_JSON(KCMFormats, "metadata.json")

KCMFormats::KCMFormats(QObject *parent, const QVariantList &args)
    : KQuickAddons::ManagedConfigModule(parent, args)
    , m_optionsModel(new OptionsModel(this))
{
    KAboutData *aboutData = new KAboutData(QStringLiteral("kcm_formats"),
                                           i18nc("@title", "Formats"),
                                           QStringLiteral("0.1"),
                                           QLatin1String(""),
                                           KAboutLicense::LicenseKey::GPL_V2,
                                           i18nc("@info:credit", "Copyright 2021 Han Young"));

    aboutData->addAuthor(i18nc("@info:credit", "Han Young"), i18nc("@info:credit", "Author"), QStringLiteral("hanyoung@protonmail.com"));

    setAboutData(aboutData);
    setQuickHelp(
        i18n("<h1>Formats</h1>"
             "You can configure the formats used for time, dates, money and other numbers here."));

    qmlRegisterAnonymousType<FormatsSettings>("kcmformats", 1);
    qmlRegisterType<LocaleListModel>("LocaleListModel", 1, 0, "LocaleListModel");
    qmlRegisterAnonymousType<OptionsModel>("kcmformats_optionsmodel", 1);
}

FormatsSettings *KCMFormats::settings() const
{
    return m_optionsModel->settings();
}

OptionsModel *KCMFormats::optionsModel() const
{
    return m_optionsModel;
}
QQuickItem *KCMFormats::getSubPage(int index) const
{
    return subPage(index);
}
#include "kcmformats.moc"
