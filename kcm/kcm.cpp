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

#include "kcm.h"

#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QAction>
#include <QStackedLayout>
#include <QListView>
#include <QDBusConnection>
#include <QDBusInterface>

#include <KServiceTypeTrader>
#include <KPluginInfo>
#include <KPluginMetaData>
#include <KPluginFactory>
#include <KAboutData>
#include <KLocalizedString>

#include "ui_kcm.h"
#include "interfaces/dbusinterfaces.h"
#include "interfaces/devicesmodel.h"
#include "devicessortproxymodel.h"

K_PLUGIN_FACTORY(KdeConnectKcmFactory, registerPlugin<KdeConnectKcm>();)

KdeConnectKcm::KdeConnectKcm(QWidget *parent, const QVariantList&)
    : KCModule(KAboutData::pluginData("kdeconnect-kcm"), parent)
    , kcmUi(new Ui::KdeConnectKcmUi())
    , daemon(new DaemonDbusInterface(this))
    , devicesModel(new DevicesModel(this))
    , currentDevice(0)
{
    kcmUi->setupUi(this);


    kcmUi->deviceList->setIconSize(QSize(32,32));

    sortProxyModel = new DevicesSortProxyModel(devicesModel);

    kcmUi->deviceList->setModel(sortProxyModel);

    kcmUi->deviceInfo->setVisible(false);
    kcmUi->progressBar->setVisible(false);
    kcmUi->messages->setVisible(false);

    //Workaround: If we set this directly (or if we set it in the .ui file), the layout breaks
    kcmUi->noDeviceLinks->setWordWrap(false);
    QTimer::singleShot(0, [this] { kcmUi->noDeviceLinks->setWordWrap(true); });

    kcmUi->rename_label->setText(daemon->announcedName());
    kcmUi->rename_edit->setText(daemon->announcedName());
    setRenameMode(false);

    setButtons(KCModule::NoAdditionalButton);

    connect(devicesModel, SIGNAL(dataChanged(QModelIndex,QModelIndex)),
            this, SLOT(resetSelection()));
    connect(kcmUi->deviceList->selectionModel(), SIGNAL(currentChanged(QModelIndex,QModelIndex)),
            this, SLOT(deviceSelected(QModelIndex)));
    connect(kcmUi->pair_button, SIGNAL(clicked()),
            this, SLOT(requestPair()));
    connect(kcmUi->unpair_button, SIGNAL(clicked()),
            this, SLOT(unpair()));
    connect(kcmUi->ping_button, SIGNAL(clicked()),
            this, SLOT(sendPing()));
    connect(kcmUi->refresh_button,SIGNAL(clicked()),
            this, SLOT(refresh()));
    connect(kcmUi->rename_edit,SIGNAL(returnPressed()),
            this, SLOT(renameDone()));
    connect(kcmUi->renameDone_button,SIGNAL(clicked()),
            this, SLOT(renameDone()));
    connect(kcmUi->renameShow_button,SIGNAL(clicked()),
            this, SLOT(renameShow()));

}

void KdeConnectKcm::renameShow()
{
    setRenameMode(true);
}

void KdeConnectKcm::renameDone()
{
    QString newName = kcmUi->rename_edit->text();
    if (newName.isEmpty()) {
        //Rollback changes
        kcmUi->rename_edit->setText(kcmUi->rename_label->text());
    } else {
        kcmUi->rename_label->setText(newName);
        daemon->setAnnouncedName(newName);
    }
    setRenameMode(false);
}

void KdeConnectKcm::setRenameMode(bool b) {
    kcmUi->renameDone_button->setVisible(b);
    kcmUi->rename_edit->setVisible(b);
    kcmUi->renameShow_button->setVisible(!b);
    kcmUi->rename_label->setVisible(!b);
}

KdeConnectKcm::~KdeConnectKcm()
{
    delete kcmUi;
}

void KdeConnectKcm::refresh()
{
    daemon->forceOnNetworkChange();
}

void KdeConnectKcm::resetSelection()
{
    kcmUi->deviceList->selectionModel()->setCurrentIndex(sortProxyModel->mapFromSource(currentIndex), QItemSelectionModel::ClearAndSelect);
}

void KdeConnectKcm::deviceSelected(const QModelIndex& current)
{

    kcmUi->noDevicePlaceholder->setVisible(false);

    if (currentDevice) {
        disconnect(currentDevice,SIGNAL(pairingSuccesful()),
            this, SLOT(pairingSuccesful()));
        disconnect(currentDevice,SIGNAL(pairingFailed(QString)),
            this, SLOT(pairingFailed(QString)));
        disconnect(currentDevice,SIGNAL(unpaired()),
            this, SLOT(unpaired()));
    }

    //Store previous device config
    pluginsConfigChanged();

    if (!current.isValid()) {
        kcmUi->deviceInfo->setVisible(false);
        return;
    }

    currentIndex = sortProxyModel->mapToSource(current);
    currentDevice = devicesModel->getDevice(currentIndex.row());

    bool valid = (currentDevice != NULL && currentDevice->isValid());
    kcmUi->deviceInfo->setVisible(valid);
    if (!valid) {
        return;
    }

    kcmUi->messages->setVisible(false);
    if (currentDevice->pairRequested()) {
        kcmUi->progressBar->setVisible(true);
        kcmUi->unpair_button->setVisible(false);
        kcmUi->pair_button->setVisible(false);
        kcmUi->ping_button->setVisible(false);
    } else {
        kcmUi->progressBar->setVisible(false);
        if (currentDevice->isPaired()) {
            kcmUi->unpair_button->setVisible(true);
            kcmUi->pair_button->setVisible(false);
            kcmUi->ping_button->setVisible(true);
        } else {
            kcmUi->unpair_button->setVisible(false);
            kcmUi->pair_button->setVisible(true);
            kcmUi->ping_button->setVisible(false);
        }
    }

    //KPluginSelector has no way to remove a list of plugins and load another, so we need to destroy and recreate it each time
    delete kcmUi->pluginSelector;
    kcmUi->pluginSelector = new KPluginSelector(this);
    kcmUi->deviceInfo_layout->addWidget(kcmUi->pluginSelector);

    kcmUi->pluginSelector->setConfigurationArguments(QStringList(currentDevice->id()));

    kcmUi->name_label->setText(currentDevice->name());
    kcmUi->status_label->setText(currentDevice->isPaired()? i18n("(paired)") : i18n("(unpaired)"));

    connect(currentDevice,SIGNAL(pairingSuccesful()),
            this, SLOT(pairingSuccesful()));
    connect(currentDevice,SIGNAL(pairingFailed(QString)),
            this, SLOT(pairingFailed(QString)));
    connect(currentDevice,SIGNAL(unpaired()),
            this, SLOT(unpaired()));

    const QList<KPluginInfo> pluginInfo = KPluginInfo::fromMetaData(KPluginLoader::findPlugins("kdeconnect/"));
    KSharedConfigPtr deviceConfig = KSharedConfig::openConfig(currentDevice->pluginsConfigFile());
    kcmUi->pluginSelector->addPlugins(pluginInfo, KPluginSelector::ReadConfigFile, i18n("Plugins"), QString(), deviceConfig);

    connect(kcmUi->pluginSelector, SIGNAL(changed(bool)),
            this, SLOT(pluginsConfigChanged()));

}

void KdeConnectKcm::requestPair()
{
    if (!currentDevice) {
        return;
    }

    kcmUi->messages->hide();

    kcmUi->pair_button->setVisible(false);
    kcmUi->progressBar->setVisible(true);

    currentDevice->requestPair();

}

void KdeConnectKcm::unpair()
{
    if (!currentDevice) {
        return;
    }

    currentDevice->unpair();
}

void KdeConnectKcm::unpaired()
{
    DeviceDbusInterface* senderDevice = (DeviceDbusInterface*) sender();
    devicesModel->deviceStatusChanged(senderDevice->id());

    if (senderDevice != currentDevice) return;

    kcmUi->pair_button->setVisible(true);
    kcmUi->unpair_button->setVisible(false);
    kcmUi->progressBar->setVisible(false);
    kcmUi->ping_button->setVisible(false);
    kcmUi->status_label->setText(i18n("(unpaired)"));
}

void KdeConnectKcm::pairingFailed(const QString& error)
{
    if (sender() != currentDevice) return;

    kcmUi->pair_button->setVisible(true);
    kcmUi->unpair_button->setVisible(false);
    kcmUi->progressBar->setVisible(false);
    kcmUi->ping_button->setVisible(false);
    kcmUi->status_label->setText(i18n("(unpaired)"));

    kcmUi->messages->setText(i18n("Error trying to pair: %1",error));
    kcmUi->messages->animatedShow();
}

void KdeConnectKcm::pairingSuccesful()
{
    DeviceDbusInterface* senderDevice = (DeviceDbusInterface*) sender();
    devicesModel->deviceStatusChanged(senderDevice->id());

    if (senderDevice != currentDevice) return;

    kcmUi->pair_button->setVisible(false);
    kcmUi->unpair_button->setVisible(true);
    kcmUi->progressBar->setVisible(false);
    kcmUi->ping_button->setVisible(true);
    kcmUi->status_label->setText(i18n("(paired)"));
}

void KdeConnectKcm::pluginsConfigChanged()
{
    //Store previous selection
    if (!currentDevice) return;

    DeviceDbusInterface* auxCurrentDevice = currentDevice;
    currentDevice = 0; //HACK to avoid infinite recursion (for some reason calling save on pluginselector emits changed)
    kcmUi->pluginSelector->save();
    currentDevice = auxCurrentDevice;

    currentDevice->reloadPlugins();
}

void KdeConnectKcm::save()
{
    pluginsConfigChanged();
    KCModule::save();
}

void KdeConnectKcm::sendPing()
{
    if (!currentDevice) return;
    QDBusMessage msg = QDBusMessage::createMethodCall("org.kde.kdeconnect", "/modules/kdeconnect/devices/"+currentDevice->id()+"/ping", "org.kde.kdeconnect.device.ping", "sendPing");
    QDBusConnection::sessionBus().call(msg);
}

QSize KdeConnectKcm::sizeHint() const
{
    return QSize(890,550); //Golden ratio :D
}

QSize KdeConnectKcm::minimumSizeHint() const
{
    return QSize(500,300);
}

#include "kcm.moc"
#include "moc_kcm.cpp"
