/*
    SPDX-FileCopyrightText: 2024 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QSortFilterProxyModel>
#include <qqmlregistration.h>

class HistoryModel;

/**
 * This class provides a view for history clip items in QML
 **/
class DeclarativeHistoryModel : public QSortFilterProxyModel
{
    Q_OBJECT
    QML_NAMED_ELEMENT(HistoryModel)

    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)
    Q_PROPERTY(int sourceCount READ sourceCount NOTIFY sourceCountChanged)
    Q_PROPERTY(bool hasStarredItems READ hasStarredItems NOTIFY hasStarredItemsChanged)
    Q_PROPERTY(QString currentText READ currentText NOTIFY currentTextChanged)

    Q_PROPERTY(bool starredOnly READ starredOnly WRITE setStarredOnly NOTIFY starredOnlyChanged)

public:
    explicit DeclarativeHistoryModel(QObject *parent = nullptr);
    ~DeclarativeHistoryModel() override;

    QString currentText() const;
    int sourceCount() const;
    bool hasStarredItems() const;

    bool starredOnly() const;
    void setStarredOnly(bool value);

    Q_INVOKABLE void moveToTop(const QString &uuid);

    Q_INVOKABLE void remove(const QString &uuid);
    Q_INVOKABLE void clearHistory();

    Q_INVOKABLE void invokeAction(const QString &uuid);

Q_SIGNALS:
    void countChanged();
    void sourceCountChanged();
    void hasStarredItemsChanged();
    void currentTextChanged();
    void starredOnlyChanged();

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

private:
    std::shared_ptr<HistoryModel> m_model;
    bool m_starredOnly = false;
};
