/*
    SPDX-FileCopyrightText: 2007 Paolo Capriotti <p.capriotti@gmail.com>
    SPDX-FileCopyrightText: 2022 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QObject>
#include <QRunnable>
#include <QSize>

#include <KPackage/Package>

/**
 * The WallpaperPackage type represents a wallpaper with the KPackage format. The wallpaper can
 * be a single image or a pair of dark and light images.
 *
 * The selectors property specifies additional supported modes:
 * * dark-light: the currently displayed image follows the color scheme
 * * day-night: the currently displayed image follows time of day. For example, the dark image
 *      is displayed at night, the light image is displayed at day
 *
 * With the day-night mode, plasma will slowly switch between wallpaper images at sunrise and
 * sunset. If this is undesired, the wallpaper can specify `X-KDE-CrossFade: false` in its metadata.
 */
class WallpaperPackage
{
public:
    WallpaperPackage(const KPackage::Package &package);

    QString displayName() const;
    KPackage::Package package() const;
    QStringList selectors() const;

private:
    KPackage::Package m_package;
    QStringList m_selectors;
};

/**
 * A runnable that finds KPackage wallpapers.
 */
class PackageFinder : public QObject, public QRunnable
{
    Q_OBJECT

public:
    PackageFinder(const QStringList &paths, const QSize &targetSize, QObject *parent = nullptr);

    void run() override;

    static void findPreferredImageInPackage(KPackage::Package &package, const QSize &targetSize);

Q_SIGNALS:
    void packageFound(const QList<WallpaperPackage> &packages);

private:
    QStringList m_paths;
    QSize m_targetSize;
};
