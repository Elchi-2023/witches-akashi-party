#include "packet_ma.h"

#include "config_manager.h"
#include "db_manager.h"
#include "server.h"
#include "packet/packet_factory.h"
#include "packet/packet_ct.h"

PacketMA::PacketMA(QStringList &contents) :
    AOPacket(contents)
{
}

PacketInfo PacketMA::getPacketInfo() const
{
    PacketInfo info{
        .acl_permission = ACLRole::Permission::NONE,
        .min_args = 3,
        .header = "MA"};
    return info;
}

void PacketMA::handlePacket(AreaData *area, AOClient &client) const
{
    Q_UNUSED(area);

    if (!client.m_joined) /* reject */
        client.m_socket->close();
    else if (client.isAuthenticated()){
        bool id_ok = false;
        auto target = client.getServer()->getClientByID(m_content.at(0).toInt(&id_ok));

        if (!id_ok)
            client.sendServerMessage("Invalid client id.");
        else if (target.isNull())
            client.sendServerMessage("User not found.");
        else{
            bool duration_ok = false;
            const int duration = qMax(-1, m_content.at(1).toInt(&duration_ok));

            if (!duration_ok)
                client.sendServerMessage("Invalid duration.");
            else{
                const auto clients = client.getServer()->getClientsByIpid(target->m_ipid);
                const QString moderator_name = ConfigManager::authType() == DataTypes::AuthType::ADVANCED ? client.m_moderator_name : client.isVAuthenticated() ? "[VIP]" : "[Moderator]";
                const QString m_reason = m_content[2];

                switch (duration){
                case 0: // [kick] type..
                    if (client.checkPermission(ACLRole::KICK)){
                        for (int index = 0; index < clients.size(); ++index){
                            clients[index]->m_is_multiclient = index != 0;
                            clients[index]->m_disconnect_reason = AOClient::Disconnected::KICK;
                            clients[index]->sendPacket("KK", {m_reason});
                            clients[index]->m_socket->close();
                        }

                        Q_EMIT client.logKick(moderator_name, target->m_ipid, m_reason);
                        if (client.isVAuthenticated()){
                            client.sendServerMessage(QString("Kicked %1 client(s) for client ID (%2) for reason: %3").arg(QString::number(clients.size()), QString::number(target->clientId()), m_reason));
                            client.getServer()->broadcast(PacketCT::CreateMessageS(QString("[%1] %2 is kicked (%3) clients for an reason: %4").arg(QString::number(client.clientId()), client.name().compare(client.m_moderator_name, Qt::CaseInsensitive) == 0 ? client.m_moderator_name : (client.name() + " | " + client.m_moderator_name), target->m_ipid, m_reason)), AOClient::AuthenticateType::MODERATOR);
                        }
                        else
                            client.sendServerMessage(QString("Kicked %1 client(s) with ipid %2 for reason: %3").arg(QString::number(clients.size()), target->m_ipid, m_reason));
                    }
                    else
                        client.sendServerMessage("You do not have permission to kick users.");
                    break;
                default: // [ban] type..
                    if (client.checkPermission(ACLRole::BAN)){
                        DBManager::BanInfo ban;

                        ban.ip = target->m_remote_ip;
                        ban.ipid = target->m_ipid;
                        ban.moderator = moderator_name;
                        ban.reason = m_reason;
                        ban.time = QDateTime::currentDateTime().toSecsSinceEpoch();
                        ban.duration = duration == -1 ? -2 : duration * 60;
                        ban.m_type = client.m_authenticated_type;

                        const QString m_timestamp(duration == -1 ? "permanently" : QDateTime::fromSecsSinceEpoch(ban.time).addSecs(ban.duration).toString("MM/dd/yyyy, hh:mm"));

                        for (int index = 0; index < clients.size(); ++index){
                            auto subclient = clients[index];
                            ban.hdid = subclient->m_hwid;

                            client.getServer()->getDatabaseManager()->addBan(ban);

                            subclient->m_disconnect_reason = AOClient::Disconnected::BAN;
                            subclient->sendPacket("KB", {m_reason});
                            subclient->m_socket->close();
                        }

                        if (client.isVAuthenticated()){
                            client.sendServerMessage(QString("Banned %1 client(s) for client ID (%2) for %3. Reason: %4").arg(QString::number(clients.size()), QString::number(target->clientId()), m_timestamp, m_reason));
                            client.getServer()->broadcast(PacketCT::CreateMessageS(QString("[%1] %2 is banned (%3) clients for an reason: %4").arg(QString::number(client.clientId()), client.name().compare(client.m_moderator_name, Qt::CaseInsensitive) == 0 ? client.m_moderator_name : (client.name() + " | " + client.m_moderator_name), target->m_ipid, m_reason)), AOClient::AuthenticateType::MODERATOR);
                        }
                        else
                            client.sendServerMessage(QString("Banned %1 client(s) with ipid %2 for %3. Reason: %4").arg(QString::number(clients.size()), target->m_ipid, m_timestamp, m_reason));

                        Q_EMIT client.logBan(moderator_name, target->m_ipid, m_timestamp, m_reason);
                        const int ban_id = client.getServer()->getDatabaseManager()->getBanID(ban.ip);
                        if (ConfigManager::discordBanWebhookEnabled()){
                            QStringList Name(QString::number(client.clientId()));
                            if (client.name().compare(client.m_moderator_name, Qt::CaseInsensitive) == 0)
                                Name << client.m_moderator_name;
                            else
                                Name << client.name() + " (" + client.m_moderator_name + ")";
                            const QString l_ban_duration_discord_format = ban.duration >= 0 ? QString("<t:%1:R>").arg(QDateTime::fromSecsSinceEpoch(ban.time).addSecs(ban.duration).toSecsSinceEpoch()) : "Permanently";
                            emit client.getServer()->banWebhookRequest(ban.ipid, qMakePair(ban.m_type, Name.join(' ')), l_ban_duration_discord_format, ban.reason, ban_id, clients.size());
                        }
                    }
                    else
                        client.sendServerMessage("You do not have permission to ban users.");
                    break;
                }
            }
        }
    }
    else
        client.sendServerMessage("You are not logged in!");
}
