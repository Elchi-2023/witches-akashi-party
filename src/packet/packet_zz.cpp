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

    if (CurrentArea.isNull() /* preventing segfault when calling <areadata> pointer.. */ || CurrentServer.isNull() /* preventing segfault when calling <server> pointer.. */ || m_content[0].trimmed().isEmpty() /* preventing accepting <empty reasons> */)
        return;
    else if (!client.m_joined)
        client.m_socket->close(QWebSocketProtocol::CloseCode::CloseCodeProtocolError);
    else{
        QStringList PrintToOOC("──── MODCALL ───");
        QPair<QString, QString> l_name; // name & area..
        QStringList m_name({"[" + QString::number(client.clientId()) + "]", client.character().isEmpty() ? "[Spectator]" : client.character(), "[" + client.getIpid() + "]"});
        if (!client.name().isEmpty())
            m_name.insert(1, ": (" + client.name() + ")");
        PrintToOOC.append("├── Caller: " + m_name.join(' '));
        l_name.first = m_name.join(' ');
        PrintToOOC.append("├── Area: " + QString("[%1] %2").arg(QString::number(CurrentArea->index()), CurrentArea->name()));
        l_name.second = QString("[%1] %2").arg(QString::number(CurrentArea->index()), CurrentArea->name());

        QPair<QString, QString> r_name; // [regarding] name & area..
        if (m_content.size() >= 2){ /* if the caller/client using <report button> feature */
            bool target_ok = false;
            const auto t_client = CurrentServer->getClientByID(m_content.at(1).toInt(&target_ok));
            if (target_ok && !t_client.isNull() && t_client->m_joined){ /* make sure both param are valid.. */
                QPointer<AreaData> t_clientArea(CurrentServer->getAreaById(t_client->areaId()));

                QStringList t_name({"[" + QString::number(t_client->clientId()) + "]", t_client->character().isEmpty() ? "[Spectator]" : t_client->character(), "[" + t_client->getIpid() + "]"});
                if (!t_client->name().isEmpty())
                    t_name.insert(1, ": (" + t_client->name() + ")");

                QString t_areaname;
                if (!t_clientArea.isNull()){
                    t_areaname = t_clientArea == CurrentArea ? "[THIS AREA]" : QString("[%1] %2").arg(QString::number(t_clientArea->index()), t_clientArea->name());
                    PrintToOOC.append(QString("├── Regarding: %1 in %2").arg(t_name.join(' '), t_areaname));
                }
                else
                    PrintToOOC.append(QString("├── Regarding: %1").arg(t_name.join(' ')));

                r_name = qMakePair(t_name.join(' '), t_areaname);
            }
        }

        PrintToOOC.append("├── Reason: " + m_content[0]);
        PrintToOOC.append("└───────────────");

        CurrentServer->broadcast(PacketFactory::createPacket("ZZ", {PrintToOOC.join('\n')}), AOClient::AuthenticateType::MODERATOR);
        emit client.logModcall((client.character() + " " + client.characterName()), {client.clientId(), client.m_ipid}, client.name(), CurrentArea->name());
        if (ConfigManager::discordWebhookEnabled())
            emit CurrentServer->modcallWebhookRequest(l_name, r_name, m_content[0], CurrentServer->getAreaBuffer(CurrentArea->name()));
    }
}
