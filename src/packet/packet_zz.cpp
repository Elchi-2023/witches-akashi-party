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
        .min_args = 1,
        .header = "ZZ"};
    return info;
}

void PacketZZ::handlePacket(AreaData *area, AOClient &client) const
{
    QPointer<AreaData> CurrentArea = area;
    QPointer<Server> CurrentServer = client.getServer();

    QStringList PrintToOOC("──── MODCALL ───");
    if (CurrentServer.isNull() /* preventing segfault when calling <server> pointer.. */ || m_content[0].trimmed().isEmpty() /* preventing accepting <empty reasons> */)
        return;
    QStringList l_names, m_name({"[" + QString::number(client.clientId()) + "]", client.character().isEmpty() ? "Spectator" : client.character(), "[" + client.getIpid() + "]"});
    if (!client.name().isEmpty())
        m_name.insert(1, ": (" + client.name() + ")");
    PrintToOOC.append("├── Caller: " + m_name.join(' '));
    l_names.append(m_name.join(' '));
    PrintToOOC.append("├── Area: " + (CurrentArea.isNull() ? "[UNKNOWN]" : QString("[%1] %2").arg(QString::number(CurrentArea->index()), CurrentArea->name())));

    if (m_content.size() >= 2){ /* if the caller/client using <report button> feature */
        bool target_ok = false;
        const int target_id = m_content.at(1).toInt(&target_ok);
        if (target_ok && !CurrentServer->getClientByID(target_id).isNull()){ /* make sure both param are vaild.. */
            const auto t_client = CurrentServer->getClientByID(target_id);
            QPointer<AreaData> t_clientArea(CurrentServer->getAreaById(t_client->areaId()));

            QStringList t_name({"[" + QString::number(t_client->clientId()) + "]", t_client->character().isEmpty() ? "Spectator" : t_client->character(), "[" + t_client->getIpid() + "]"});
            if (!t_client->name().isEmpty())
                t_name.insert(1, ": (" + t_client->name() + ")");
            const QString t_areaname(t_clientArea.isNull() ? "[UNKNOWN]" : t_clientArea == CurrentArea ? "[THIS AREA]" : QString("[%1] %2").arg(QString::number(t_clientArea->index()), t_clientArea->name()));
            PrintToOOC.append(QString("├── Regarding: %1 in %2").arg(t_name.join(' '), t_areaname));
            l_names.append(" • Name: " + t_name.join(' ') + "\n • In: " + t_areaname);
        }
    }

    PrintToOOC.append("├── Reason: " + m_content[0]);
    PrintToOOC.append("└───────────────");

    for (auto l_client : CurrentServer->getClients()){
        if (!l_client.isNull() && l_client->m_authenticated)
            l_client->sendPacket(PacketFactory::createPacket("ZZ", {PrintToOOC.join('\n')}));
    }
    emit client.logModcall((client.character() + " " + client.characterName()), {client.clientId(), client.m_ipid}, client.name(), CurrentArea.isNull() ? "SERVER" : CurrentArea->name());
    if (ConfigManager::discordWebhookEnabled())
        emit client.getServer()->modcallWebhookRequest(l_names, CurrentArea.isNull() ? "<UNKNOWN>" : CurrentArea->name(), m_content[0], client.getServer()->getAreaBuffer(CurrentArea.isNull() ? "" : CurrentArea->name()));

}
