/**
 * Copyright 2014 Pramod Dematagoda <pmdematagoda@mykolab.ch>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef SCREENSAVERINHIBITPLUGIN_H
#define SCREENSAVERINHIBITPLUGIN_H

#include <QObject>

#include <core/kdeconnectplugin.h>

class Q_DECL_EXPORT ScreensaverInhibitPlugin
    : public KdeConnectPlugin
{
    Q_OBJECT

public:
    explicit ScreensaverInhibitPlugin(QObject *parent, const QVariantList &args);
    virtual ~ScreensaverInhibitPlugin();

public Q_SLOTS:
    virtual bool receivePackage(const NetworkPackage& np);
    virtual void connected();

private:
    uint inhibitCookie;
};

#endif
