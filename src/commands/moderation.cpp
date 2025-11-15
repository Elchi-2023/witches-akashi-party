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
#include "config_manager.h"
#include "db_manager.h"
#include "server.h"

// This file is for commands under the moderation category in aoclient.h
// Be sure to register the command in the header before adding it here!

void AOClient::cmdBan(int argc, QStringList argv)
{
    QString l_args_str = argv[2];
    if (argc > 3) {
        for (int i = 3; i < argc; i++)
            l_args_str += " " + argv[i];
    }

    DBManager::BanInfo l_ban;

    long long l_duration_seconds = 0;
    if (argv[1] == "perma")
        l_duration_seconds = -2;
    else
        l_duration_seconds = parseTime(argv[1]);

    if (l_duration_seconds == -1) {
        sendServerMessage("Invalid time format. Format example: 1h30m");
        return;
    }

    bool isclientID;
    int clientID = argv[0].toInt(&isclientID);
    l_ban.duration = l_duration_seconds;
    if (isclientID && server->getClientByID(clientID) != nullptr)
        l_ban.ipid = server->getClientByID(clientID)->m_ipid;
    else
        l_ban.ipid = argv[0];
    l_ban.reason = l_args_str;
    l_ban.time = QDateTime::currentDateTime().toSecsSinceEpoch();

    switch (ConfigManager::authType()) {
    case DataTypes::AuthType::SIMPLE:
        l_ban.moderator = "moderator";
        break;
    case DataTypes::AuthType::ADVANCED:
        l_ban.moderator = m_moderator_name;
        break;
    }

    const QList<AOClient *> l_targets = server->getClientsByIpid(l_ban.ipid);
    if (l_targets.isEmpty()){ /* We're banning someone not connected. */
        server->getDatabaseManager()->addBan(l_ban);
        sendServerMessage("Banned " + l_ban.ipid + " for reason: " + l_ban.reason);
    }
    else if (l_targets.size() == 1){
        auto target = l_targets.first();
        l_ban.ip = target->m_remote_ip;
        l_ban.hdid = target->m_hwid;
        server->getDatabaseManager()->addBan(l_ban);
        sendServerMessage("Banned user with ipid " + l_ban.ipid + " for reason: " + l_ban.reason);

        QDateTime ban_until = QDateTime::fromSecsSinceEpoch(l_ban.time);
        const QString l_ban_duration = l_ban.duration >= 0 ? ban_until.addSecs(l_ban.duration).toString("MM/dd/yyyy, hh:mm") : "Permanently.";

        const int l_ban_id = server->getDatabaseManager()->getBanID(l_ban.ip);
        target->m_is_multiclient = false;
        target->m_disconnect_reason = Disconnected::BAN;
        target->sendPacket("KB", {QStringList({l_ban.reason, "ID: " + QString::number(l_ban_id), "Until: " + l_ban_duration}).join('\n')});
        target->m_socket->close();

        emit logBan(l_ban.moderator, l_ban.ipid, l_ban_duration, l_ban.reason);
        if (ConfigManager::discordBanWebhookEnabled()){
            QStringList Name{"[" + QString::number(clientId()) + "]", "(" + l_ban.moderator + ")"};
            if (!name().isEmpty())
                Name.insert(1, name());
            const QString l_ban_duration_discord_format = l_ban.duration >= 0 ? QString("<t:%1:R>").arg(ban_until.addSecs(l_ban.duration).toSecsSinceEpoch()) : "Permanently";
            emit server->banWebhookRequest(l_ban.ipid, Name.join(' '), l_ban_duration_discord_format, l_ban.reason, l_ban_id, l_targets.size());
        }
    }
    else{
        auto target = l_targets.first();
        l_ban.ip = target->m_remote_ip;
        l_ban.hdid = target->m_hwid;
        server->getDatabaseManager()->addBan(l_ban);

        QDateTime ban_until = QDateTime::fromSecsSinceEpoch(l_ban.time);
        const int l_ban_id = server->getDatabaseManager()->getBanID(l_ban.ip);
        int l_kick_counter = 0;

        for (auto *target : l_targets){
            const QString l_ban_duration = l_ban.duration >= 0 ? ban_until.addSecs(l_ban.duration).toString("MM/dd/yyyy, hh:mm") : "Permanently.";
            target->m_is_multiclient = l_kick_counter != 0;
            target->m_disconnect_reason = Disconnected::BAN;
            target->sendPacket("KB", {QStringList({l_ban.reason, "ID: " + QString::number(l_ban_id), "Until: " + l_ban_duration}).join('\n')});
            target->m_socket->close();
            l_kick_counter += 1;
        }

        emit logBan(l_ban.moderator, l_ban.ipid, l_ban.duration >= 0 ? ban_until.addSecs(l_ban.duration).toString("MM/dd/yyyy, hh:mm") : "Permanently.", l_ban.reason);
        if (ConfigManager::discordBanWebhookEnabled()){
            QStringList Name{"[" + QString::number(clientId()) + "]", "(" + l_ban.moderator + ")"};
            if (!name().isEmpty())
                Name.insert(1, name());
            const QString l_ban_duration_discord_format = l_ban.duration >= 0 ? QString("<t:%1:R>").arg(ban_until.addSecs(l_ban.duration).toSecsSinceEpoch()) : "Permanently";
            emit server->banWebhookRequest(l_ban.ipid, Name.join(' '), l_ban_duration_discord_format, l_ban.reason, l_ban_id, l_targets.size());
        }
        sendServerMessage(QString("Banned user with ipid [%1] for reason: [%2] and kicked %3 clients with matching ipids.").arg(l_ban.ipid, l_ban.reason, QString::number(l_targets.size())));
    }
}

void AOClient::cmdKick(int argc, QStringList argv)
{
    QString l_target_ipid = argv[0];
    QString l_reason = argv[1];
    int l_kick_counter = 0;

    if (argc > 2) {
        for (int i = 2; i < argv.length(); i++) {
            l_reason += " " + argv[i];
        }
    }
    bool isclientID;
    int clientID = l_target_ipid.toInt(&isclientID);
    if (isclientID && server->getClientByID(clientID) != nullptr)
        l_target_ipid = server->getClientByID(clientID)->m_ipid;

    const QList<AOClient *> l_targets = server->getClientsByIpid(l_target_ipid);
    for (int index = 0; index < l_targets.size(); ++index){
        l_targets[index]->m_is_multiclient = index != 0;
        l_targets[index]->m_disconnect_reason = Disconnected::KICK;
        l_targets[index]->sendPacket("KK", {l_reason});
        l_targets[index]->m_socket->close();
        l_kick_counter = index +1;
    }

    switch (l_kick_counter){
        case 0:
            sendServerMessage("User with ipid not found!");
            break;
        default:
            if (m_authenticated){
                if (ConfigManager::authType() == DataTypes::AuthType::ADVANCED)
                    emit logKick(m_moderator_name, l_target_ipid, l_reason);
                else
                    emit logKick("Moderator", l_target_ipid, l_reason);
                sendServerMessage("Kicked " + QString::number(l_kick_counter) + " client(s) with ipid " + l_target_ipid + " for reason: " + l_reason);
            }
            else if (m_vip_authenticated){
                if (ConfigManager::authType() == DataTypes::AuthType::ADVANCED)
                    emit logKick("[VIP] " + m_moderator_name, l_target_ipid, l_reason);
                else
                    emit logKick("[VIP]", l_target_ipid, l_reason);
                sendServerMessage("Kicked " + QString::number(l_kick_counter) + " client(s) for reason: " + l_reason);
            }
            break;
    }
}

void AOClient::cmdMods(int argc, QStringList argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    QMap<int, QStringList> EntriesMap; /* why use qmap?.. because we needs area_id for get areas name */
    for (AOClient *client : server->getClients()){
        if (client->m_authenticated){
            QString user_entry = QString("· [%1] %2").arg(QString::number(client->clientId()), client->character().isEmpty() ? "[Spectator]" : client->character());
            if (!client->name().trimmed().isEmpty() && m_authenticated) /* moderator can see ooc name when other moderator had ooc name on it */
                user_entry.append(" (" + client->name().trimmed() + ")");
            if (EntriesMap.contains(client->areaId())) /* if area_id was on enteriesmap, we adds another entry */
                EntriesMap[client->areaId()].append(user_entry);
            else /* otherwise, we needs captures area_id and entry */
                EntriesMap.insert(client->areaId(), {user_entry});
        }
    } /* if you wander about where's akashi profiles stuff?.. mint don't want that stuff (unless if he change mind) */

    /* kfo/tsu-like behavior */
    QStringList entry("=== Moderator ===");
    int entry_count = 0;

    for (auto EMap = EntriesMap.begin(); EMap != EntriesMap.end(); ++EMap){
        entry.append(QString("=== %1 ===\n%2").arg(server->getAreaById(EMap.key())->name(), EMap.value().join('\n')));
        entry_count += EMap.value().size();
    }
    entry.append(QString("=== Total online : %1 ===").arg(QString::number(entry_count)));

    sendServerMessage(entry.join('\n'));
}

void AOClient::cmdCurses(int argc, QStringList argv){
    if (argc <= 0)
        return;
    bool vaild_id = false;
    int target_id = argv[0].toInt(&vaild_id);

    if (!vaild_id){
        sendServerMessage("Invalid user ID.");
        return;
    }

    auto target_client = server->getClientByID(target_id);

    if (target_client == nullptr){
        sendServerMessage("No client with that ID found.");
        return;
    }

    const QStringList m_type{"disemvoweled", "shaked", "medievaled", "gimped"};
    switch (argc){
    case 1:
        if (target_client->m_is_disemvoweled)
            sendServerMessage("That target are already been curses (disemvoweled).");
        else{
            target_client->m_is_disemvoweled = true;
            sendServerMessage("You gives target an curses (disemvoweled).");
            target_client->sendServerMessage("You been curses (disemvoweled) by Moderator! " + getReprimand(false));
        }
        break;
    case 2: default:
        bool vaild_type = false;
        const int _type = argv[1].toInt(&vaild_type);
        if (!vaild_type){
            sendServerMessage("Invaild Type.");
            return;
        }

        const QList<bool*> target_state{&target_client->m_is_disemvoweled, &target_client->m_is_shaken, &target_client->m_is_medieval, &target_client->m_is_gimped};
        switch (_type){
        case -1:
            if (target_client->m_is_disemvoweled && target_client->m_is_shaken && target_client->m_is_medieval && target_client->m_is_gimped)
                sendServerMessage("That target are already been *TRUE* curses.");
            else{
                for (auto state : target_state){
                    if (!*state)
                        *state = true;
                }
                sendServerMessage("You gives target an *TRUE* curses.");
                target_client->sendServerMessage("You been *TRUE* curses by Moderator! " + getReprimand(false));
            }
            break;
        case 0: case 1: case 2: case 3:
            if (*target_state[_type])
                sendServerMessage(QString("That target are already been curses (%1)").arg(m_type[_type]));
            else{
                *target_state[_type] = true;
                sendServerMessage(QString("You gives target an curses (%1).").arg(m_type[_type]));
                target_client->sendServerMessage(QString("You been curses (%1) by Moderator! ").arg(m_type[_type]) + getReprimand(false));
            }
            break;
        default:
            if (target_client->m_is_disemvoweled)
                sendServerMessage("That target are already been curses (disemvoweled).");
            else{
                target_client->m_is_disemvoweled = true;
                sendServerMessage("You gives target an curses (disemvoweled).");
                target_client->sendServerMessage("You been curses (disemvoweled) by Moderator! " + getReprimand(false));
            }
        }
        break;
    }
}

void AOClient::cmdUnCurses(int argc, QStringList argv){
    if (argc <= 0)
        return;
    bool vaild_id = false;
    int target_id = argv[0].toInt(&vaild_id);

    if (!vaild_id){
        sendServerMessage("Invalid user ID.");
        return;
    }

    auto target_client = server->getClientByID(target_id);

    if (target_client == nullptr){
        sendServerMessage("No client with that ID found.");
        return;
    }

    const QStringList m_type{"disemvoweled", "shaked", "medievaled", "gimped"};
    switch (argc){
    case 1:
        if (!target_client->m_is_disemvoweled)
            sendServerMessage("That target are already been freed from curses (disemvoweled).");
        else{
            target_client->m_is_disemvoweled = false;
            sendServerMessage("You freed target from an curses (disemvoweled).");
            target_client->sendServerMessage("You been freed from an curses (disemvoweled) by Moderator! " + getReprimand(true));
        }
        break;
    case 2: default:
        bool vaild_type = false;
        const int _type = argv[1].toInt(&vaild_type);
        if (!vaild_type){
            sendServerMessage("Invaild Type.");
            return;
        }

        const QList<bool*> target_state{&target_client->m_is_disemvoweled, &target_client->m_is_shaken, &target_client->m_is_medieval, &target_client->m_is_gimped};
        switch (_type){
        case -1:
            if (!target_client->m_is_disemvoweled && !target_client->m_is_shaken && !target_client->m_is_medieval && !target_client->m_is_gimped)
                sendServerMessage("That target are already freed from an *TRUE* curses.");
            else{
                for (auto state : target_state){
                    if (*state)
                        *state = false;
                }
                sendServerMessage("You freed target from an *TRUE* curses.");
                target_client->sendServerMessage("You been freed from an *TRUE* curses by Moderator! " + getReprimand(true));
            }
            break;
        case 0: case 1: case 2: case 3:
            if (!*target_state[_type])
                sendServerMessage(QString("That target are already freed from an (%1)").arg(m_type[_type]));
            else{
                *target_state[_type] = false;
                sendServerMessage(QString("You freed target from an curses (%1).").arg(m_type[_type]));
                target_client->sendServerMessage(QString("You been freed from an curses (%1) by Moderator! ").arg(m_type[_type]) + getReprimand(true));
            }
            break;
        default:
            if (!target_client->m_is_disemvoweled)
                sendServerMessage("That target are already been freed from curses (disemvoweled).");
            else{
                target_client->m_is_disemvoweled = false;
                sendServerMessage("You freed target from an curses (disemvoweled).");
                target_client->sendServerMessage("You been freed from an curses (disemvoweled) by Moderator! " + getReprimand(true));
            }
        }
        break;
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

void AOClient::cmdHelp(int argc, QStringList argv)
{
    if (argc > 1) {
        sendServerMessage("Too many arguments. Please only use the command name.");
        return;
    }

    QString l_command_name = argv[0];
    ConfigManager::help l_command_info = ConfigManager::commandHelp(l_command_name);
    if (l_command_info.usage.isEmpty() || l_command_info.text.isEmpty()) // my picoseconds :(
        sendServerMessage("Unable to find the command " + l_command_name + ".");
    else
        sendServerMessage("==Help==\n" + l_command_info.usage + "\n" + l_command_info.text);
}

void AOClient::cmdMOTD(int argc, QStringList argv)
{
    Q_UNUSED(argc)
    Q_UNUSED(argv)

    sendServerMessage("=== MOTD ===\r\n" + ConfigManager::motd() + "\r\n=============");
}

void AOClient::cmdSetMOTD(int argc, QStringList argv)
{
    Q_UNUSED(argc)

    QString l_MOTD = argv.join(" ");
    ConfigManager::setMotd(l_MOTD);
    sendServerMessage("MOTD has been changed.");
}

void AOClient::cmdBans(int argc, QStringList argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    QStringList l_recent_bans;
    l_recent_bans << "Last 5 bans:";
    l_recent_bans << "-----";
    const QList<DBManager::BanInfo> l_bans_list = server->getDatabaseManager()->getRecentBans();
    for (const DBManager::BanInfo &l_ban : l_bans_list) {
        QString l_banned_until;
        if (l_ban.duration == -2)
            l_banned_until = "The heat death of the universe";
        else
            l_banned_until = QDateTime::fromSecsSinceEpoch(l_ban.time).addSecs(l_ban.duration).toString("MM/dd/yyyy, hh:mm");
        l_recent_bans << "Ban ID: " + QString::number(l_ban.id);
        l_recent_bans << "Affected IPID: " + l_ban.ipid;
        l_recent_bans << "Affected HDID: " + l_ban.hdid;
        l_recent_bans << "Reason for ban: " + l_ban.reason;
        l_recent_bans << "Date of ban: " + QDateTime::fromSecsSinceEpoch(l_ban.time).toString("MM/dd/yyyy, hh:mm");
        l_recent_bans << "Ban lasts until: " + l_banned_until;
        l_recent_bans << "Moderator: " + l_ban.moderator;
        l_recent_bans << "-----";
    }
    sendServerMessage(l_recent_bans.join("\n"));
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
                QStringList m_name({"[" + QString::number(clientId()) + "]", "(" + m_moderator_name + ")"});
                if (name().toLower() != m_moderator_name.toLower())
                    m_name.insert(1, name());
                Q_EMIT server->UnbanWebhookRequested(current.ipid, {current.moderator, m_name.join(' ')}, current.id, current.duration, QDateTime::fromSecsSinceEpoch(current.time), current.reason);
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

    sendPacket("CT", {"The akashi dev team", "Thank you for using akashi! Made with love by scatterflower, with help from in1tiate, Salanto, and mangosarentliterature. akashi " + QCoreApplication::applicationVersion() + ". For documentation and reporting issues, see the source: https://github.com/AttorneyOnline/akashi"});
}

void AOClient::cmdMute(int argc, QStringList argv)
{
    Q_UNUSED(argc);
    bool conv_ok = false;
    int l_uid = argv[0].toInt(&conv_ok);
    if (!conv_ok) {
        sendServerMessage("Invalid user ID.");
        return;
    }

    AOClient *target = server->getClientByID(l_uid);

    if (argv.isEmpty())
        return;
    else if (argv.size() == 1){
        if (target == nullptr) {
            sendServerMessage("No client with that ID found.");
            return;
        }

        if (target->m_is_muted)
            sendServerMessage("That player is already muted!");
        else {
            sendServerMessage("Muted player.");
            target->sendServerMessage("You were muted by a moderator. " + getReprimand());
            target->m_is_muted = true;
        }
    }
    else{
        bool type_ok = false;
        int l_type = argv[1].toInt(&type_ok);
        if (!type_ok){
            sendServerMessage("Invaild Type.");
            return;
        }

        switch (l_type){
        case 0: default:
            if (target->m_is_muted)
                sendServerMessage("That player is already IC muted!");
            else{
                sendServerMessage("Muted IC player.");
                target->sendServerMessage("You were IC muted by a moderator. " + getReprimand());
                target->m_is_muted = true;
            }
            break;
        case 1:
            if (target->m_is_ooc_muted)
                sendServerMessage("That player is already OOC muted!");
            else{
                sendServerMessage("OOC muted player.");
                target->sendServerMessage("You were OOC muted by a moderator. " + getReprimand());
                target->m_is_ooc_muted = true;
            }
            break;
        case 2:
            if (target->m_is_muted && target->m_is_ooc_muted)
                sendServerMessage("That player is already [IC & OOC] muted!");
            else{
                if (!target->m_is_muted)
                    target->m_is_muted = true;
                if (!target->m_is_ooc_muted)
                    target->m_is_ooc_muted = true;
                sendServerMessage("[IC & OOC] muted player.");
                target->sendServerMessage("You were [IC & OOC] muted by a moderator. " + getReprimand());
            }
            break;
        }
    }
}

void AOClient::cmdUnMute(int argc, QStringList argv)
{
    Q_UNUSED(argc)
    bool conv_ok = false;
    int l_uid = argv[0].toInt(&conv_ok);
    if (!conv_ok) {
        sendServerMessage("Invalid user ID.");
        return;
    }

    AOClient *target = server->getClientByID(l_uid);

    if (argv.isEmpty())
        return;
    else if (argv.size() == 1){
        if (target == nullptr) {
            sendServerMessage("No client with that ID found.");
            return;
        }

        if (!target->m_is_muted)
            sendServerMessage("That player is already unmuted!");
        else {
            sendServerMessage("Unmuted player.");
            target->sendServerMessage("You were unmuted by a moderator. " + getReprimand());
            target->m_is_muted = false;
        }
    }
    else{
        bool type_ok = false;
        int l_type = argv[1].toInt(&type_ok);
        if (!type_ok){
            sendServerMessage("Invaild Type.");
            return;
        }

        switch (l_type){
        case 0: default:
            if (!target->m_is_muted)
                sendServerMessage("That player is already IC unmuted!");
            else{
                sendServerMessage("unmuted IC player.");
                target->sendServerMessage("You were IC unmuted by a moderator. " + getReprimand());
                target->m_is_muted = false;
            }
            break;
        case 1:
            if (!target->m_is_ooc_muted)
                sendServerMessage("That player is already OOC muted!");
            else{
                sendServerMessage("OOC unmuted player.");
                target->sendServerMessage("You were OOC unmuted by a moderator. " + getReprimand());
                target->m_is_ooc_muted = false;
            }
            break;
        case 2:
            if (!target->m_is_muted && !target->m_is_ooc_muted)
                sendServerMessage("That player is already [IC & OOC] unmuted!");
            else{
                if (target->m_is_muted)
                    target->m_is_muted = false;
                if (target->m_is_ooc_muted)
                    target->m_is_ooc_muted = false;
                sendServerMessage("[IC & OOC] unmuted player.");
                target->sendServerMessage("You were [IC & OOC] unmuted by a moderator. " + getReprimand());
            }
            break;
        }
    }
}

void AOClient::cmdOocMute(int argc, QStringList argv){
    Q_UNUSED(argc)
    
    bool conv_ok = false;
    int l_uid = argv[0].toInt(&conv_ok);
    if (!conv_ok) {
        sendServerMessage("Invalid user ID.");
        return;
    }

    AOClient *target = server->getClientByID(l_uid);
    
    if (target == nullptr) {
        sendServerMessage("No client with that ID found.");
        return;
    }

    if (target->m_is_ooc_muted)
        sendServerMessage("That player is already OOC muted!");
    else{
        sendServerMessage("OOC muted player.");
        target->sendServerMessage("You were OOC muted by a moderator. " + getReprimand());
        target->m_is_ooc_muted = true;
    }
    
}

void AOClient::cmdOocUnMute(int argc, QStringList argv){
    Q_UNUSED(argc)
    
    bool conv_ok = false;
    int l_uid = argv[0].toInt(&conv_ok);
    if (!conv_ok) {
        sendServerMessage("Invalid user ID.");
        return;
    }
    
    AOClient *target = server->getClientByID(l_uid);
    
    if (!target->m_is_ooc_muted)
        sendServerMessage("That player is already OOC unmuted!");
    else{
        sendServerMessage("OOC unmuted player.");
        target->sendServerMessage("You were OOC unmuted by a moderator. " + getReprimand());
        target->m_is_ooc_muted = false;
    }
}

void AOClient::cmdBlockWtce(int argc, QStringList argv)
{
    Q_UNUSED(argc);

    bool conv_ok = false;
    int l_uid = argv[0].toInt(&conv_ok);
    if (!conv_ok) {
        sendServerMessage("Invalid user ID.");
        return;
    }

    AOClient *l_target = server->getClientByID(l_uid);

    if (l_target == nullptr) {
        sendServerMessage("No client with that ID found.");
        return;
    }

    if (l_target->m_is_wtce_blocked)
        sendServerMessage("That player is already judge blocked!");
    else {
        sendServerMessage("Revoked player's access to judge controls.");
        l_target->sendServerMessage("A moderator revoked your judge controls access. " + getReprimand());
    }
    l_target->m_is_wtce_blocked = true;
}

void AOClient::cmdUnBlockWtce(int argc, QStringList argv)
{
    Q_UNUSED(argc);

    bool conv_ok = false;
    int l_uid = argv[0].toInt(&conv_ok);
    if (!conv_ok) {
        sendServerMessage("Invalid user ID.");
        return;
    }

    AOClient *l_target = server->getClientByID(l_uid);

    if (l_target == nullptr) {
        sendServerMessage("No client with that ID found.");
        return;
    }

    if (!l_target->m_is_wtce_blocked)
        sendServerMessage("That player is not judge blocked!");
    else {
        sendServerMessage("Restored player's access to judge controls.");
        l_target->sendServerMessage("A moderator restored your judge controls access. " + getReprimand(true));
    }
    l_target->m_is_wtce_blocked = false;
}

void AOClient::cmdAllowBlankposting(int argc, QStringList argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    QString l_sender_name = name();
    AreaData *l_area = server->getAreaById(areaId());
    l_area->toggleBlankposting();
    if (l_area->blankpostingAllowed() == false) {
        sendServerMessageArea(l_sender_name + " has set blankposting in the area to forbidden.");
    }
    else {
        sendServerMessageArea(l_sender_name + " has set blankposting in the area to allowed.");
    }
}

void AOClient::cmdBanInfo(int argc, QStringList argv)
{
    QStringList l_ban_info;
    l_ban_info << ("Ban Info for " + argv[0]);
    l_ban_info << "-----";
    QString l_lookup_type;

    if (argc == 1) {
        l_lookup_type = "banid";
    }
    else if (argc == 2) {
        l_lookup_type = argv[1];
        if (!((l_lookup_type == "banid") || (l_lookup_type == "ipid") || (l_lookup_type == "hdid"))) {
            sendServerMessage("Invalid ID type.");
            return;
        }
    }
    else {
        sendServerMessage("Invalid command.");
        return;
    }
    QString l_id = argv[0];
    const QList<DBManager::BanInfo> l_bans = server->getDatabaseManager()->getBanInfo(l_lookup_type, l_id);
    for (const DBManager::BanInfo &l_ban : l_bans) {
        QString l_banned_until;
        if (l_ban.duration == -2)
            l_banned_until = "The heat death of the universe";
        else
            l_banned_until = QDateTime::fromSecsSinceEpoch(l_ban.time).addSecs(l_ban.duration).toString("MM/dd/yyyy, hh:mm");
        l_ban_info << "Ban ID: " + QString::number(l_ban.id);
        l_ban_info << "Affected IPID: " + l_ban.ipid;
        l_ban_info << "Affected HDID: " + l_ban.hdid;
        l_ban_info << "Reason for ban: " + l_ban.reason;
        l_ban_info << "Date of ban: " + QDateTime::fromSecsSinceEpoch(l_ban.time).toString("MM/dd/yyyy, hh:mm");
        l_ban_info << "Ban lasts until: " + l_banned_until;
        l_ban_info << "Moderator: " + l_ban.moderator;
        l_ban_info << "-----";
    }
    sendServerMessage(l_ban_info.join("\n"));
}

void AOClient::cmdReload(int argc, QStringList argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    // Todo: Make this a signal when splitting AOClient and Server.
    server->reloadSettings();
    sendServerMessage("Reloaded configurations");
}

void AOClient::cmdForceImmediate(int argc, QStringList argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    AreaData *l_area = server->getAreaById(areaId());
    l_area->toggleImmediate();
    QString l_state = l_area->forceImmediate() ? "on." : "off.";
    sendServerMessage("Forced immediate text processing in this area is now " + l_state);
}

void AOClient::cmdAllowIniswap(int argc, QStringList argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    AreaData *l_area = server->getAreaById(areaId());
    l_area->toggleIniswap();
    QString state = l_area->iniswapAllowed() ? "allowed." : "disallowed.";
    sendServerMessage("Iniswapping in this area is now " + state);
}

void AOClient::cmdPermitSaving(int argc, QStringList argv)
{
    Q_UNUSED(argc);

    AOClient *l_client = server->getClientByID(argv[0].toInt());
    if (l_client == nullptr) {
        sendServerMessage("Invalid ID.");
        return;
    }
    l_client->m_testimony_saving = true;
    sendServerMessage("Testimony saving has been enabled for client " + QString::number(l_client->clientId()));
}

void AOClient::cmdKickUid(int argc, QStringList argv)
{
    QString l_reason = argv[1];

    if (argc > 2) {
        for (int i = 2; i < argv.length(); i++) {
            l_reason += " " + argv[i];
        }
    }

    bool conv_ok = false;
    int l_uid = argv[0].toInt(&conv_ok);
    if (!conv_ok) {
        sendServerMessage("Invalid user ID.");
        return;
    }

    AOClient *l_target = server->getClientByID(l_uid);
    if (l_target == nullptr) {
        sendServerMessage("No client with that ID found.");
        return;
    }
    const QString targetData(QString("[%1] %2 (%3)").arg(QString::number(l_target->clientId()), l_target->name().isEmpty() ? l_target->character().isEmpty() ? "Spectator" : l_target->character() : l_target->name(), l_target->m_ipid));
    l_target->sendPacket("KK", {l_reason});
    l_target->m_socket->close();
    sendServerMessage("Kicked client with UID " + argv[0] + " for reason: " + l_reason);
    if (m_vip_authenticated){
        const QString userdata(QString("[%1] %2").arg(QString::number(clientId()), name().isEmpty() ? character().isEmpty() ? "Spectator" : character() : name()));
        for (auto C : server->getClients())
            if (m_authenticated)
                C->sendServerMessage(QString("[VIP]%1 was kicking %2: %3").arg(userdata, targetData, l_reason));
    }
}

void AOClient::cmdUpdateBan(int argc, QStringList argv)
{
    bool conv_ok = false;
    int l_ban_id = argv[0].toInt(&conv_ok);
    if (!conv_ok) {
        sendServerMessage("Invalid ban ID.");
        return;
    }
    QVariant l_updated_info;
    if (argv[1] == "duration") {
        long long l_duration_seconds = 0;
        if (argv[2] == "perma")
            l_duration_seconds = -2;
        else
            l_duration_seconds = parseTime(argv[2]);
        if (l_duration_seconds == -1) {
            sendServerMessage("Invalid time format. Format example: 1h30m");
            return;
        }
        l_updated_info = QVariant(l_duration_seconds);
    }
    else if (argv[1] == "reason") {
        QString l_args_str = argv[2];
        if (argc > 3) {
            for (int i = 3; i < argc; i++)
                l_args_str += " " + argv[i];
        }
        l_updated_info = QVariant(l_args_str);
    }
    else {
        sendServerMessage("Invalid update type.");
        return;
    }
    if (!server->getDatabaseManager()->updateBan(l_ban_id, argv[1], l_updated_info)) {
        sendServerMessage("There was an error updating the ban. Please confirm the ban ID is valid.");
        return;
    }
    sendServerMessage("Ban updated.");
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
    Q_UNUSED(argv);

    AreaData *l_area = server->getAreaById(areaId());
    foreach (int l_client_id, l_area->owners()) {
        l_area->removeOwner(l_client_id);
    }
    arup(ARUPType::CM, true);
    sendServerMessage("Removed all CMs from this area.");
}

void AOClient::cmdKickOther(int argc, QStringList argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    int l_kick_counter = 0;

    QList<AOClient *> l_target_clients;
    const QList<AOClient *> l_targets_hwid = server->getClientsByHwid(m_hwid);
    l_target_clients = server->getClientsByIpid(m_ipid);

    // Merge both lookups into one single list.)
    for (AOClient *l_target_candidate : qAsConst(l_targets_hwid)) {
        if (!l_target_clients.contains(l_target_candidate)) {
            l_target_clients.append(l_target_candidate);
        }
    }

    // The list is unique, we can only have on instance of the current client.
    l_target_clients.removeOne(this);
    for (AOClient *l_target_client : qAsConst(l_target_clients)) {
        l_target_client->m_socket->close();
        l_kick_counter++;
    }
    sendServerMessage("Kicked " + QString::number(l_kick_counter) + " multiclients from the server.");
}
