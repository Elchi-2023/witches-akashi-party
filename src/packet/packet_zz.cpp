#include "packet/packet_zz.h"
#include "config_manager.h"
#include "packet/packet_factory.h"
#include "server.h"

#include <QQueue>

PacketZZ::PacketZZ(QStringList &contents) :
    AOPacket(contents)
{
}

PacketInfo PacketZZ::getPacketInfo() const
{
    PacketInfo info{
        .acl_permission = ACLRole::Permission::NONE,
        .min_args = 2,
        .header = "ZZ"};
    return info;
}

void PacketZZ::handlePacket(AreaData *area, AOClient &client) const
{
    QStringList l_name(QString("[%1] %2 (%3)").arg(QString::number(client.clientId()), client.name(), client.getIpid()));
    if (client.name().isEmpty())
        l_name.replace(0, QString("[%1] %2 (%3)").arg(QString::number(client.clientId()), client.character().isEmpty() ? "Spectator" : client.character(), client.getIpid()));

    const QString l_areaName = area->name();

    QStringList l_notification{"Area: " + l_areaName, "Caller: " + l_name.at(0)};

    const int target_id = m_content.at(1).toInt();
    if (target_id >= 0 && client.getServer()->getClientByID(target_id)){
        const AOClient *target = client.getServer()->getClientByID(target_id);
        if (target->character().isEmpty())
            l_name.append(QString("[%1] %2 (%3)").arg(QString::number(target->clientId()), target->character().isEmpty() ? "Spectator" : target->character(), target->getIpid()));
        else
            l_name.append(QString("[%1] %2 (%3)").arg(QString::number(target->clientId()), target->name(), target->getIpid()));
        l_notification.append("Regarding: " + l_name.last());
    }
    l_notification.append("Reason: " + m_content[0]);

    const QVector<AOClient *> l_clients = client.getServer()->getClients();
    for (AOClient *l_client : l_clients) {
        if (l_client->m_authenticated)
            l_client->sendPacket(PacketFactory::createPacket("ZZ", {"!!!MODCALL!!!\n" + l_notification.join('\n')}));
    }
    emit client.logModcall((client.character() + " " + client.characterName()), client.m_ipid, client.name(), client.getServer()->getAreaById(client.areaId())->name());

    if (ConfigManager::discordWebhookEnabled())
        emit client.getServer()->modcallWebhookRequest(l_name, l_areaName, l_notification.last(), client.getServer()->getAreaBuffer(l_areaName));
}
