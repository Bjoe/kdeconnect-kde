/*
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

#include "downloadjob.h"

DownloadJob::DownloadJob(QHostAddress address, qint16 port, QObject* parent)
    : QObject(parent)
    , mAddress(address)
    , mPort(port)
    , mSocket(QSharedPointer<QTcpSocket>(new QTcpSocket))
{
    connect(mSocket.data(), &QTcpSocket::disconnected, this, &DownloadJob::disconnected);
}

void DownloadJob::start()
{
    //kDebug(kdeconnect_kded()) << "DownloadJob Start";
    mSocket->connectToHost(mAddress, mPort, QIODevice::ReadOnly);
    //TODO: Implement payload encryption somehow (create an intermediate iodevice to encrypt the payload here?)
}

void DownloadJob::disconnected()
{
    //kDebug(kdeconnect_kded()) << "DownloadJob End";
    deleteLater();
}

DownloadJob *DownloadJob::createInstance(QHostAddress address, QVariantMap transferInfo, QObject* parent)
{
    qint16 port = transferInfo["port"].toInt();
    return new DownloadJob(address, port, parent);
}

QSharedPointer<QIODevice> DownloadJob::getPayload()
{
    //kDebug(kdeconnect_kded()) << "getPayload";
    return mSocket.staticCast<QIODevice>();
}
