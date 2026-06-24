//////////////////////////////////////////////////////////////////////////////////////
//    akashi - a server for Attorney Online 2                                       //
//    Copyright (C) 2020  scatterflower                                             //
//                                                                                  //
//    This program is free software: you can redistribute it and/or modify          //
//    it under the terms of the GNU Affero General Public License as                //
//    published by the Free Software Foundation, either version 3 of the            //
//    License, or (at your option) any later version.                               //
//                                                                                  //
//    This program is distributed in the hope that it will be useful,               //
//    but WITHOUT ANY WARRANTY; without even the implied warranty of                //
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                 //
//    GNU Affero General Public License for more details.                           //
//                                                                                  //
//    You should have received a copy of the GNU Affero General Public License      //
//    along with this program.  If not, see <https://www.gnu.org/licenses/>.        //
//////////////////////////////////////////////////////////////////////////////////////
#include "aoclient.h"

#include "area_data.h"
#include "command_extension.h"
#include "packet/packet_factory.h"
#include "packet/packet_ct.h"
#include "config_manager.h"
#include "db_manager.h"
#include "server.h"

// This file is for commands under the moderation category in aoclient.h
// Be sure to register the command in the header before adding it here!

void AOClient::cmdBan(int argc, QStringList argv)
{
    Q_UNUSED(argc)
    
    DBManager::BanInfo l_ban;
    
    if (isVAuthenticated()){ // [VIP].. they only using it by ids. .
        const long long l_duration_seconds = argv[1].compare("perma", Qt::CaseInsensitive) == 0 ? -2 : CalendarParse(argv[1]);

        bool valid_id;
        auto l_client = server->getClientByID(argv[0].toInt(&valid_id));

        if (!valid_id)
            sendServerMessage("That doesn't look a valid id.");
        else if (l_client.isNull())
            sendServerMessage("Target ID are not exist or Invalid.");
        else if (l_duration_seconds == -1)
            sendServerMessage("Invalid time format. Format example: 1h30m or \"perma\"");
        else{
            l_ban.duration = l_duration_seconds;
            l_ban.ipid = l_client->m_ipid;
            l_ban.reason = argv.mid(2).join(" ");
            l_ban.time = QDateTime::currentDateTime().toSecsSinceEpoch();
            l_ban.m_type = 0;

            const QList<QPointer<AOClient>> l_targets = server->getClientsByIpid(l_ban.ipid);
            QStringList Name("[" + QString::number(clientId()) + "]");
            switch (ConfigManager::authType()) {
            case DataTypes::AuthType::SIMPLE:
                l_ban.moderator = "[VIP]";
                Name[0].prepend("[VIP]");
                Name << m_moderator_name;
                break;
            case DataTypes::AuthType::ADVANCED:
                l_ban.moderator = m_moderator_name;
                name().compare(l_ban.moderator, Qt::CaseInsensitive) == 0 ? Name.append(l_ban.moderator) : Name.append({name(), "(" + l_ban.moderator + ")"});
                break;
            }

            l_ban.ip = l_client->m_remote_ip;
            l_ban.hdid = l_client->m_hwid;
            server->getDatabaseManager()->addBan(l_ban);
            const QDateTime ban_until = QDateTime::fromSecsSinceEpoch(l_ban.time).addSecs(l_ban.duration);
            const QString remains  = EpochToString(std::chrono::seconds(ban_until.secsTo(QDateTime::fromSecsSinceEpoch(l_ban.time))));
            const int l_ban_id = server->getDatabaseManager()->getBanID(l_ban.ip);
            int l_kick_counter = 0;

            for (auto target : l_targets){
                const QString l_ban_duration = l_ban.duration >= 0 ? QString("%1 (%2)").arg(ban_until.toString("MM/dd/yyyy, hh:mm"), remains) : "Permanently.";
                target->m_is_multiclient = l_kick_counter >= 1;
                target->m_disconnect_reason = Disconnected::BAN;
                target->sendPacket("KB", {QStringList({l_ban.reason, "ID: " + QString::number(l_ban_id), "Until: " + l_ban_duration}).join('\n')});
                target->m_socket->close();
                l_kick_counter += 1;
            }

            emit logBan(l_ban.moderator, l_ban.ipid, l_ban.duration >= 0 ? ban_until.toString("MM/dd/yyyy, hh:mm") : "Permanently.", l_ban.reason);
            if (ConfigManager::discordBanWebhookEnabled()){
                const QString l_ban_duration_discord_format = l_ban.duration >= 0 ? QString("<t:%1:R>").arg(ban_until.toSecsSinceEpoch()) : "Permanently";
                emit server->banWebhookRequest(l_ban.ipid, qMakePair(l_ban.m_type, Name.join(' ')), l_ban_duration_discord_format, l_ban.reason, l_ban_id, l_targets.size());
            }

            sendServerMessage(QString("You are banned client id of [%1] (%2) for reason: [%3]\nand kills %4 clients with matching linked clients.").arg(QString::number(l_client->clientId()), l_duration_seconds == -2 ? "permanently" : argv[1], l_ban.reason, QString::number(l_targets.size())), "[Mini-Moderation]");
            server->broadcast(PacketCT::CreateMessageS(QString("%1 is banning client of [%2] %3 for reason: %4\nkicked %5 clients with matching linked client(s).").arg(Name.join(' '), QString::number(l_client->clientId()) + " | "  + l_ban.ipid, l_duration_seconds == -2 ? "permanently" : argv[1], l_ban.reason, QString::number(l_targets.size())), "[Moderation]"), AOClient::AuthenticateType::MODERATOR); /* > notify the online [Moderator] about an [VIP] is banning someone.. < */

        }
    }
    else{
        const long long l_duration_seconds = argv[1].compare("perma", Qt::CaseInsensitive) == 0 ? -2 : CalendarParse(argv[1]);
        bool isclientID;
        const auto l_client = server->getClientByID(argv[0].toInt(&isclientID));

        if (isclientID){
            if (l_client.isNull())
                sendServerMessage("Target ID are not exist or Invalid.");
            else if (l_duration_seconds == -1)
                sendServerMessage("Invalid time format. Format example: 1h30m or \"perma\"");
            else{
                l_ban.ipid = l_client->m_ipid;
                l_ban.duration = l_duration_seconds;
                l_ban.reason = argv.mid(2).join(" ");
                l_ban.time = QDateTime::currentDateTime().toSecsSinceEpoch();

                switch (ConfigManager::authType()) {
                case DataTypes::AuthType::SIMPLE:
                    l_ban.moderator = "[Moderator]";
                    l_ban.m_type = 1;
                    break;
                case DataTypes::AuthType::ADVANCED:
                    l_ban.moderator = m_moderator_name;
                    l_ban.m_type = server->getDatabaseManager()->getUserType(m_moderator_name);
                    break;
                }

                const QList<QPointer<AOClient>> l_targets = server->getClientsByIpid(l_ban.ipid);

                l_ban.ip = l_client->m_remote_ip;
                l_ban.hdid = l_client->m_hwid;
                server->getDatabaseManager()->addBan(l_ban);

                const QDateTime ban_until = QDateTime::fromSecsSinceEpoch(l_ban.time).addSecs(l_ban.duration);
                const QString remains = EpochToString(std::chrono::seconds(ban_until.secsTo(QDateTime::fromSecsSinceEpoch(l_ban.time))));
                const int l_ban_id = server->getDatabaseManager()->getBanID(l_ban.ip);
                int l_kick_counter = 0;

                for (auto target : l_targets){
                    const QString l_ban_duration = l_ban.duration >= 0 ? QString("%1 (%2)").arg(ban_until.toString("MM/dd/yyyy, hh:mm"), remains) : "Permanently.";
                    target->m_is_multiclient = l_kick_counter >= 1;
                    target->m_disconnect_reason = Disconnected::BAN;
                    target->sendPacket("KB", {QStringList({l_ban.reason, "ID: " + QString::number(l_ban_id), "Until: " + l_ban_duration}).join('\n')});
                    target->m_socket->close();
                    l_kick_counter += 1;
                }

                emit logBan(l_ban.moderator, l_ban.ipid, l_ban.duration >= 0 ? ban_until.toString("MM/dd/yyyy, hh:mm") : "Permanently.", l_ban.reason);
                if (ConfigManager::discordBanWebhookEnabled()){
                    QStringList Name("[" + QString::number(clientId()) + "]");
                    if (name().compare(l_ban.moderator, Qt::CaseInsensitive) == 0)
                        Name.append(l_ban.moderator);
                    else
                        Name.append({name(), "(" + l_ban.moderator + ")"});
                    const QString l_ban_duration_discord_format = l_ban.duration >= 0 ? QString("<t:%1:R>").arg(ban_until.toSecsSinceEpoch()) : "Permanently";
                    emit server->banWebhookRequest(l_ban.ipid, qMakePair(l_ban.m_type, Name.join(' ')), l_ban_duration_discord_format, l_ban.reason, l_ban_id, l_targets.size());
                }

                sendServerMessage(QString("You are banned client id of [%1] (%2) for reason: [%3]\nand kills %4 clients with matching linked clients.").arg(QString::number(l_client->clientId()) + " | " + l_ban.ipid, l_duration_seconds == -2 ? "permanently" : remains, l_ban.reason, QString::number(l_targets.size())), "[Moderation]");
            }
        }
        else if (argv[0].length() != 8)
            sendServerMessage("Invalid target ipid, length must exacty 8.");
        else if (l_duration_seconds == -1)
            sendServerMessage("Invalid time format. Format example: 1h30m or \"perma\"");
        else{
            l_ban.ipid = argv[0];
            l_ban.duration = l_duration_seconds;
            l_ban.reason = argv.mid(2).join(" ");
            l_ban.time = QDateTime::currentDateTime().toSecsSinceEpoch();

            switch (ConfigManager::authType()) {
            case DataTypes::AuthType::SIMPLE:
                l_ban.moderator = "[Moderator]";
                l_ban.m_type = 1;
                break;
            case DataTypes::AuthType::ADVANCED:
                l_ban.moderator = m_moderator_name;
                l_ban.m_type = server->getDatabaseManager()->getUserType(m_moderator_name);
                break;
            }

            const QList<QPointer<AOClient>> l_targets = server->getClientsByIpid(l_ban.ipid);

            if (l_targets.isEmpty()){ /* We're banning someone not connected. */
                server->getDatabaseManager()->addBan(l_ban);
                const int l_ban_id = server->getDatabaseManager()->getBanID(l_ban.ip);
                const QDateTime ban_until = QDateTime::fromSecsSinceEpoch(l_ban.time).addSecs(l_ban.duration);
                const QString remains = EpochToString(std::chrono::seconds(ban_until.secsTo(QDateTime::fromSecsSinceEpoch(l_ban.time))));
                sendServerMessage(QString("You are banned a ipid of [%1] with reason: %2").arg(l_ban.ipid, l_ban.reason), "[Moderation]");
                if (ConfigManager::discordBanWebhookEnabled()){
                    QStringList Name("[" + QString::number(clientId()) + "]");
                    if (name().compare(l_ban.moderator, Qt::CaseInsensitive) == 0)
                        Name.append(l_ban.moderator);
                    else
                        Name.append({name(), "(" + l_ban.moderator + ")"});
                    const QString l_ban_duration_discord_format = l_ban.duration >= 0 ? QString("<t:%1:R>").arg(ban_until.toSecsSinceEpoch()) : "Permanently";
                    emit server->banWebhookRequest(l_ban.ipid, qMakePair(l_ban.m_type, Name.join(' ')), l_ban_duration_discord_format, l_ban.reason, l_ban_id, 0);
                }
            }
            else{
                l_ban.ip = l_targets.first()->m_remote_ip;
                l_ban.hdid = l_targets.first()->m_hwid;
                server->getDatabaseManager()->addBan(l_ban);

                const QDateTime ban_until = QDateTime::fromSecsSinceEpoch(l_ban.time).addSecs(l_ban.duration);
                const QString remains  = EpochToString(std::chrono::seconds(ban_until.secsTo(QDateTime::fromSecsSinceEpoch(l_ban.time))));
                const int l_ban_id = server->getDatabaseManager()->getBanID(l_ban.ip);
                int l_kick_counter = 0;

                for (auto target : l_targets){
                    const QString l_ban_duration = l_ban.duration >= 0 ? QString("%1 (%2)").arg(ban_until.toString("MM/dd/yyyy, hh:mm"), remains) : "Permanently.";
                    target->m_is_multiclient = l_kick_counter >= 1;
                    target->m_disconnect_reason = Disconnected::BAN;
                    target->sendPacket("KB", {QStringList({l_ban.reason, "ID: " + QString::number(l_ban_id), "Until: " + l_ban_duration}).join('\n')});
                    target->m_socket->close();
                    l_kick_counter += 1;
                }

                emit logBan(l_ban.moderator, l_ban.ipid, l_ban.duration >= 0 ? ban_until.toString("MM/dd/yyyy, hh:mm") : "Permanently.", l_ban.reason);
                if (ConfigManager::discordBanWebhookEnabled()){
                    QStringList Name("[" + QString::number(clientId()) + "]");
                    if (name().compare(l_ban.moderator, Qt::CaseInsensitive) == 0)
                        Name.append(l_ban.moderator);
                    else
                        Name.append({name(), "(" + l_ban.moderator + ")"});
                    const QString l_ban_duration_discord_format = l_ban.duration >= 0 ? QString("<t:%1:R>").arg(ban_until.toSecsSinceEpoch()) : "Permanently";
                    emit server->banWebhookRequest(l_ban.ipid, qMakePair(l_ban.m_type, Name.join(' ')), l_ban_duration_discord_format, l_ban.reason, l_ban_id, l_targets.size());
                }
                sendServerMessage(QString("You are banned client id of [%1] (%2) for reason: [%3]\nand kills %4 clients with matching linked clients.").arg(QString::number(l_client->clientId()) + " | " + l_ban.ipid, l_duration_seconds == -2 ? "permanently" : argv[1], l_ban.reason, QString::number(l_targets.size())), "[Moderation]");
            }
        }
    }
}

void AOClient::cmdKick(int argc, QStringList argv)
{
    Q_UNUSED(argc)
    
    QString l_target_ipid = argv[0];
    QString l_reason = argv.mid(1).join(" ");
    
    if (l_target_ipid.startsWith("*")){ // > [single] <
        bool isclientID;
        int clientID = QString(l_target_ipid).remove(0, 1).toInt(&isclientID);
        if (isclientID){ // > [ID] <
            auto l_client = server->getClientByID(clientID);
            if (l_client.isNull())
                sendServerMessage("User with that id not found!");
            else{
                l_client->m_disconnect_reason = Disconnected::KICK;
                l_client->sendPacket("KK", {l_reason});
                l_client->m_socket->close();

                switch (m_authenticated_type){
                case AOClient::AuthenticateType::NONE:
                    break;
                case AOClient::AuthenticateType::VIP:
                    ConfigManager::authType() == DataTypes::AuthType::ADVANCED ? emit logKick("[VIP] " + m_moderator_name, l_client->m_ipid, l_reason) : emit logKick("[VIP]", l_client->m_ipid, l_reason);
                    sendServerMessage(QString("Single kicked for ID %1 for an reason: %2").arg(QString::number(l_client->clientId()), l_reason));
                    server->broadcast(PacketCT::CreateMessageS(QString("[%1] %2 is Single kicked of (%3) client for an reason: %4").arg(QString::number(clientId()), name().compare(m_moderator_name, Qt::CaseInsensitive) == 0 ? m_moderator_name : (name() + " | " + m_moderator_name), l_target_ipid, l_reason)), AOClient::AuthenticateType::MODERATOR);
                    break;
                default:
                    ConfigManager::authType() == DataTypes::AuthType::ADVANCED ? emit logKick(m_moderator_name, l_client->m_ipid, l_reason) : emit logKick("[Moderator]", l_client->m_ipid, l_reason);
                    sendServerMessage(QString("Single kicked for %1 for an reason: %2").arg(QString::number(l_client->clientId()) + " | " + l_client->m_ipid, l_reason));
                    break;
                }
            }
        }
        else if (!isMAuthenticated()) // since.. [VIP] cannot see [IPID], nothing can do with this..
            sendServerMessage("That not looks valid ID.");
        else if (l_target_ipid.length() != 8)
            sendServerMessage("Invalid target ipid, length must exacty 8.");
        else if (!server->getClientsByIpid(QString(l_target_ipid).remove(0, 1)).isEmpty()){ // > [IPID] <
            auto l_client = server->getClientsByIpid(QString(l_target_ipid).remove(0, 1)).first(); // catching the first client from the list..
            l_client->m_disconnect_reason = Disconnected::KICK;
            l_client->sendPacket("KK", {l_reason});
            l_client->m_socket->close();

            ConfigManager::authType() == DataTypes::AuthType::ADVANCED ? emit logKick(m_moderator_name, l_client->m_ipid, l_reason) : emit logKick("[Moderator]", l_client->m_ipid, l_reason);
            sendServerMessage(QString("Single kicked for %1 for an reason: %2").arg(QString::number(l_client->clientId()) + " | " + l_client->name(), l_reason));
        }
        else
            sendServerMessage("User with that ipid/id not found!");
    }
    else{
        bool isclientID;
        auto target = server->getClientByID(l_target_ipid.toInt(&isclientID));
        if (isclientID){
            if (target.isNull())
                sendServerMessage("User with id not found!");
            else{
                const QList<QPointer<AOClient>> l_targets = server->getClientsByIpid(target->m_ipid);
                for (int index = 0; index < l_targets.size(); ++index){
                    l_targets[index]->m_is_multiclient = index != 0;
                    l_targets[index]->m_disconnect_reason = Disconnected::KICK;
                    l_targets[index]->sendPacket("KK", {l_reason});
                    l_targets[index]->m_socket->close();
                }
                switch (m_authenticated_type){
                case AOClient::AuthenticateType::NONE:
                    break;
                case AOClient::AuthenticateType::VIP:
                    ConfigManager::authType() == DataTypes::AuthType::ADVANCED ? emit logKick("[VIP] " + m_moderator_name, l_target_ipid, l_reason) : emit logKick("[VIP]", l_target_ipid, l_reason);
                    sendServerMessage("Kicked " + QString::number(l_targets.size()) + " client(s) for reason: " + l_reason);
                    server->broadcast(PacketCT::CreateMessageS(QString("[%1] %2 is kicked (%3) clients for an reason: %4").arg(QString::number(clientId()), name().compare(m_moderator_name, Qt::CaseInsensitive) == 0 ? m_moderator_name : (name() + " | " + m_moderator_name), l_target_ipid, l_reason)), AOClient::AuthenticateType::MODERATOR);
                    break;
                default:
                    ConfigManager::authType() == DataTypes::AuthType::ADVANCED ? emit logKick(m_moderator_name, l_target_ipid, l_reason) : emit logKick("Moderator", l_target_ipid, l_reason);
                    sendServerMessage("Kicked " + QString::number(l_targets.size()) + " client(s) for reason: " + l_reason);
                    break;
                }
            }
        }
        else if (!isMAuthenticated())
            sendServerMessage("That not looks valid ID.");
        else if (l_target_ipid.length() != 8)
            sendServerMessage("Invalid target ipid, length must exacty 8.");
        else{
            const auto l_targets = server->getClientsByIpid(l_target_ipid);

            if (l_targets.isEmpty())
                sendServerMessage("User with ipid not found!");
            else{
                for (int index = 0; index < l_targets.size(); ++index){
                    l_targets[index]->m_is_multiclient = index != 0;
                    l_targets[index]->m_disconnect_reason = Disconnected::KICK;
                    l_targets[index]->sendPacket("KK", {l_reason});
                    l_targets[index]->m_socket->close();
                }
                switch (m_authenticated_type){
                case AOClient::AuthenticateType::NONE:
                    break;
                case AOClient::AuthenticateType::VIP:
                    ConfigManager::authType() == DataTypes::AuthType::ADVANCED ? emit logKick("[VIP] " + m_moderator_name, l_target_ipid, l_reason) : emit logKick("[VIP]", l_target_ipid, l_reason);
                    sendServerMessage("Kicked " + QString::number(l_targets.size()) + " client(s) for reason: " + l_reason);
                    server->broadcast(PacketCT::CreateMessageS(QString("[%1] %2 is kicked (%3) clients for an reason: %4").arg(QString::number(clientId()), name().compare(m_moderator_name, Qt::CaseInsensitive) == 0 ? m_moderator_name : (name() + " | " + m_moderator_name), l_target_ipid, l_reason)), AOClient::AuthenticateType::MODERATOR);
                    break;
                default:
                    ConfigManager::authType() == DataTypes::AuthType::ADVANCED ? emit logKick(m_moderator_name, l_target_ipid, l_reason) : emit logKick("Moderator", l_target_ipid, l_reason);
                    sendServerMessage("Kicked " + QString::number(l_targets.size()) + " client(s) for reason: " + l_reason);
                    break;
                }
            }
        }
    }
}

void AOClient::cmdMods(int argc, QStringList argv){
    Q_UNUSED(argc);
    Q_UNUSED(argv);
    QHash<int, QStringList> EntriesMap; /* why use qhash/qmap?.. because we needs area_id for get areas name */
    for (auto client : server->getClients()){
        const auto current_role = client->getServer()->getACLRolesHandler()->getRoleById(client->m_acl_role_id);
        if (client->isMAuthenticated()){
            QStringList user_entry(QString("[%1] %2").arg(QString::number(client->clientId()), client->character().isEmpty() ? "[Spectator]" : client->character()));
            if (client->m_authenticated_type == AuthenticateType::ROOT) /* marked the "👑" for "root"/owner */
                user_entry[0].prepend(m_version.type == AOClient::ClientVersion::NDS ? "[OWNER]" : "[👑]");
            else if (current_role.checkPermission(ACLRole::SUPER)) /* marked the [SUPER] perms moderator */
                user_entry[0].prepend("[S]");
            
            if (isMAuthenticated()){ /* only moderator can see names */
                if (client->name().compare(client->m_moderator_name, Qt::CaseInsensitive) == 0) /* otherwise.. just shown their moderator name instead */
                    user_entry << "(" + client->m_moderator_name + ")";
                else /* capture the diff name between their oocname and their moderator name */
                    user_entry << "(" + QStringList({client->name(), client->m_moderator_name}).join(" | ") + ")";
            }
            m_version.type == AOClient::ClientVersion::NDS ? user_entry.prepend(client == this ? "[YOU]" : " * ") : user_entry.prepend(client == this ? " ➤ " : " · "); /* if /getarea(s) has user marked.. why not we do the same for this */
            if (EntriesMap.contains(client->areaId())) /* if area_id was on enteriesmap, we adds another entry */
                EntriesMap[client->areaId()].append(user_entry.join(' '));
            else /* otherwise, we needs captures area_id and entry */
                EntriesMap.insert(client->areaId(), {user_entry.join(' ')});
        }
        else if (client->isVAuthenticated()){ // if is [VIP] (note: only some of perms could registered as [M-VIP] (mini-moderator-vip), otherwise never be shown)..
            QStringList user_entry(QString("[%1] %2").arg(QString::number(client->clientId()), client->character().isEmpty() ? "[Spectator]" : client->character()));
            if (current_role.checkPermission(ACLRole::SUPER)){ // [SUPER] but [VIP]..
                user_entry[0].prepend("[S-VIP]");
                if (isMAuthenticated()){
                    if (client->name().compare(client->m_moderator_name, Qt::CaseInsensitive) == 0)
                        user_entry << "(" + client->m_moderator_name + ")";
                    else
                        user_entry << "(" + QStringList({client->name(), client->m_moderator_name}).join(" | ") + ")";
                }
                m_version.type == AOClient::ClientVersion::NDS ? user_entry.prepend(client == this ? "[YOU]" : " * ") : user_entry.prepend(client == this ? " ➤ " : " · ");
                if (EntriesMap.contains(client->areaId()))
                    EntriesMap[client->areaId()].append(user_entry.join(' '));
                else
                    EntriesMap.insert(client->areaId(), {user_entry.join(' ')});
            }
            else if (current_role.checkPermission(ACLRole::KICK) || current_role.checkPermission(ACLRole::BAN) || current_role.checkPermission(ACLRole::MUTE) || current_role.checkPermission(ACLRole::MODCHAT)){
                user_entry[0].prepend("[M-VIP]");
                if (isMAuthenticated()){
                    if (client->name().compare(client->m_moderator_name, Qt::CaseInsensitive) == 0)
                        user_entry << "(" + client->m_moderator_name + ")";
                    else
                        user_entry << "(" + QStringList({client->name(), client->m_moderator_name}).join(" | ") + ")";
                }
                m_version.type == AOClient::ClientVersion::NDS ? user_entry.prepend(client == this ? "[YOU]" : " * ") : user_entry.prepend(client == this ? " ➤ " : " · ");
                if (EntriesMap.contains(client->areaId()))
                    EntriesMap[client->areaId()].append(user_entry.join(' '));
                else
                    EntriesMap.insert(client->areaId(), {user_entry.join(' ')});
            }
        }
    }
    
    /* kfo/tsu-like behavior */
    QStringList entry("=== Moderator ===");
    int entry_count = 0;
    
    for (auto EMap = EntriesMap.begin(); EMap != EntriesMap.end(); ++EMap){
        auto area = server->getAreaById(EMap.key());
        if (area.isNull())
            continue;
        
        entry.append(QString("=== [%1] %2 ===\n%3").arg(QString::number(area->index()), area->name(), EMap.value().join('\n')));
        entry_count += EMap.value().size();
    }
    entry.append(QString("=== Total online : %1 ===").arg(QString::number(entry_count)));
    
    sendServerMessage('\n' + entry.join('\n'));
}

void AOClient::cmdCurses(int argc, QStringList argv){
    bool valid_id = false;
    auto target_client = server->getClientByID(argv[0].toInt(&valid_id));
    
    if (!valid_id || target_client.isNull())
        sendServerMessage("Invalid user ID or not exist client.");
    else{
        switch (argc){
        case 1:
            if (target_client->isCursed(CurseType::DISEMVOWEL))
                sendServerMessage("That target are already been curses (disemvoweled).");
            else{
                target_client->SetCursed(CurseType::DISEMVOWEL, true);
                sendServerMessage("You gives target an curses (disemvoweled).");
                target_client->sendServerMessage("You been curses (disemvoweled) by Moderator! " + getReprimand(false));
            }
            break;
        default:
            bool valid_type = false;
            const int _type = argv[1].toInt(&valid_type);
            const QVector<QPair<AOClient::CurseType, QString>> define_type{
                {AOClient::CurseType::DISEMVOWEL, "disemvoweled"},
                {AOClient::CurseType::SHAKE, "shaked"},
                {AOClient::CurseType::MEDIEVAL, "medievaled"},
                {AOClient::CurseType::GIMP, "gimped"},
                {AOClient::CurseType::UWUIFY, "uwuify"},
                {AOClient::CurseType::PIGIFY, "pigify"}
            };
            if (valid_type){
                switch (_type){
                case -1:
                    if (target_client->isCursed(CurseType::FULL))
                        sendServerMessage("That target are already been *TRUE* curses.");
                    else{
                        target_client->SetCursed(CurseType::FULL, true);
                        sendServerMessage("You gives target an *TRUE* curses.");
                        target_client->sendServerMessage("You been *TRUE* curses by Moderator! " + getReprimand(false));
                    }
                    break;
                default:
                    if (_type >= 0 && _type <= define_type.size() -1){
                        if (target_client->isCursed(define_type[_type].first))
                            sendServerMessage(QString("That target are already been curses (%1)").arg(define_type[_type].second));
                        else{
                            target_client->SetCursed(define_type[_type].first, true);
                            sendServerMessage(QString("You gives target an curses (%1).").arg(define_type[_type].second));
                            target_client->sendServerMessage(QString("You been curses (%1) by Moderator! ").arg(define_type[_type].second) + getReprimand(false));
                        }
                    }
                    else{
                        if (target_client->isCursed(CurseType::DISEMVOWEL))
                            sendServerMessage("That target are already been curses (disemvoweled).");
                        else{
                            target_client->SetCursed(CurseType::DISEMVOWEL, true);
                            sendServerMessage("You gives target an curses (disemvoweled).");
                            target_client->sendServerMessage("You been curses (disemvoweled) by Moderator! " + getReprimand(false));
                        }
                    }
                    break;
                }
            }
            else
                sendServerMessage("invalid curse-type.");
            break;
        }
    }
}

void AOClient::cmdUnCurses(int argc, QStringList argv){
    bool valid_id = false;
    auto target_client = server->getClientByID(argv[0].toInt(&valid_id));
    
    if (!valid_id || target_client.isNull())
        sendServerMessage("Invalid user ID or not exist client.");
    else{
        switch (argc){
        case 1:
            if (target_client->isCursed(CurseType::DISEMVOWEL)){
                target_client->SetCursed(CurseType::DISEMVOWEL, false);
                sendServerMessage("You freed target from (disemvoweled).");
                target_client->sendServerMessage("You been freed from an (disemvoweled) by Moderator! " + getReprimand(true));
            }
            else
                sendServerMessage("That target are already been freed from (disemvoweled).");
            break;
        default:
            bool valid_type = false;
            const int _type = argv[1].toInt(&valid_type);
            const QVector<QPair<AOClient::CurseType, QString>> define_type{
                {AOClient::CurseType::DISEMVOWEL, "disemvoweled"},
                {AOClient::CurseType::SHAKE, "shaked"},
                {AOClient::CurseType::MEDIEVAL, "medievaled"},
                {AOClient::CurseType::GIMP, "gimped"},
                {AOClient::CurseType::UWUIFY, "uwuify"},
                {AOClient::CurseType::PIGIFY, "pigify"}
            };
            if (valid_type){
                switch (_type){
                case -1:
                    if (target_client->isCursed(CurseType::FULL)){
                        target_client->SetCursed(CurseType::FULL, false);
                        sendServerMessage("You freed target from an *TRUE* curses.");
                        target_client->sendServerMessage("You been freed from an *TRUE* curses by Moderator! " + getReprimand(true));

                    }
                    else
                        sendServerMessage("That target are already freed from an *TRUE* curses.");
                    break;
                default:
                    if (_type >= 0 && _type <= define_type.size() -1){
                        if (target_client->isCursed(define_type[_type].first)){
                            target_client->SetCursed(define_type[_type].first, false);
                            sendServerMessage(QString("You freed target from (%1).").arg(define_type[_type].second));
                            target_client->sendServerMessage(QString("You been freed from an (%1) by Moderator! ").arg(define_type[_type].second) + getReprimand(false));
                        }
                        else
                            sendServerMessage(QString("That target are already freed from an (%1)").arg(define_type[_type].second));
                    }
                    else{
                        if (target_client->isCursed(CurseType::DISEMVOWEL)){
                            target_client->SetCursed(CurseType::DISEMVOWEL, false);
                            sendServerMessage("You freed target from (disemvoweled).");
                            target_client->sendServerMessage("You been freed from an (disemvoweled) by Moderator! " + getReprimand(true));
                        }
                        else
                            sendServerMessage("That target are already been freed from (disemvoweled).");
                    }
                    break;
                }
            }
            else
                sendServerMessage("invalid curse-type.");
            break;
        }
    }
}

void AOClient::cmdCommands(int argc, QStringList argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);
    
    QStringList l_entries;
    l_entries << "Allowed commands:";
    QMap<QString, CommandInfo>::const_iterator i;
    for (i = COMMANDS.constBegin(); i != COMMANDS.constEnd(); ++i) {
        const CommandInfo l_command = i.value();
        const CommandExtension l_extension = server->getCommandExtensionCollection()->getExtension(i.key());
        const QVector<ACLRole::Permission> l_permissions = l_extension.getPermissions(l_command.acl_permissions);
        bool l_has_permission = false;
        for (const ACLRole::Permission i_permission : qAsConst(l_permissions)) {
            if (checkPermission(i_permission)) {
                l_has_permission = true;
                break;
            }
        }
        if (!l_has_permission) {
            continue;
        }
        
        QString l_info = "/" + i.key();
        const QStringList l_aliases = l_extension.getAliases();
        if (!l_aliases.isEmpty()) {
            l_info += " [aka: " + l_aliases.join(", ") + "]";
        }
        l_entries << l_info;
    }
    sendServerMessage(l_entries.join("\n"));
}

void AOClient::cmdHelp(int argc, QStringList argv){
    Q_UNUSED(argc)

    if (argv.isEmpty()){
        QStringList l_category = category_command.uniqueKeys();
        l_category.sort();
        sendServerMessage(QString("Welcome to akashi! You can use /help <command> on any known command to get up-to-date help on it\nYou may also use /help <category> to see allowed commands for that category. (you can also see all-of-allowed commands by /help *)\nAvailable Categories:\n%1").arg(l_category.join('\n')));
    }
    else if (argv[0] == "*"){ // assuming if user want see all-of-allowed (based from their perms) commands..
        QStringList Listcommands;
        for (const auto &I : category_command.values()){
            const CommandExtension l_extension = server->getCommandExtensionCollection()->getExtension(I.first);
            const QVector<ACLRole::Permission> l_permissions = l_extension.getPermissions(I.second.acl_permissions);

            switch (l_permissions.size()){
            case 0:
                continue; // skipped
            case 1:
                if (!checkPermission(l_permissions[0]))
                    continue; // no perms?.. just skipped.
                break;
            default:
                bool l_has_permission = false;
                for (const ACLRole::Permission i_permission : qAsConst(l_permissions)) { // loop perms checker..
                    if (checkPermission(i_permission)) { // if user has perms from the <specific> perms..
                        l_has_permission = true; // set boolan to <true> and break the loop..
                        break;
                    }
                }
                if (!l_has_permission)
                    continue; // otherwise.. skipped..
                break;
            }

            const ConfigManager::help l_command_info = ConfigManager::commandHelp(I.first);
            QString command_name(l_extension.getAliases().isEmpty() ? ("/" + I.first) : ("/" + I.first + " [aka: /" + l_extension.getAliases().join(", /") + "]"));
            if (!l_command_info.usage.isEmpty()){
                command_name.append(": (" + l_command_info.usage + ")");
                if (!l_command_info.text.isEmpty())
                    command_name.append(" " + l_command_info.text);
            }
            Listcommands << command_name;
        }
        sendServerMessage("Allowed commands:\n" + Listcommands.join("\r\n"));
    }
    else if (category_command.contains(argv[0].toLower())){ // assuming it's <category> param..
        QStringList Listcommands;
        for (const auto &I : category_command.values(argv[0].toLower())){
            const CommandExtension l_extension = server->getCommandExtensionCollection()->getExtension(I.first);
            const QVector<ACLRole::Permission> l_permissions = l_extension.getPermissions(I.second.acl_permissions);

            switch (l_permissions.size()){
            case 0:
                continue;
            case 1:
                if (!checkPermission(l_permissions[0]))
                    continue;
                break;
            default:
                bool l_has_permission = false;
                for (const ACLRole::Permission i_permission : qAsConst(l_permissions)) {
                    if (checkPermission(i_permission)) {
                        l_has_permission = true;
                        break;
                    }
                }
                if (!l_has_permission)
                    continue;
                break;
            }

            const ConfigManager::help l_command_info = ConfigManager::commandHelp(I.first);
            QString command_name(l_extension.getAliases().isEmpty() ? ("/" + I.first) : ("/" + I.first + " [aka: /" + l_extension.getAliases().join(", /") + "]"));
            if (!l_command_info.usage.isEmpty()){
                command_name.append(": (" + l_command_info.usage + ")");
                if (!l_command_info.text.isEmpty())
                    command_name.append(" " + l_command_info.text);
            }
            Listcommands << command_name;
        }
        sendServerMessage(QString("Allowed commands from category [%1]:\n%2").arg(argv[0], Listcommands.join("\r\n")));
    }
    else if (ConfigManager::commandHelp(argv[0]).usage.isEmpty()) // ansuming it's <command> params..
        sendServerMessage("Unable to find the command/category " + argv[0] + ".");
    else{ // keep the behaviors as is..
        const ConfigManager::help l_command_info = ConfigManager::commandHelp(argv[0]);
        QStringList help_param(l_command_info.usage);
        if (!l_command_info.text.isEmpty())
            help_param << l_command_info.text;
        sendServerMessage("\n== Help ==\n" + help_param.join("\n\r"));
    }
}

void AOClient::cmdMOTD(int argc, QStringList argv)
{
    Q_UNUSED(argc)
    Q_UNUSED(argv)
    
    sendServerMessage("=== MOTD ===\r\n" + QString(ConfigManager::motd()).replace('\n', "\r\n") + "\r\n=============");
}

void AOClient::cmdSetMOTD(int argc, QStringList argv)
{
    Q_UNUSED(argc)
    
    QString l_MOTD = argv.join(" ");
    ConfigManager::setMotd(l_MOTD);
    sendServerMessage("MOTD has been changed.");
}

void AOClient::cmdBans(int argc, QStringList argv){
    Q_UNUSED(argc);
    Q_UNUSED(argv);
    const QList<DBManager::BanInfo> l_bans_list = server->getDatabaseManager()->getRecentBans();
    if (l_bans_list.isEmpty())
        sendServerMessage("There is nothing in recents of 5 ban(s)-list.");
    else{
        QStringList l_recent_bans;
        const auto client_type = m_version.type;
        for (const auto &l_ban : l_bans_list){
            const QString ids = QString("=== [%1] ===").arg(QString::number(l_ban.id));
            l_recent_bans << ids;
            if (client_type == ClientVersion::NDS){
                l_recent_bans << "├─ [M]: " + l_ban.moderator; // don't mind with the police emojis..
                if (l_ban.m_type >= 0)
                    l_recent_bans << "├─ [M-TYPE]: " + QStringList({"VIP", "Moderator", "ROOT"})[l_ban.m_type];
                if (isMAuthenticated()){ // [ipid / hdid] only been seen by moderators..
                    if (!l_ban.hdid.isEmpty())
                        l_recent_bans << "├─ [HDID]: " + l_ban.ipid;
                    if (!l_ban.ipid.isEmpty())
                        l_recent_bans << "├─ [IPID]: " + l_ban.ipid;
                }
                const QDateTime b_current_date = QDateTime::fromSecsSinceEpoch(l_ban.time);
                l_recent_bans << "├─ [Until]: " + QString(l_ban.duration == -2 ? "The heat death of the universe" : QString("%1 (%2)").arg(b_current_date.addSecs(l_ban.duration).toString("MM/dd/yyyy, hh:mm"), EpochToString(std::chrono::seconds(b_current_date.secsTo(QDateTime::currentDateTime())), true)));
                l_recent_bans << "├─ [BAN-Date]: " + b_current_date.toString("MM/dd/yyyy, hh:mm");
                l_recent_bans << "├─ [Reason]: " + l_ban.reason;
            }
            else{
                l_recent_bans << "├─ [👮]: " + l_ban.moderator;
                if (l_ban.m_type >= 0)
                    l_recent_bans << "├─ [👮|TYPE]: " + QStringList({"VIP", "Moderator", "ROOT"})[l_ban.m_type];
                if (isMAuthenticated()){
                    if (!l_ban.hdid.isEmpty())
                        l_recent_bans << "├─ [HDID]: " + l_ban.ipid;
                    if (!l_ban.ipid.isEmpty())
                        l_recent_bans << "├─ [IPID]: " + l_ban.ipid;
                }
                const QDateTime b_current_date = QDateTime::fromSecsSinceEpoch(l_ban.time);
                l_recent_bans << "├─ [⏳]: " + QString(l_ban.duration == -2 ? "The heat death of the universe" : QString("%1 (%2)").arg(b_current_date.addSecs(l_ban.duration).toString("MM/dd/yyyy, hh:mm"), EpochToString(std::chrono::seconds(b_current_date.secsTo(QDateTime::currentDateTime())), true)));
                l_recent_bans << "├─ [📅]: " + b_current_date.toString("MM/dd/yyyy, hh:mm");
                l_recent_bans << "├─ [📋]: " + l_ban.reason;
                l_recent_bans << QString().fill('=', ids.length());
            }
        }
        sendServerMessage("\n" + l_recent_bans.join('\n'), "[📝 Last 5 bans 📝]");
    }
}

void AOClient::cmdUnBan(int argc, QStringList argv)
{
    Q_UNUSED(argc);
    
    bool ok;
    int l_target_ban = argv[0].toInt(&ok);
    if (ok){
        const auto list = server->getDatabaseManager()->getBanInfo("banid", QString::number(l_target_ban));
        if (list.isEmpty())
            sendServerMessage("That ban ID not exists.");
        else if (server->getDatabaseManager()->invalidateBan(l_target_ban)){
            const auto current = list.first();
            sendServerMessage("Successfully invalidated ban " + argv[0] + ".");
            if (ConfigManager::discordBanWebhookEnabled()){
                QStringList Name("[" + QString::number(clientId()) + "]"), m_reason(current.reason);
                if (name().compare(m_moderator_name, Qt::CaseInsensitive) == 0)
                    Name.append(m_moderator_name);
                else
                    Name.append({name(), "(" + m_moderator_name + ")"});
                if (argv.size() > 1)
                    m_reason.append(argv.mid(1).join(" "));

                Q_EMIT server->UnbanWebhookRequested(current.ipid, {qMakePair(current.m_type, current.moderator), qMakePair(server->getDatabaseManager()->getUserType(m_moderator_name), Name.join(' '))}, current.id, current.duration, QDateTime::fromSecsSinceEpoch(current.time), m_reason);
            }
        }
        else
            sendServerMessage("Couldn't invalidate ban " + argv[0] + ".");
    }
    else
        sendServerMessage("Invalid ban ID.");
}

void AOClient::cmdAbout(int argc, QStringList argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);
    
    sendServerMessage(QString("This server using akashi %1! (Made with love by scatterflower, with help from in1tiate, Salanto, and mangosarentliterature).\nFor documentation and reporting issues, see the source: https://github.com/AttorneyOnline/akashi").arg(QCoreApplication::applicationVersion()), "[The akashi dev team]");
}

void AOClient::cmdMute(int argc, QStringList argv){
    bool conv_ok = false;
    auto target = server->getClientByID(argv[0].toInt(&conv_ok));
    if (conv_ok && !target.isNull()){
        switch (argc){
        case 1:
            if (target->isAccessBlocked(BlockType::IC))
                sendServerMessage(target == this ? "You already IC muted yourself though." : "That player is already IC muted!");
            else{
                target->SetAccessBlock(BlockType::IC, true);
                sendServerMessage("Muted player.");
                target->sendServerMessage(QString("You were IC muted by a %1. ").arg(isVAuthenticated() ? QString("[VIP] (ID: %1)").arg(clientId()) : "Moderator") + getReprimand());
            }
            break;
        default:
            bool type_ok = false;
            const int l_type = argv[1].toInt(&type_ok);
            if (type_ok){
                const QVector<QPair<BlockType, QString>> defined_type{
                    {BlockType::IC, "IC"},
                    {BlockType::OOC, "OOC"}
                };
                switch (l_type){
                case 0: case 1:
                    if (target->isAccessBlocked(defined_type[l_type].first))
                        sendServerMessage(QString("That player is already %1 muted!").arg(defined_type[l_type].second));
                    else{
                        target->SetAccessBlock(defined_type[l_type].first, true);
                        sendServerMessage(QString("Muted %1 player.").arg(defined_type[l_type].second));
                        target->sendServerMessage(QString("You were %1 muted by a %2.  ").arg(defined_type[l_type].second, isVAuthenticated() ? QString("[VIP] (ID: %1)").arg(clientId()) : "Moderator") + getReprimand());
                    }
                    break;
                case 2:
                    if (target->isAccessBlocked(BlockType::IC) && target->isAccessBlocked(BlockType::OOC))
                        sendServerMessage("That player is already [IC & OOC] muted!");
                    else{
                        if (!target->isAccessBlocked(BlockType::IC))
                            target->SetAccessBlock(BlockType::IC, true);
                        if (!target->isAccessBlocked(BlockType::OOC))
                            target->SetAccessBlock(BlockType::OOC, true);
                        sendServerMessage("Muted [IC & OOC] player.");
                        target->sendServerMessage(QString("You were [IC & OOC] muted by a %1. ").arg(isVAuthenticated() ? QString("[VIP] (ID: %1)").arg(clientId()) : "Moderator") + getReprimand());
                    }
                    break;
                default:
                    if (target->isAccessBlocked(BlockType::IC))
                        sendServerMessage("That player is already muted!");
                    else{
                        target->SetAccessBlock(BlockType::IC, true);
                        sendServerMessage("Muted player.");
                        target->sendServerMessage(QString("You were IC muted by a %1. ").arg(isVAuthenticated() ? QString("[VIP] (ID: %1)").arg(clientId()) : "Moderator") + getReprimand());
                    }
                    break;
                }
            }
        }
    }
    else
        sendServerMessage("No client with that ID found.");
}

void AOClient::cmdUnMute(int argc, QStringList argv){
    bool conv_ok = false;
    auto target = server->getClientByID(argv[0].toInt(&conv_ok));
    if (conv_ok && !target.isNull()){
        switch (argc){
        case 1: // both by default..
            if (!target->isAccessBlocked(BlockType::IC) && !target->isAccessBlocked(BlockType::OOC))
                sendServerMessage(target == this ? "You are already freed from been both muted(s)." : "that player already been freed from been both muted(s).");
            else{
                if (target->isAccessBlocked(BlockType::IC))
                    target->SetAccessBlock(BlockType::IC, false);
                if (target->isAccessBlocked(BlockType::OOC))
                    target->SetAccessBlock(BlockType::OOC, false);
                if (target == this)
                    sendServerMessage("You are freed yourself from been some of muted(s).");
                else{
                    sendServerMessage("Release that player from been some of muted(s).");
                    target->sendServerMessage(QString("You were freed from been some of muted(s) by a %1. ").arg(isVAuthenticated() ? QString("[VIP] (ID: %1)").arg(clientId()) : "Moderator") + getReprimand(true));
                }
            }
            break;
        default:
            bool type_ok = false;
            const int l_type = argv[1].toInt(&type_ok);
            if (type_ok){
                const QVector<QPair<BlockType, QString>> defined_type{
                    {BlockType::IC, "In-Character (IC)"},
                    {BlockType::OOC, "Out-Of-Character (OOC)"}
                };
                switch (l_type){
                case 0: case 1:
                    if (!target->isAccessBlocked(defined_type[l_type].first))
                        sendServerMessage(QString(target == this ? "You are already freed from %1 muted." : "That player is already %1 unmuted!").arg(defined_type[l_type].second));
                    else{
                        target->SetAccessBlock(defined_type[l_type].first, false);
                        sendServerMessage(QString("Unmuted %1 player.").arg(defined_type[l_type].second));
                        target->sendServerMessage(QString("You were %1 unmuted by a %1. ").arg(defined_type[l_type].second) + getReprimand(true));
                    }
                    break;
                case 2:
                    if (!target->isAccessBlocked(BlockType::IC) && !target->isAccessBlocked(BlockType::OOC))
                        sendServerMessage("That player is already [IC & OOC] unmuted!");
                    else{
                        if (target->isAccessBlocked(BlockType::IC))
                            target->SetAccessBlock(BlockType::IC, false);
                        if (target->isAccessBlocked(BlockType::OOC))
                            target->SetAccessBlock(BlockType::OOC, false);
                        if (target == this)
                            sendServerMessage("You were freed yourself from been both type of mute(s).");
                        else{
                            sendServerMessage("Unmuted [IC & OOC] player.");
                            target->sendServerMessage(QString("You were [IC & OOC] unmuted by a %1. ").arg(isVAuthenticated() ? QString("[VIP] (ID: %1)").arg(clientId()) : "Moderator") + getReprimand(true));
                        }
                    }
                    break;
                default:
                    if (!target->isAccessBlocked(BlockType::IC) && !target->isAccessBlocked(BlockType::OOC))
                        sendServerMessage(target == this ? "You are already freed from been both muted(s)." : "that player already been freed from been both muted(s).");
                    else{
                        if (target->isAccessBlocked(BlockType::IC))
                            target->SetAccessBlock(BlockType::IC, false);
                        if (target->isAccessBlocked(BlockType::OOC))
                            target->SetAccessBlock(BlockType::OOC, false);
                        if (target == this)
                            sendServerMessage("You are freed yourself from been some of muted(s).");
                        else{
                            sendServerMessage("Release that player from been some of muted(s).");
                            target->sendServerMessage(QString("You were freed from been some of muted(s) by a %1. ").arg(isVAuthenticated() ? QString("[VIP] (ID: %1)").arg(clientId()) : "Moderator") + getReprimand(true));
                        }
                    }
                    break;
                }
            }
        }
    }
    else
        sendServerMessage("No client with that ID found.");
}

void AOClient::cmdBlockWtce(int argc, QStringList argv)
{
    Q_UNUSED(argc);
    
    bool conv_ok = false;
    auto l_target = server->getClientByID(argv[0].toInt(&conv_ok));
    if (!conv_ok)
        sendServerMessage("Invalid user ID.");
    else if (l_target.isNull())
        sendServerMessage("No client with that ID found.");
    else if (l_target->isAccessBlocked(BlockType::WTCE))
        sendServerMessage("That player is already judge blocked!");
    else {
        sendServerMessage("Revoked player's access to judge controls.");
        l_target->sendServerMessage("A moderator revoked your judge controls access. " + getReprimand());
        l_target->SetAccessBlock(BlockType::WTCE, true);
    }
}

void AOClient::cmdUnBlockWtce(int argc, QStringList argv)
{
    Q_UNUSED(argc);
    
    bool conv_ok = false;
    auto l_target = server->getClientByID(argv[0].toInt(&conv_ok));
    if (!conv_ok)
        sendServerMessage("Invalid user ID.");
    else if (l_target.isNull())
        sendServerMessage("No client with that ID found.");
    else if (!l_target->isAccessBlocked(BlockType::WTCE))
        sendServerMessage("That player is not judge blocked!");
    else {
        sendServerMessage("Restored player's access to judge controls.");
        l_target->sendServerMessage("A moderator restored your judge controls access. " + getReprimand(true));
        l_target->SetAccessBlock(BlockType::WTCE, false);
    }
}

void AOClient::cmdAllowBlankposting(int argc, QStringList argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);
    
    auto l_area = server->getAreaById(areaId());
    if (l_area.isNull())
        return;
    
    l_area->toggleBlankposting();
    sendServerMessageArea(QString("%1 has set blankposting in the area to %2.").arg(l_area->name(), l_area->blankpostingAllowed() ? "allowed" : "forbidden"));
}

void AOClient::cmdBanInfo(int argc, QStringList argv)
{
    QString l_lookup_type;
    
    switch (argc){
    case 1:
        l_lookup_type = "banid";
        break;
    case 2:
        if (argv[1].compare("banid", Qt::CaseInsensitive) == 0 || argv[1].compare("ipid", Qt::CaseInsensitive) == 0 || argv[1].compare("hdid", Qt::CaseInsensitive) == 0)
            l_lookup_type = argv[1].toLower();
        else{
            sendServerMessage("Invalid ID type.");
            return;
        }
        break;
    default:
        sendServerMessage("Invalid command.");
        return;
    }
    
    QStringList l_ban_info;
    const QString l_id = argv[0];
    const QList<DBManager::BanInfo> l_bans = server->getDatabaseManager()->getBanInfo(l_lookup_type, l_id);
    if (l_bans.isEmpty())
        sendServerMessage(QString("There is nothing data of [%1] for %2.").arg(l_lookup_type, l_id), "[Ban-Info]");
    else{
        const auto client_type = m_version.type;
        for (const DBManager::BanInfo &l_ban : l_bans){
            const QString ids = QString("=== [%1] ===").arg(QString::number(l_ban.id));
            l_ban_info << ids;
            if (client_type == ClientVersion::NDS){
                l_ban_info << "├─ [M]: " + l_ban.moderator; // don't mind with the police emojis..
                if (l_ban.m_type >= 0)
                    l_ban_info << "├─ [M-TYPE]: " + QStringList({"VIP", "Moderator", "ROOT"})[l_ban.m_type];
                if (isMAuthenticated()){ // [ipid / hdid] only been seen by moderators..
                    if (!l_ban.hdid.isEmpty())
                        l_ban_info << "├─ [HDID]: " + l_ban.ipid;
                    if (!l_ban.ipid.isEmpty())
                        l_ban_info << "├─ [IPID]: " + l_ban.ipid;
                }
                const QDateTime b_current_date = QDateTime::fromSecsSinceEpoch(l_ban.time);
                l_ban_info << "├─ [Until]: " + QString(l_ban.duration == -2 ? "The heat death of the universe" : QString("%1 (%2)").arg(b_current_date.addSecs(l_ban.duration).toString("MM/dd/yyyy, hh:mm"), EpochToString(std::chrono::seconds(b_current_date.secsTo(QDateTime::currentDateTime())), true)));
                l_ban_info << "├─ [BAN-Date]: " + b_current_date.toString("MM/dd/yyyy, hh:mm");
                l_ban_info << "├─ [Reason]: " + l_ban.reason;
            }
            else{
                l_ban_info << "├─ [👮]: " + l_ban.moderator;
                if (l_ban.m_type >= 0)
                    l_ban_info << "├─ [👮|TYPE]: " + QStringList({"VIP", "Moderator", "ROOT"})[l_ban.m_type];
                if (isMAuthenticated()){
                    if (!l_ban.hdid.isEmpty())
                        l_ban_info << "├─ [HDID]: " + l_ban.ipid;
                    if (!l_ban.ipid.isEmpty())
                        l_ban_info << "├─ [IPID]: " + l_ban.ipid;
                }
                const QDateTime b_current_date = QDateTime::fromSecsSinceEpoch(l_ban.time);
                l_ban_info << "├─ [⏳]: " + QString(l_ban.duration == -2 ? "The heat death of the universe" : QString("%1 (%2)").arg(b_current_date.addSecs(l_ban.duration).toString("MM/dd/yyyy, hh:mm"), EpochToString(std::chrono::seconds(b_current_date.secsTo(QDateTime::currentDateTime())), true)));
                l_ban_info << "├─ [📅]: " + b_current_date.toString("MM/dd/yyyy, hh:mm");
                l_ban_info << "├─ [📋]: " + l_ban.reason;
                l_ban_info << QString().fill('=', ids.length());
            }
        }
        sendServerMessage(QString("Data of [%1] for %2:\n%3").arg(l_lookup_type, l_id, l_ban_info.join('\n')), "[Ban Info]");
    }
}

void AOClient::cmdReload(int argc, QStringList argv){
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    if (m_authenticated_type != AuthenticateType::ROOT)
        return; // not even [SUPER] Moderators allowed..

    /* === [Devs notes/Todo] ===
     * Make this a signal when splitting AOClient and Server.
     * ========================
     * (which is i kinda done it..)
     */
    qInfo() << QString("[I][AKASHI]: [%1] %2 is triggered reload server.").arg(QString::number(clientId()), m_moderator_name);
    server->reloadSettings();
}

void AOClient::cmdForceImmediate(int argc, QStringList argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);
    
    auto l_area = server->getAreaById(areaId());
    if (l_area.isNull())
        return;
    
    l_area->toggleImmediate();
    sendServerMessage("Forced immediate text processing in this area is now " + QStringList({"off, on"})[l_area->forceImmediate()]);
}

void AOClient::cmdAllowIniswap(int argc, QStringList argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);
    
    auto l_area = server->getAreaById(areaId());
    if (l_area.isNull())
        return;
    
    l_area->toggleIniswap();
    sendServerMessage("Iniswapping in this area is now " + QStringList({"disallowed.", "allowed."})[l_area->iniswapAllowed()]);
}

void AOClient::cmdPermitSaving(int argc, QStringList argv){
    Q_UNUSED(argc);

    bool id_ok = false;
    auto l_client = server->getClientByID(argv[0].toInt(&id_ok));
    if (id_ok && !l_client.isNull()){
        l_client->m_testimony_saving = true;
        sendServerMessage("Testimony saving has been enabled for client " + QString::number(l_client->clientId()));
    }
    else
        sendServerMessage("Invalid ID.");
}

void AOClient::cmdUpdateBan(int argc, QStringList argv){
    Q_UNUSED(argc)
    if (isVAuthenticated()) // if [VIP] has "ban" or "super" perms..
        sendServerMessage("This command only for moderators.");
    else{
        bool id_pass = false;
        const int ID = argv[0].toInt(&id_pass);
        
        if (id_pass){
            QVariant l_updated_info;
            bool isInt_type = false;
            const int int_type = argv[1].toInt(&isInt_type);
            QString l_field_type;
            
            if (isInt_type){
                switch (int_type) {
                default:
                    sendServerMessage("Invalid int type of update type, must between 1 or 2.");
                    return;
                case 2: // [reason]..
                    l_updated_info = QVariant::fromValue(argv.mid(2).join(" "));
                    l_field_type = "reason";
                    break;
                case 1: // [duration]..
                    const long long l_durationsc = (argv[1].compare("perma", Qt::CaseInsensitive) || argv[1].compare("forever", Qt::CaseInsensitive)) ? -2 : CalendarParse(argv[2]);
                    
                    if (l_durationsc != -1)
                        l_updated_info = QVariant::fromValue(l_durationsc);
                    else{
                        sendServerMessage("Invalid time format. Format example: 1h30m");
                        return;
                    }
                    l_field_type = "duration";
                    break;
                }
            }
            else if (argv[1].compare("duration", Qt::CaseInsensitive) == 0 || argv[1].compare("reason", Qt::CaseInsensitive) == 0){
                if (argv[1].compare("duration", Qt::CaseInsensitive) == 0){
                    const long long l_durationsc = (argv[1].compare("perma", Qt::CaseInsensitive) || argv[1].compare("forever", Qt::CaseInsensitive)) ? -2 : CalendarParse(argv[2]);
                    
                    if (l_durationsc != -1)
                        l_updated_info = QVariant::fromValue(l_durationsc);
                    else{
                        sendServerMessage("Invalid time format. Format example: 1h30m");
                        return;
                    }
                }
                else 
                    l_updated_info = QVariant::fromValue(argv.mid(2).join(" "));
                l_field_type = argv[1].toLower();
            }
            else{
                sendServerMessage("Invalid update type, must between [duration, reason] or [1, 2] int type.");
                return;
            }
            
            sendServerMessage(server->getDatabaseManager()->updateBan(ID, l_field_type, l_updated_info) ? "Ban updated." : "There was an error updating the ban. Please confirm the ban ID is valid.");
        }
        else
            sendServerMessage("Invalid ban ID.");
    }
}

void AOClient::cmdNotice(int argc, QStringList argv)
{
    Q_UNUSED(argc);
    
    sendNotice(argv.join(" "));
}
void AOClient::cmdNoticeGlobal(int argc, QStringList argv)
{
    Q_UNUSED(argc);
    
    sendNotice(argv.join(" "), true);
}

void AOClient::cmdClearCM(int argc, QStringList argv)
{
    Q_UNUSED(argc);

    auto l_area = server->getAreaById(areaId());
    if (l_area.isNull())
        return;

    // Parse the optional "all areas" parameter
    bool clearAllAreas = false;

    if (argv.size() >= 1) {
        // Check for numeric 0/1
        bool ok;
        int val = argv[0].toInt(&ok);
        bool isNumeric = ok && (val == 0 || val == 1);

        // Check for string "true"/"false"
        bool isTrueStr  = (argv[0].compare("true", Qt::CaseInsensitive) == 0);
        bool isFalseStr = (argv[0].compare("false", Qt::CaseInsensitive) == 0);

        if (!isNumeric && !isTrueStr && !isFalseStr) {
            sendServerMessage("Invalid param, must be [0, 1, true, false].");
            return;
        }

        // Determine if we're clearing all areas
        clearAllAreas = isNumeric ? (val == 1) : isTrueStr;

        // Permission check for clearing all areas
        if (clearAllAreas && !checkPermission(ACLRole::UNCM)) {
            sendServerMessage("You do not have permission to clear CMs from all areas. Use this command without parameters to clear only this area.");
            return;
        }
    }

    if (clearAllAreas) {
        // Clear CMs from ALL areas
        int areasCleared = 0;
        int cmsRemoved = 0;
        const auto areas = server->getAreas();

        for (auto area : areas) {
            if (area.isNull() || area->owners().isEmpty())
                continue;

            cmsRemoved += area->owners().size();
            for (int ownerId : area->owners()) {
                area->removeOwner(ownerId);
            }
            areasCleared++;
        }

        if (areasCleared == 0) {
            sendServerMessage("There are no CMs in any areas.");
        } else if (areasCleared == areas.size()) {
            sendServerMessage(cmsRemoved > 0
                ? "Successfully removed all CMs from every area."
                : "There were no CMs to remove.");
        } else {
            sendServerMessage(QString("Removed %1 CMs from %2 / %3 areas.")
                .arg(QString::number(cmsRemoved),
                     QString::number(areasCleared),
                     QString::number(areas.size())));
        }
        arup(ARUPType::CM, true);
    } else {
        // Clear CMs from THIS area only
        if (l_area->owners().isEmpty()) {
            sendServerMessage("There are no CMs in this area.");
        } else {
            int count = l_area->owners().size();
            for (int id : l_area->owners()) {
                l_area->removeOwner(id);
            }
            arup(ARUPType::CM, true);
            sendServerMessage(QString("Removed %1 CM(s) from this area.").arg(count));
        }
    }

    arup(ARUPType::LOCKED, true);
}

void AOClient::cmdKickOther(int argc, QStringList argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);
    
    int l_kick_counter = 0;
    
    QList<QPointer<AOClient>> l_targets = (server->getClientsByIpid(m_ipid) += server->getClientsByHwid(m_hwid));
    l_targets.removeAll(this);
    
    // The list is unique, we can only have on instance of the current client.
    for (auto l_client : l_targets){
        if (l_client.isNull())
            continue;
        l_client->m_socket->close();
        ++l_kick_counter;
    }
    sendServerMessage("Kicked " + QString::number(l_kick_counter) + " multiclients from the server.");
}

static QString lockdownStatusMessage(Server *server){
    if (!server->isLockdownState())
        return "The lockdown is currently disabled..";
    if (server->lockdown_timeout && server->lockdown_timeout->isActive() && server->lockdown_timeout->remainingTime() > 1){
        auto remain = server->lockdown_timeout->remainingTimeAsDuration();
        return QString("The lockdown is active and will last %1..").arg(AOClient::EpochToString(remain));
    }
    return "The lockdown is enabled..";
}
void AOClient::cmdlockdown(int argc, QStringList argv){
    if (argc == 0)
        sendServerMessage(lockdownStatusMessage(server));
    else{
        const bool isStart = (argv[0].compare("start", Qt::CaseInsensitive) == 0 || argv[0].compare("on", Qt::CaseInsensitive) == 0);
        const bool isStop  = (argv[0].compare("stop", Qt::CaseInsensitive) == 0 || argv[0].compare("off", Qt::CaseInsensitive) == 0);
        if (isStart || isStop){
            if (server->isLockdownState() == (isStart ? true : false))
                sendServerMessage(lockdownStatusMessage(server));
            else{
                server->setlockdownstate(isStart);
                server->broadcast(PacketCT::CreateMessageS(QString("%1 is %2..").arg(AOClient::NameWId(this), isStart ? "initializing the lockdown" : "releasing the lockdown")), AOClient::AuthenticateType::MODERATOR);
            }
        }
        else if (argv[0].compare("add", Qt::CaseInsensitive) == 0 || argv[0].compare("remove", Qt::CaseInsensitive) == 0){
            const bool isAdd = (argv[0].compare("add", Qt::CaseInsensitive) == 0);
            if (argc < 2)
                sendServerMessage(QString("You must provide a hashid to %1 the lockdown whitelist..").arg(isAdd ? "add to" : "remove from"), "[" + ConfigManager::serverTag() + "][LOCKDOWN]");
            else {
                const QString hashid = argv[1];
                if (hashid.size() != 12)
                    sendServerMessage("The hashid must be exactly 12 characters long.", "[" + ConfigManager::serverTag() + "][LOCKDOWN]");
                else {
                    const bool success = server->LockdownRegister(hashid.toUtf8(), isAdd);
                    if (success) sendServerMessage(QString("Successfully %1 %2 %3 the lockdown whitelist.").arg(isAdd ? "added" : "removed", hashid, isAdd ? "to" : "from"), "[" + ConfigManager::serverTag() + "][LOCKDOWN]");
                    else sendServerMessage(QString("Hashid %1 is already %2 the lockdown whitelist.").arg(hashid, isAdd ? "in" : "not in"), "[" + ConfigManager::serverTag() + "][LOCKDOWN]");
                }
            }
        }
        else{
            const long long durationMs = AOClient::CalendarParse(argv[0], true);
            if (durationMs >= 1) {
                QString msg;
                const auto prevRemaining = server->lockdown_timeout->remainingTimeAsDuration();
                if (prevRemaining.count() > 1)
                    msg = QString("%1 changes the lockdown duration from %2 to %3.").arg(AOClient::NameWId(this), AOClient::EpochToString(prevRemaining), AOClient::EpochToString(std::chrono::milliseconds(durationMs)));
                else
                    msg = QString("%1 initializes the lockdown for %3..").arg(AOClient::NameWId(this), AOClient::EpochToString(std::chrono::milliseconds(durationMs)));
                server->broadcast(PacketCT::CreateMessageS(msg), AOClient::AuthenticateType::MODERATOR);
                server->startlockdown(durationMs);
            }
            else
                sendServerMessage("Invalid time parameter.. try /lockdown [start|stop|on|off|add|remove|time].");
        }
    }
}
void AOClient::cmdlockdownlist(int argc, QStringList argv) {
    int page = 1;
    if (argc > 0){
        bool ok = false;
        page = qMax(1, argv[0].toInt(&ok));
        if (!ok){
            sendServerMessage("Page must be a number.");
            return;
        }
    }

    const QVector<QByteArray> whitelist = server->Getwhitelistclient();
    const int totalPages = (whitelist.size() + 9) / 10;
    if (whitelist.isEmpty())
        sendServerMessage("The lockdown whitelist is empty.");
    else if (page > totalPages)
        sendServerMessage("Page number is out of range.");
    else {
        QString message = "=== [Lockdown Whitelist] ===\n";
        const int startIndex = (page - 1) * 10;
        const int endIndex = qMin(startIndex + 10, whitelist.size());
        for (int i = startIndex; i < endIndex; ++i)
            message += QString("[%1] %2\n").arg(i + 1).arg(QString::fromUtf8(whitelist.at(i)));
        message += QString("=== [Page %1 / %2] ===").arg(page).arg(totalPages);
        sendServerMessage(message);
    }
}
