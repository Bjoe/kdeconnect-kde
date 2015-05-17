/**
 * Copyright 2013 Albert Vaca <albertvaka@gmail.com>
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

#include "daemon.h"

//#include <QDBusConnection>
#include <QNetworkSession>
#include <QNetworkConfigurationManager>
#include <QNetworkAccessManager>
#include <QDebug>
#include <QPointer>

#include "core_debug.h"
#include "kdeconnectconfig.h"
#include "networkpackage.h"
#include "backends/lan/lanlinkprovider.h"
#include "backends/loopback/loopbacklinkprovider.h"
#include "device.h"
#include "networkpackage.h"
#include "backends/devicelink.h"
#include "backends/linkprovider.h"

Q_GLOBAL_STATIC(Daemon*, s_instance)

struct DaemonPrivate
{
    //Different ways to find devices and connect to them
    QSet<LinkProvider*> mLinkProviders;

    //Every known device
    QMap<QString, Device*> mDevices;

};

Daemon* Daemon::instance()
{
    Q_ASSERT(s_instance.exists());
    return *s_instance;
}

Daemon::Daemon(QObject *parent)
    : QObject(parent)
    , d(new DaemonPrivate)
{
    Q_ASSERT(!s_instance.exists());
    *s_instance = this;
    qCDebug(KDECONNECT_CORE) << "KdeConnect daemon starting";

    //Load backends
    d->mLinkProviders.insert(new LanLinkProvider());
    //d->mLinkProviders.insert(new LoopbackLinkProvider());

    //Read remebered paired devices
    const QStringList& list = KdeConnectConfig::instance()->trustedDevices();
    Q_FOREACH(const QString& id, list) {
        Device* device = new Device(this, id);
        connect(device, SIGNAL(reachableStatusChanged()),
                this, SLOT(onDeviceReachableStatusChanged()));
        d->mDevices[id] = device;
        Q_EMIT deviceAdded(id);
    }

    //Listen to new devices
    Q_FOREACH (LinkProvider* a, d->mLinkProviders) {
        connect(a, SIGNAL(onConnectionReceived(NetworkPackage, DeviceLink*)),
                this, SLOT(onNewDeviceLink(NetworkPackage, DeviceLink*)));
    }
    setDiscoveryEnabled(true);

    //Detect when a network interface changes status (TODO: Move to the lan link provider?)
    QNetworkConfigurationManager* networkManager;
    networkManager = new QNetworkConfigurationManager(this);
    connect(networkManager, &QNetworkConfigurationManager::configurationChanged, [this, networkManager](QNetworkConfiguration config) {
        qCDebug(KDECONNECT_CORE()) << config.name() << " state changed to " << config.state();
        qCDebug(KDECONNECT_CORE()) << "Online status: " << (networkManager->isOnline()? "online":"offline");
        forceOnNetworkChange();
    });

    //Register on DBus
    // TODO QDBusConnection QDBusConnection::sessionBus().registerService("org.kde.kdeconnect");
    // TODO QDBusConnection QDBusConnection::sessionBus().registerObject("/modules/kdeconnect", this, QDBusConnection::ExportScriptableContents);

    qCDebug(KDECONNECT_CORE) << "KdeConnect daemon started";
}

void Daemon::setDiscoveryEnabled(bool b)
{
    Q_FOREACH (LinkProvider* a, d->mLinkProviders) {
        if (b)
            a->onStart();
        else
            a->onStop();
    }
}

void Daemon::forceOnNetworkChange()
{
    qCDebug(KDECONNECT_CORE) << "Sending onNetworkChange to " << d->mLinkProviders.size() << " LinkProviders";
    Q_FOREACH (LinkProvider* a, d->mLinkProviders) {
        a->onNetworkChange();
    }
}

QStringList Daemon::devices(bool onlyReachable, bool onlyVisible) const
{
    QStringList ret;
    Q_FOREACH(Device* device, d->mDevices) {
        if (onlyReachable && !device->isReachable()) continue;
        if (onlyVisible && !device->isPaired()) continue;
        ret.append(device->id());
    }
    return ret;
}

void Daemon::onNewDeviceLink(const NetworkPackage& identityPackage, DeviceLink* dl)
{
    const QString& id = identityPackage.get<QString>("deviceId");

    //qCDebug(KDECONNECT_CORE) << "Device discovered" << id << "via" << dl->provider()->name();

    if (d->mDevices.contains(id)) {
        //qCDebug(KDECONNECT_CORE) << "It is a known device";
        Device* device = d->mDevices[id];
        device->addLink(identityPackage, dl);
    } else {
        //qCDebug(KDECONNECT_CORE) << "It is a new device";

        Device* device = new Device(this, identityPackage, dl);
        connect(device, SIGNAL(reachableStatusChanged()), this, SLOT(onDeviceReachableStatusChanged()));
        d->mDevices[id] = device;

        Q_EMIT deviceAdded(id);
    }

    Q_EMIT deviceVisibilityChanged(id, true);
}

void Daemon::onDeviceReachableStatusChanged()
{
    Device* device = (Device*)sender();
    QString id = device->id();

    Q_EMIT deviceVisibilityChanged(id, device->isReachable());

    //qCDebug(KDECONNECT_CORE) << "Device" << device->name() << "reachable status changed:" << device->isReachable();

    if (!device->isReachable()) {

        if (!device->isPaired()) {

            qCDebug(KDECONNECT_CORE) << "Destroying device" << device->name();
            Q_EMIT deviceRemoved(id);
            d->mDevices.remove(id);
            device->deleteLater();
        }

    }
}

void Daemon::setAnnouncedName(QString name)
{
    qCDebug(KDECONNECT_CORE()) << "Announcing name";
    KdeConnectConfig::instance()->setName(name);
    forceOnNetworkChange();
}

QString Daemon::announcedName()
{
    return KdeConnectConfig::instance()->name();
}

QNetworkAccessManager* Daemon::networkAccessManager()
{
    static QPointer<QNetworkAccessManager> manager;
    if (!manager) {
        manager = new QNetworkAccessManager(this);
    }
    return manager;
}

Daemon::~Daemon()
{

}

