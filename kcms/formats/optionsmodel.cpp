/*
    optionsmodel.cpp
    SPDX-FileCopyrightText: 2021 Han Young <hanyoung@protonmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include <KLocalizedString>

#include "exampleutility.cpp"
#include "formatssettings.h"
#include "optionsmodel.h"

OptionsModel::OptionsModel(QObject *parent)
    : QAbstractListModel(parent)
    , m_settings(new FormatsSettings(this))
{
    m_staticNames = {{{i18n("Region"), QStringLiteral("lang")},
                      {i18n("Number"), QStringLiteral("numeric")},
                      {i18n("Time"), QStringLiteral("time")},
                      {i18n("Currency"), QStringLiteral("currency")},
                      {i18n("Measurement"), QStringLiteral("measurement")},
                      {i18n("Collate and Sorting"), QStringLiteral("collate")}}};
    connect(m_settings, &FormatsSettings::langChanged, this, &OptionsModel::handleLangChange);
    connect(m_settings, &FormatsSettings::numericChanged, this, [this] {
        Q_EMIT dataChanged(createIndex(1, 0), createIndex(1, 0), {Subtitle, Example});
    });
    connect(m_settings, &FormatsSettings::timeChanged, this, [this] {
        Q_EMIT dataChanged(createIndex(2, 0), createIndex(2, 0), {Subtitle, Example});
    });
    connect(m_settings, &FormatsSettings::monetaryChanged, this, [this] {
        Q_EMIT dataChanged(createIndex(3, 0), createIndex(3, 0), {Subtitle, Example});
    });
    connect(m_settings, &FormatsSettings::measurementChanged, this, [this] {
        Q_EMIT dataChanged(createIndex(4, 0), createIndex(4, 0), {Subtitle, Example});
    });
    connect(m_settings, &FormatsSettings::collateChanged, this, [this] {
        Q_EMIT dataChanged(createIndex(5, 0), createIndex(5, 0), {Subtitle, Example});
    });
}
int OptionsModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return m_staticNames.size();
}
QVariant OptionsModel::data(const QModelIndex &index, int role) const
{
    int row = index.row();
    if (row < 0 || row >= (int)m_staticNames.size())
        return {};

    switch (role) {
    case Name:
        return m_staticNames[row].first;
    case Subtitle: {
        switch (row) {
        case 0:
            return m_settings->lang();
        case 1:
            return m_settings->numeric();
        case 2:
            return m_settings->time();
        case 3:
            return m_settings->monetary();
        case 4:
            return m_settings->measurement();
        case 5:
            return m_settings->collate();
        default:
            return {};
        }
    }
    case Example: {
        switch (row) {
        case 0:
            return QString();
        case 1:
            return numberExample();
        case 2:
            return timeExample();
        case 3:
            return currencyExample();
        case 4:
            return measurementExample();
        case 5:
            return collateExample();
        default:
            return {};
        }
    }
    case Page:
        return m_staticNames[row].second;
    default:
        return {};
    }
}

QHash<int, QByteArray> OptionsModel::roleNames() const
{
    return {{Name, "name"}, {Subtitle, "localeName"}, {Example, "example"}, {Page, "page"}};
}

void OptionsModel::handleLangChange()
{
    auto defaultVal = i18n("Default");
    if (m_settings->numeric() == defaultVal)
        Q_EMIT dataChanged(createIndex(1, 0), createIndex(1, 0), {Subtitle, Example});
    if (m_settings->time() == defaultVal)
        Q_EMIT dataChanged(createIndex(2, 0), createIndex(2, 0), {Subtitle, Example});
    if (m_settings->measurement() == defaultVal)
        Q_EMIT dataChanged(createIndex(3, 0), createIndex(3, 0), {Subtitle, Example});
    if (m_settings->monetary() == defaultVal)
        Q_EMIT dataChanged(createIndex(4, 0), createIndex(4, 0), {Subtitle, Example});
    if (m_settings->collate() == defaultVal)
        Q_EMIT dataChanged(createIndex(5, 0), createIndex(5, 0), {Subtitle, Example});
}

QString OptionsModel::numberExample() const
{
    return Utility::numericExample(QLocale(m_settings->numeric()));
}
QString OptionsModel::timeExample() const
{
    return Utility::timeExample(QLocale(m_settings->time()));
}
QString OptionsModel::currencyExample() const
{
    return Utility::monetaryExample(QLocale(m_settings->monetary()));
}
QString OptionsModel::measurementExample() const
{
    return Utility::measurementExample(QLocale(m_settings->measurement()));
}
QString OptionsModel::collateExample() const
{
    return Utility::collateExample(m_settings->collate());
}
FormatsSettings *OptionsModel::settings() const
{
    return m_settings;
}
