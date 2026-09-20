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
#include "config_manager.h"
#include "packet/packet_factory.h"
#include "packet/packet_mc.h"
#include "server.h"

// This file is for commands under the area category in aoclient.h
// Be sure to register the command in the header before adding it here!

void AOClient::cmdCM(const int argc, const QStringList &argv){
    auto current_area = server->getAreaById(areaId());
    if (current_area.isNull())
        return;
    
    switch (argc){ /* switch were better */
    case 0: // > [user] <
        if (current_area->isProtected()){ // > [AREA-PROTECTED] <
            if (isAuthenticated()){ // > [VIP / Moderator / root] bypass.. <
                if (current_area->RegisterOwner(clientId())){
                    sendServerMessageArea(QString("[%1][%2] %3 is now CM in this area.").arg(isMAuthenticated() ? "M" : "VIP", QString::number(clientId()), name()));
                    arup(ARUPType::CM, true);
                }
                else
                    sendServerMessage("You are already a CM in this area.");

            }
            else if (current_area->owners().contains(clientId()))
                sendServerMessage("You are already a CM in this [protected] area.");
            else // > regular user 'not' bypass the protected area and they cannot <
                sendServerMessage("This area is protected, you may not become CM.");
        }
        else if (current_area->RegisterOwner(clientId())){
            sendServerMessageArea(QString("[%1] %2 is now CM in this area.").arg(QString::number(clientId()), name()));
            arup(ARUPType::CM, true);
        }
        else
            sendServerMessage(current_area->owners().contains(clientId()) ? "You are already a CM in this area." : "You must become a CM in this area, please ask the other CMs to add you.");
        break;
    case 1: // > specific target <
        if (current_area->isProtected()){
            if (isAuthenticated()){
                bool l_client_ok = false;
                auto l_client = server->getClientByID(argv[0].toInt(&l_client_ok));
                const QString m_name = QString("[%1] %2").arg(QString::number(clientId()), name().isEmpty() ? isSpectator() ? "[Spectator]" : character() : name());

                if (!l_client_ok || l_client.isNull())
                    sendServerMessage("That doesn't look like a valid ID.");
                else if (l_client->areaId() != current_area->index())
                    sendServerMessage("That client is not in this area.");
                else if (current_area->RegisterOwner(l_client->clientId())){
                    const QString l_name = QString("[%1] %2").arg(QString::number(l_client->clientId()), l_client->name().isEmpty() ? l_client->isSpectator() ? "[Spectator]" : l_client->character() : l_client->name());
                    sendServerMessageArea(QString("%1 is now CM in this [protected] area by %2.").arg(l_name, m_name));
                    arup(ARUPType::CM, true);
                }
                else
                    sendServerMessage(l_client == this ? "You are already a CM in this [protected] area." : "That client is already a CM in this [protected] area.");
            }
            else if (current_area->owners().contains(clientId())) // > regular user cannot add another person (following up the akashi area protect behave should applied) <
                sendServerMessage(argv[0].toInt() == clientId() ? "You are already a CM in this area." : "You cannot add a someone be CM in this [protected] area.");
            else
                sendServerMessage("This area is protected, you may not become CM.");
        }
        else if (checkPermission(ACLRole::CM)){
            bool l_client_ok = false;
            auto l_client = server->getClientByID(argv[0].toInt(&l_client_ok));
            const QString m_name = QString("[%1] %2").arg(QString::number(clientId()), name().isEmpty() ? isSpectator() ? "[Spectator]" : character() : name());

            if (!l_client_ok || l_client.isNull())
                sendServerMessage("That doesn't look like a valid ID.");
            else if (l_client->areaId() != current_area->index())
                sendServerMessage("That client is not in this area.");
            else if (current_area->RegisterOwner(l_client->clientId())){
                const QString l_name = QString("[%1] %2").arg(QString::number(l_client->clientId()), l_client->name().isEmpty() ? l_client->isSpectator() ? "[Spectator]" : l_client->character() : l_client->name());
                sendServerMessageArea(QString("%1 is now CM in this area by %2.").arg(l_name, m_name));
                arup(ARUPType::CM, true);
            }
            else
                sendServerMessage(l_client == this ? "You are already a CM in this area." : "That client is already a CM in this area.");
        }
        else{
            static QString message("You must become a CM in this area");
            if (!current_area->owners().isEmpty())
                message.append(", please ask the other CMs to add you");
            sendServerMessage(message + ".");
        }
        break;
    default: // > [Multi-Param] (aka "<number><space><number>") <
        if (current_area->isProtected()){
            if (isAuthenticated()){
                QStringList GetList = argv.filter(QRegularExpression("^[0-9]+$")), RegisteredCMS;
                GetList.removeDuplicates(); // somehow.. user could tried to insert more than 1 same client ids..
                const QString m_name = QString("[%1] %2").arg(QString::number(clientId()), name().isEmpty() ? isSpectator() ? "[Spectator]" : character() : name());

                for (auto I : GetList){
                    bool l_client_ok = false;
                    auto l_client = server->getClientByID(I.toInt(&l_client_ok));

                    if (!l_client_ok || l_client.isNull() || l_client->areaId() != current_area->index())
                        continue; // skip this Invalid.. or if that client are not in this area..

                    if (current_area->RegisterOwner(l_client->clientId())){
                        l_client->sendServerMessage(QString("You are become a CM in this [protected] area by %1.").arg(m_name));
                        RegisteredCMS.append(QString("[%1] %2").arg(QString::number(l_client->clientId()), l_client->name().isEmpty() ? l_client->isSpectator() ? "[Spectator]" : l_client->character() : l_client->name()));
                    }
                }
                RegisteredCMS.isEmpty() ? sendServerMessage("[ERROR] No clients would become CMs.") : sendServerMessageArea(QString("Some clients will become CM in this area by (%1) as follows:\n%2").arg(m_name, RegisteredCMS.join("\n")));
                arup(ARUPType::CM, true);
            }
            else
                sendServerMessage(current_area->owners().contains(clientId()) ? "You cannot add a someone be CM in this [protected] area." : "This area is protected, you cannot using this command.");
        }
        else if (checkPermission(ACLRole::CM)){
            QStringList GetList = argv.filter(QRegularExpression("^[0-9]+$")), RegisteredCMS;
            GetList.removeDuplicates(); // somehow.. user could tried to insert more than 1 same client ids..
            if (GetList.isEmpty()) // non-numbers..
                sendServerMessage("Invalid IDs.");
            else{ // catched numbers..
                const QString m_name = QString("[%1] %2").arg(QString::number(clientId()), name().isEmpty() ? isSpectator() ? "[Spectator]" : character() : name());

                for (auto I : GetList){
                    bool l_client_ok = false;
                    auto l_client = server->getClientByID(I.toInt(&l_client_ok));

                    if (!l_client_ok || l_client.isNull() || l_client->areaId() != current_area->index())
                        continue; // skip this Invalid.. or if that client are not in this area..

                    if (current_area->RegisterOwner(l_client->clientId())){
                        l_client->sendServerMessage(QString("You are become a CM in this area by %1.").arg(m_name));
                        RegisteredCMS.append(QString("[%1] %2").arg(QString::number(l_client->clientId()), l_client->name().isEmpty() ? l_client->isSpectator() ? "[Spectator]" : l_client->character() : l_client->name()));
                    }
                }
                RegisteredCMS.isEmpty() ? sendServerMessage("[ERROR] No clients would become CMs.") : sendServerMessageArea(QString("Some clients will become CM in this area by (%1) as follows:\n%2").arg(m_name, RegisteredCMS.join("\n")));
                arup(ARUPType::CM, true);
            }
        }
        else{
            static QString message("You must become a CM in this area");
            if (!current_area->owners().isEmpty())
                message.append(", please ask the other CMs to add you");
            sendServerMessage(message + ".");
        }
        break;
    }
}

void AOClient::cmdUnCM(const int argc, const QStringList &argv){
    auto current_area = server->getAreaById(areaId());
    if (current_area.isNull())
        return;

    if (current_area->owners().isEmpty())
        sendServerMessage("There are no CMs in this area.");
    else{
        switch (argc){ /* switch were winner again.. :) */
        case 0: // [self]
            if (current_area->RegisterOwner(clientId(), true))
                arup(ARUPType::LOCKED, true);
            sendServerMessage("You are no longer CM in this area.");
            arup(ARUPType::CM, true);
            break;
        default: // [Multi-target]
            if (current_area->owners().contains(clientId()) || checkPermission(ACLRole::UNCM)){
                QStringList GetList = argv.filter(QRegularExpression("[0-9]+"));
                GetList.removeDuplicates();
                if (GetList.isEmpty())
                    sendServerMessage("Invalid IDs.");
                else{
                    const QStringList AuthType{"[CM]", "[VIP]", "[M]", "[*M]"};
                    const QString m_name = QString("%1[%2] %3").arg(AuthType[m_authenticated_type +1], QString::number(clientId()), name().isEmpty() ? isSpectator() ? "[Spectator]" : character() : name());
                    QList<int> index_pass;

                    for (auto I : GetList){
                        auto l_client = server->getClientByID(I.toInt());

                        if (l_client.isNull())
                            continue;

                        if (current_area->RegisterOwner(l_client->clientId(), true)){
                            l_client->sendServerMessage(l_client == this ? "You are no longer CM in this area." : l_client->areaId() == current_area->index() ? QString("Your CM has been revoked by %1.").arg(m_name) : QString("Your CM of [%1] has been revoked by %2.").arg(current_area->name(), m_name));
                            index_pass.append(l_client->clientId());
                        }
                    }

                    if (index_pass.isEmpty())
                        sendServerMessage("[ERROR] No clients would be unCMed.");
                    else
                        sendServerMessage(index_pass.size() == GetList.size() ? "You successfully revoked CM from all target clients." : QString("You successfully revoked CM from (%1 / %2) clients.").arg(QString::number(index_pass.size()), QString::number(GetList.size())));

                    arup(ARUPType::LOCKED, true);
                    arup(ARUPType::CM, true);
                }
            }
            else
                sendServerMessage(isAuthenticated() ? "You do not have permission to unCM others." : "You must become CM in this area.");
            break;
        case 1: // [specific]
            bool valid_uid;
            auto l_client = server->getClientByID(argv[0].toInt(&valid_uid));
            if (!valid_uid || l_client.isNull())
                sendServerMessage("No client with that ID found or invalid.");
            else if (current_area->owners().contains(clientId()) || checkPermission(ACLRole::UNCM)){
                if (current_area->RegisterOwner(l_client->clientId(), true)){
                    const QStringList AuthType{"[CM]", "[VIP]", "[M]", "[*M]"};
                    const QString m_name = QString("%1[%2] %3").arg(AuthType[m_authenticated_type +1], QString::number(clientId()), name().isEmpty() ? isSpectator() ? "[Spectator]" : character() : name());
                    if (l_client == this)
                        sendServerMessage("You are no longer CM in this area.");
                    else{
                        sendServerMessage(QString("[%1] %2 was successfully removed from CM in this area.").arg(QString::number(l_client->clientId()), l_client->name().isEmpty() ? l_client->m_current_char.isEmpty() ? "[Spectator]" : l_client->m_current_char : l_client->name()));
                        l_client->sendServerMessage(QString("Your CM of this area has been revoked by %1.").arg(m_name));
                    }
                    arup(ARUPType::LOCKED, true);
                    arup(ARUPType::CM, true);
                }
                else
                    sendServerMessage("That client are not in CMs of this area.");
            }
            else
                sendServerMessage(isAuthenticated() ? "You do not have permission to unCM others." : "You must become CM in this area.");
            break;
        }
    }
}

void AOClient::cmdInvite(const int argc, const QStringList &argv){
    auto l_area = server->getAreaById(areaId());
    if (l_area.isNull())
        return;

    if (l_area->lockStatus() == AreaData::LockStatus::FREE) // preventing user re-use this when area are <free>..
        sendServerMessage("This command can only be used when this area is [spectatable / locked].");
    else if (argc >= 2){
        QStringList GetList = argv.filter(QRegularExpression("[0-9]+")), RegisteredInvited;
        GetList.removeDuplicates();
        if (GetList.isEmpty())
            sendServerMessage("Invalid IDs.");
        else{
            const QString m_name = QString("%1[%2] %3").arg(QStringList({"", "[VIP]", "[M]", "[*M]"})[m_authenticated_type +1], QString::number(clientId()), name().isEmpty() ? isSpectator() ? "[Spectator]" : character() : name());

            for (auto I : GetList){
                bool l_client_ok = false;
                auto l_client = server->getClientByID(I.toInt(&l_client_ok));

                if (!l_client_ok || l_client.isNull())
                    continue;

                if (l_area->invite(l_client->clientId())){
                    l_client->areaId() == l_area->index() ? l_client->sendServerMessage(l_client == this ? "You were given access to this area." : QString("You were given access to this area by %1").arg(m_name)) : l_client->sendServerMessage(QString("You were invited and given access to %1.").arg(l_area->name()));
                    RegisteredInvited.append(QString("%1[%2] %3").arg(QStringList({"", "[VIP]", "[M]", "[*M]"})[l_client->m_authenticated_type +1], QString::number(l_client->clientId()), l_client->name().isEmpty() ? l_client->isSpectator() ? "[Spectator]" : l_client->character() : l_client->name()));
                }
            }
            switch (RegisteredInvited.size()){
            case 0:
                sendServerMessage("You unsuccessfully invited clients from those IDs.");
                break;
            default:
                sendServerMessage(RegisteredInvited.size() == GetList.size() ? QString("You successfully invited clients in this area:\n%1").arg(RegisteredInvited.join('\n')) : QString("You successfully invited (%1 / %2) clients in this area:\n%3").arg(QString::number(RegisteredInvited.size()), QString::number(GetList.size()), RegisteredInvited.join('\n')));
                break;
            }
        }
    }
    else{
        bool validID;
        auto l_invite_client = server->getClientByID(argv[0].toInt(&validID));
        if (validID && !l_invite_client.isNull()){
            if (l_area->invite(l_invite_client->clientId())){
                sendServerMessage("You invited ID " + QString::number(l_invite_client->clientId()));
                l_invite_client->sendServerMessage("You were invited and given access to " + l_area->name());
            }
            else
                sendServerMessage("That ID is already on the invite list.");
        }
        else
            sendServerMessage("That does not look like a valid ID or Invalid client.");
    }
}

void AOClient::cmdUnInvite(const int argc, const QStringList &argv){
    Q_UNUSED(argc);
    auto l_area = server->getAreaById(areaId());
    if (l_area.isNull())
        return;

    switch (l_area->lockStatus()){
    case AreaData::LockStatus::FREE: // preventing user re-use this when area are <free>..
        sendServerMessage("This command can only be used when this area is [spectatable / locked].");
        break;
    default:
        if (argc >= 2){
            QStringList GetList = argv.filter(QRegularExpression("^[0-9]+$")), RegisteredUnInvited;
            GetList.removeDuplicates();
            if (GetList.isEmpty())
                sendServerMessage("Invalid IDs.");
            else{
                const QString m_name = QString("%1[%2] %3").arg(QStringList({"", "[VIP]", "[M]", "[*M]"})[m_authenticated_type +1], QString::number(clientId()), name().isEmpty() ? isSpectator() ? "[Spectator]" : character() : name());
                
                for (auto I : GetList){
                    bool l_client_ok = false;
                    auto l_client = server->getClientByID(I.toInt(&l_client_ok));
                    
                    if (!l_client_ok || l_client.isNull())
                        continue;
                    
                    if (l_area->uninvite(l_client->clientId())){
                        if (l_area->RegisterVoice(l_client->clientId(), true))
                            server->broadcastVJoinLeave(l_client->clientId(), true, l_area->index());
                        l_client->areaId() == l_area->index() ? l_client->sendServerMessage(l_client == this ? "You were uninvited from this area." : QString("You were uninvited from this area by %1").arg(m_name)) : l_client->sendServerMessage(QString("You were uninvited from %1.").arg(l_area->name()));
                        RegisteredUnInvited.append(QString("%1[%2] %3").arg(QStringList({"", "[VIP]", "[M]", "[*M]"})[l_client->m_authenticated_type +1], QString::number(l_client->clientId()), l_client->name().isEmpty() ? l_client->isSpectator() ? "[Spectator]" : l_client->character() : l_client->name()));
                    }
                }
                switch (RegisteredUnInvited.size()){
                case 0:
                    sendServerMessage("You unsuccessfully uninvited clients from those IDs.");
                    break;
                default:
                    sendServerMessage(RegisteredUnInvited.size() == GetList.size() ? QString("You successfully uninvited clients in this area:\n%1").arg(RegisteredUnInvited.join('\n')) : QString("You successfully uninvited (%1 / %2) clients in this area:\n%3").arg(QString::number(RegisteredUnInvited.size()), QString::number(GetList.size()), RegisteredUnInvited.join('\n')));
                    break;
                }
            }
        }
        else{
            bool validID;
            auto l_uninvite_client = server->getClientByID(argv[0].toInt(&validID));
            if (validID && !l_uninvite_client.isNull()){
                if (l_area->owners().contains(l_uninvite_client->clientId()))
                    sendServerMessage("You cannot uninvite a CM!");
                else if (l_area->uninvite(l_uninvite_client->clientId())){
                    if (l_area->RegisterVoice(l_uninvite_client->clientId(), true))
                        server->broadcastVJoinLeave(l_uninvite_client->clientId(), true, l_area->index());
                    sendServerMessage("You uninvited ID " + QString::number(l_uninvite_client->clientId()));
                    l_uninvite_client->sendServerMessage("You were uninvited from " + l_area->name());
                }
                else
                    sendServerMessage("That ID is not on the invite list.");
            }
            else
                sendServerMessage("That does not look like a valid ID or Invalid client.");
        }
        break;
    }
}

void AOClient::cmdLock(const int argc, const QStringList &argv){
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    auto l_area = server->getAreaById(areaId());
    if (l_area.isNull())
        return;

    if (l_area->isProtected())
        sendServerMessage("You cannot change area lock state, this area are protected.");
    else if (l_area->lock()){
        const QVector<int> c_index = l_area->joinedIDs();
        for (const int I : l_area->joinedIDs()){
            const auto j_client = server->getClientByID(I);
            if (j_client.isNull())
                continue;
            if (!l_area->owners().contains(j_client->clientId()))
                j_client->sendServerMessage("This area is now locked.");
            l_area->invite(j_client->clientId());
        }
        
        for (const int I : l_area->owners()){
            const auto cm_client = server->getClientByID(I);
            if (cm_client.isNull())
                continue;
            cm_client->sendServerMessage(cm_client == this ? "You set this area to locked." : QString("[%1] %2 set %3 area to locked.").arg(QString::number(clientId()), name().isEmpty() ? character().isEmpty() ? "[Spectator]" : character() : name(), cm_client->areaId() == l_area->index() ? "this" : "[" + l_area->name() + "]"));
        }
        arup(ARUPType::LOCKED, true);
    }
    else
        sendServerMessage("This area is already locked.");
}

void AOClient::cmdSpectatable(const int argc, const QStringList &argv){
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    auto l_area = server->getAreaById(areaId());
    if (l_area.isNull())
        return;

    if (l_area->isProtected())
        sendServerMessage("You cannot change area lock state, this area are protected.");
    else if (l_area->spectatable()){
        const QVector<int> c_index = l_area->joinedIDs();
        for (const int I : l_area->joinedIDs()){
            const auto j_client = server->getClientByID(I);
            if (j_client.isNull())
                continue;
            if (!l_area->owners().contains(j_client->clientId()))
                j_client->sendServerMessage("This area is now spectatable.");
            l_area->invite(j_client->clientId());
        }
        
        for (const int I : l_area->owners()){
            const auto cm_client = server->getClientByID(I);
            if (cm_client.isNull())
                continue;
            cm_client->sendServerMessage(cm_client == this ? "You set this area to spectatable." : QString("[%1] %2 set %3 area to spectatable.").arg(QString::number(clientId()), name().isEmpty() ? character().isEmpty() ? "[Spectator]" : character() : name(), cm_client->areaId() == l_area->index() ? "this" : "[" + l_area->name() + "]"));
        }
        arup(ARUPType::LOCKED, true);
    }
    else
        sendServerMessage("This area is already in spectate mode.");
}

void AOClient::cmdUnLock(const int argc, const QStringList &argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    auto l_area = server->getAreaById(areaId());
    if (l_area.isNull())
        return;

    if (l_area->isProtected())
        sendServerMessage("You cannot change area lock state, this area are protected.");
    else if (l_area->unlock())
        arup(ARUPType::LOCKED, true);
    else
        sendServerMessage("this area is already unlocked.");
}

void AOClient::cmdGetAreas(const int argc, const QStringList &argv){
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    QStringList l_entries;
    l_entries.append("\n== Currently Online: " + QString::number(server->getPlayerCount()) + " ==");
    for (const auto &Area : server->getAreas()){
        if (!Area.isNull() && Area->playerCount() >= 1)
            l_entries.append(buildAreaList(Area->index()));
    }
    sendServerMessage(l_entries.join("\n"));
}

void AOClient::cmdGetArea(const int argc, const QStringList &argv){
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    sendServerMessage(buildAreaList(areaId()).join("\n"));
}

void AOClient::cmdArea(const int argc, const QStringList &argv){
    Q_UNUSED(argc);

    bool validAreaID;
    int l_new_area = argv[0].toInt(&validAreaID);
    if (validAreaID && !server->getAreaById(l_new_area).isNull())
        changeArea(l_new_area);
    else
        sendServerMessage("That does not look like a valid area ID.");
}

void AOClient::cmdAreaKick(const int argc, const QStringList &argv){
    auto l_area = server->getAreaById(areaId());
    if (l_area.isNull())
        return;
    
    bool validID;
    const int l_idx = argv[0].toInt(&validID);
    switch (argc){
    case 1:
        if (validID){
            auto Target_kick = server->getClientByID(l_idx);
    
            if (Target_kick.isNull())
                sendServerMessage("No client with that ID found.");
            else if (server->getAreaById(Target_kick->areaId()) != l_area)
                sendServerMessage("That client is not in this area.");
            else if (l_area->index() == 0) // if user in first area..
                sendServerMessage("You cannot use this command in the first area.");
            else if (l_area->owners().contains(Target_kick->clientId()))
                sendServerMessage("You cannot kick another CM!");
            else if (Target_kick == this)
                sendServerMessage("Are you sure you want to be kicked from this area?\nAnyway, you cannot kick yourself from here."); // who'll knows if someone really want that.. :)
            else{
                Target_kick->changeArea(0);
                l_area->uninvite(Target_kick->clientId());
                sendServerMessage("Client ID " + QString::number(Target_kick->clientId()) + " kicked back to " + server->getAreaName(0));
            }
        }
        else if (argv[0] == "*"){ // [all]
            auto GetJoinedIDs = l_area->joinedIDs();
            GetJoinedIDs.removeAll(clientId()); // removing user from the listed..
            if (GetJoinedIDs.isEmpty())
                sendServerMessage("You are attempting to use this command when no one is in this area besides yourself.");
            else if (l_area->index() == 0) // if user in first area..
                sendServerMessage("You cannot using this command in first area.");
            else if (checkPermission(ACLRole::KICK)){ // including the CMs (if they have perms)..
                for (const int Ids : GetJoinedIDs){
                    auto l_client = server->getClientByID(Ids);
                    if (l_client.isNull())
                        continue;
                    l_area->removeOwner(Ids);
                    l_client->changeArea(0);
                    l_client->sendServerMessage(QString("You have been kicked from the [%1] area.").arg(l_area->name()));
                }
                sendServerMessage("All clients (including CMs) kicked back to " + server->getAreaName(0));
            }
            else{
                for (const int Ids : GetJoinedIDs){
                    auto l_client = server->getClientByID(Ids);
                    if (l_client.isNull() || l_area->owners().contains(l_client->clientId()))
                        continue;
                    l_client->changeArea(0);
                    l_client->sendServerMessage(QString("You have been kicked from the [%1] area.").arg(l_area->name()));
                }
                sendServerMessage("All clients (except CMs) kicked back to " + server->getAreaName(0));
            }
        }
        else
            sendServerMessage("That does not look like a valid ID.");
        break;
    case 2: default:
        if (validID){
            auto Target_kick = server->getClientByID(l_idx);
    
            if (Target_kick.isNull())
                sendServerMessage("No client with that ID found.");
            else if (server->getAreaById(Target_kick->areaId()) != l_area)
                sendServerMessage("That client is not in this area.");
            else if (l_area->owners().contains(Target_kick->clientId()))
                sendServerMessage("You cannot kick another CM!");
            else{
                bool areaId_pass = false;
                auto target_area = server->getAreaById(argv[1].toInt(&areaId_pass));
                if (!areaId_pass)
                    sendServerMessage("That does not look like a valid Area ID.");
                else if (target_area.isNull())
                    sendServerMessage("That area doesn't exist.");
                else if (!checkPermission(ACLRole::KICK))
                    sendServerMessage("You do not have permission to kick to specific area, do /area_kick [ID] instead.");
                else if (target_area == l_area)
                    sendServerMessage("You cannot use the same area ID as this area's ID.");
                else if (Target_kick == this)
                    sendServerMessage("Are you sure you want to be kicked from this area?\nAnyway, you cannot kick yourself from here.");
                else{
                    Target_kick->changeArea(0);
                    l_area->uninvite(Target_kick->clientId());
                    sendServerMessage("Client ID " + QString::number(Target_kick->clientId()) + " kicked back to area 0.");
                }
            }
        }
        else if (argv[0] == "*"){
            auto GetJoinedIDs = l_area->joinedIDs();
            GetJoinedIDs.removeAll(clientId()); // removing user from the listed..
            if (GetJoinedIDs.isEmpty())
                sendServerMessage("You are attempting to use this command when no one is in this area besides yourself.");
            else if (checkPermission(ACLRole::KICK)){ // including the CMs (if they have perms)..
                bool areaId_pass = false;
                const auto target_area = server->getAreaById(argv[1].toInt(&areaId_pass));
                if (!areaId_pass)
                    sendServerMessage("That does not look like a valid Area ID.");
                else if (target_area.isNull())
                    sendServerMessage("That area doesn't exist.");
                else if (target_area == l_area)
                    sendServerMessage("You cannot use the same area ID as this area's ID.");
                else if (checkPermission(ACLRole::UNCM)){ // if.. [VIP / Moderators] has [UNCM] perms ([ROOT] always passed)..
                    for (const int Ids : GetJoinedIDs){ /* > including the CMs.. < */
                        auto l_client = server->getClientByID(Ids);
                        if (l_client.isNull())
                            continue;

                        l_area->removeOwner(Ids);
                        l_client->changeArea(target_area->index());
                        l_client->sendServerMessage(QString("You have been kicked from the [%1] area.").arg(l_area->name()));
                    }
                    sendServerMessage(QString("All clients (including CMs) kicked to [%1] area.").arg(target_area->name()));
                }
                else{
                    for (const int Ids : GetJoinedIDs){
                        auto l_client = server->getClientByID(Ids);
                        if (l_client.isNull() || l_area->owners().contains(l_client->clientId()))
                            continue;

                        l_client->changeArea(target_area->index());
                        l_client->sendServerMessage(QString("You have been kicked from the [%1] area.").arg(l_area->name()));
                    }
                    sendServerMessage(QString("All clients (except CMs) kicked to [%1] area.").arg(target_area->name()));
                }
            }
            else
                sendServerMessage("You do not have permission to kick to specific area, do /area_kick [ID] instead.");
        }
        else
            sendServerMessage("That does not look like a valid ID.");
        break;
    }
}

void AOClient::cmdSetBackground(const int argc, const QStringList &argv){
    Q_UNUSED(argc);

    QString f_background = argv.join(" ");
    auto area = server->getAreaById(areaId());
    if (area){
        switch (area->lockStatus()){
        case AreaData::LockStatus::SPECTATABLE: case AreaData::LockStatus::LOCKED:
            if (checkPermission(ACLRole::BYPASS_LOCKS)){
                if (area->bgLocked()){
                    if (checkPermission(ACLRole::SUPER)){
                        area->setBackground(f_background);
                        server->broadcast(PacketFactory::createPacket("BN", {f_background, area->side()}), areaId());
                        QString ambience_name = ConfigManager::ambience()->value(f_background + "/ambience").toString();
                        const bool isEmptyAmbie = ambience_name.isEmpty();
                        server->broadcast(PacketMC::CreateMusic(isEmptyAmbie ? "~stop.mp3" : ambience_name, -1, characterName(), !isEmptyAmbie, 1), areaId());
                        sendServerMessageArea(QString("[%1] %2 changed the background to %3").arg(QString::number(clientId()), character().isEmpty() ? "[Spectator]" : character(), f_background));
                    }
                    else
                        sendServerMessage("This area's background is locked.");
                }
                else if (checkPermission(ACLRole::SUPER) || server->getBackgrounds().contains(f_background, Qt::CaseInsensitive) || area->ignoreBgList()) {
                    area->setBackground(f_background);
                    server->broadcast(PacketFactory::createPacket("BN", {f_background, area->side()}), areaId());
                    QString ambience_name = ConfigManager::ambience()->value(f_background + "/ambience").toString();
                    const bool isEmptyAmbie = ambience_name.isEmpty();
                    server->broadcast(PacketMC::CreateMusic(isEmptyAmbie ? "~stop.mp3" : ambience_name, -1, characterName(), !isEmptyAmbie, 1), areaId());
                    sendServerMessageArea(QString("[%1] %2 changed the background to %3").arg(QString::number(clientId()), character().isEmpty() ? "[Spectator]" : character(), f_background));
                }
                else
                    sendServerMessage("Invalid background name.");
            }
            else if (area->invited().contains(clientId())){
                if (area->bgLocked()){
                    if (checkPermission(ACLRole::SUPER)){
                        area->setBackground(f_background);
                        server->broadcast(PacketFactory::createPacket("BN", {f_background, area->side()}), areaId());
                        QString ambience_name = ConfigManager::ambience()->value(f_background + "/ambience").toString();
                        const bool isEmptyAmbie = ambience_name.isEmpty();
                        server->broadcast(PacketMC::CreateMusic(isEmptyAmbie ? "~stop.mp3" : ambience_name, -1, characterName(), !isEmptyAmbie, 1), areaId());
                        sendServerMessageArea(QString("[%1] %2 changed the background to %3").arg(QString::number(clientId()), character().isEmpty() ? "[Spectator]" : character(), f_background));
                    }
                    else
                        sendServerMessage("This area's background is locked.");
                }
                else if (checkPermission(ACLRole::SUPER) || server->getBackgrounds().contains(f_background, Qt::CaseInsensitive) || area->ignoreBgList()) {
                    area->setBackground(f_background);
                    server->broadcast(PacketFactory::createPacket("BN", {f_background, area->side()}), areaId());
                    QString ambience_name = ConfigManager::ambience()->value(f_background + "/ambience").toString();
                    const bool isEmptyAmbie = ambience_name.isEmpty();
                    server->broadcast(PacketMC::CreateMusic(isEmptyAmbie ? "~stop.mp3" : ambience_name, -1, characterName(), !isEmptyAmbie, 1), areaId());
                    sendServerMessageArea(QString("[%1] %2 changed the background to %3").arg(QString::number(clientId()), character().isEmpty() ? "[Spectator]" : character(), f_background));
                }
                else
                    sendServerMessage("Invalid background name.");
            }
            else
                sendServerMessage("Spectators / Uninvites are blocked from changing the background.");
            break;
        case AreaData::LockStatus::FREE:
            if (area->bgLocked()){
                if (checkPermission(ACLRole::SUPER)){
                    area->setBackground(f_background);
                    server->broadcast(PacketFactory::createPacket("BN", {f_background, area->side()}), areaId());
                    QString ambience_name = ConfigManager::ambience()->value(f_background + "/ambience").toString();
                    const bool isEmptyAmbie = ambience_name.isEmpty();
                    server->broadcast(PacketMC::CreateMusic(isEmptyAmbie ? "~stop.mp3" : ambience_name, -1, characterName(), !isEmptyAmbie, 1), areaId());
                    sendServerMessageArea(QString("[%1] %2 changed the background to %3").arg(QString::number(clientId()), character().isEmpty() ? "[Spectator]" : character(), f_background));
                }
                else
                    sendServerMessage("This area's background is locked.");
            }
            else if (checkPermission(ACLRole::SUPER) || server->getBackgrounds().contains(f_background, Qt::CaseInsensitive) || area->ignoreBgList()) {
                area->setBackground(f_background);
                server->broadcast(PacketFactory::createPacket("BN", {f_background, area->side()}), areaId());
                QString ambience_name = ConfigManager::ambience()->value(f_background + "/ambience").toString();
                const bool isEmptyAmbie = ambience_name.isEmpty();
                server->broadcast(PacketMC::CreateMusic(isEmptyAmbie ? "~stop.mp3" : ambience_name, -1, characterName(), !isEmptyAmbie, 1), areaId());
                sendServerMessageArea(QString("[%1] %2 changed the background to %3").arg(QString::number(clientId()), character().isEmpty() ? "[Spectator]" : character(), f_background));
            }
            else
                sendServerMessage("Invalid background name.");
            break;
        }
    }
}

void AOClient::cmdSetSide(const int argc, const QStringList &argv){
    Q_UNUSED(argc);

    AreaData *area = server->getAreaById(areaId());
    if (area->bgLocked())
        sendServerMessage("This area's background is locked.");
    else{
        const QString side = argv.join(" ");
        area->setSide(side);
        server->broadcast(PacketFactory::createPacket("BN", {area->background(), side}), areaId());
        sendServerMessageArea(QString("[%1] %2 %3").arg(QString::number(clientId()), character().isEmpty() ? "[Spectator]" : character(), side.isEmpty() ? "unlocked the background side" : "locked the background side to " + side));
    }
}

void AOClient::cmdBgLock(const int argc, const QStringList &argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    auto l_area = server->getAreaById(areaId());
    if (l_area.isNull())
        return;

    if (!l_area->bgLocked())
        l_area->toggleBgLock();

    sendServerMessageArea(QString("[%1] %2 locked the background.").arg(QString::number(clientId()), character().isEmpty() ? "[Spectator]" : character()));
}

void AOClient::cmdBgUnlock(const int argc, const QStringList &argv){
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    auto l_area = server->getAreaById(areaId());
    if (l_area.isNull())
        return;

    if (l_area->bgLocked())
        l_area->toggleBgLock();

    sendServerMessageArea(QString("[%1] %2 unlocked the background.").arg(QString::number(clientId()), character().isEmpty() ? "[Spectator]" : character()));
}

void AOClient::cmdStatus(const int argc, const QStringList &argv){
    Q_UNUSED(argc);

    auto l_area = server->getAreaById(areaId());
    if (l_area.isNull())
        return;
    const QString l_arg = argv[0].toLower();

    if (l_area->changeStatus(l_arg)) {
        arup(ARUPType::STATUS, true);
        sendServerMessageArea(QString("[%1] %2 changed status to %3.").arg(QString::number(clientId()), character().isEmpty() ? "[Spectator]" : character(), l_arg.toUpper()));
    }
    else
        sendServerMessage("That does not look like a valid status. Valid statuses are: [" + AreaData::map_statuses.keys().join(", ") + "]");
}

void AOClient::cmdJudgeLog(const int argc, const QStringList &argv){
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    auto l_area = server->getAreaById(areaId());
    if (l_area.isNull())
        return;

    if (l_area->judgelog().isEmpty())
        sendServerMessage("There have been no judge actions in this area.");
    else
        sendServerMessage(checkPermission(ACLRole::KICK) || checkPermission(ACLRole::BAN) ? l_area->judgelog().join("\n") : l_area->judgelog().replaceInStrings(QRegularExpression("[(].*[)]"), "").join('\n'));
}

void AOClient::cmdIgnoreBgList(const int argc, const QStringList &argv){
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    auto l_area = server->getAreaById(areaId());
    if (l_area.isNull())
        return;

    l_area->toggleIgnoreBgList();
    sendServerMessage("BG list in this area is now " + QString(l_area->ignoreBgList() ? "ignored." : "enforced."));
}

void AOClient::cmdAreaMessage(const int argc, const QStringList &argv){
    Q_UNUSED(argc)

    auto l_area = server->getAreaById(areaId());
    if (l_area.isNull())
        return;

    if (argv.isEmpty())
        sendServerMessage(l_area->areaMessage());
    else{
        l_area->changeAreaMessage(argv.join(" "));
        sendServerMessage("Updated this area's message.");
    }
}

void AOClient::cmdToggleAreaMessageOnJoin(const int argc, const QStringList &argv){
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    auto l_area = server->getAreaById(areaId());
    if (l_area.isNull())
        return;

    l_area->toggleAreaMessageJoin();
    sendServerMessage("Sending message on area join is now " + QString(l_area->sendAreaMessageOnJoin() ? "enabled." : "disabled."));
}

void AOClient::cmdToggleWtce(const int argc, const QStringList &argv){
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    auto l_area = server->getAreaById(areaId());
    if (l_area.isNull())
        return;

    l_area->toggleWtceAllowed();
    sendServerMessage("Using testimony animations is now " + QString(l_area->isWtceAllowed() ? "enabled." : "disabled."));
}

void AOClient::cmdToggleShouts(const int argc, const QStringList &argv){
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    auto l_area = server->getAreaById(areaId());
    if (l_area.isNull())
        return;

    l_area->toggleShoutAllowed();
    sendServerMessage("Using shouts is now " + QString(l_area->isShoutAllowed() ? "enabled." : "disabled."));
}

void AOClient::cmdClearAreaMessage(const int argc, const QStringList &argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    auto l_area = server->getAreaById(areaId());
    if (l_area.isNull())
        return;

    l_area->clearAreaMessage();
    if (l_area->sendAreaMessageOnJoin())              // Turn off the automatic sending.
        cmdToggleAreaMessageOnJoin(0, QStringList{}); // Dummy values.
}

void AOClient::cmdWebfiles(const int argc, const QStringList &argv){
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    QStringList l_weblinks;
    const auto clients = server->getAreaById(areaId())->joinedIDs();
    for (int Index : clients){
        const auto client = server->getClientByID(Index);
        if (!client.isNull() && !client->isSpectator()){
            if (client->m_current_iniswap.isEmpty())
                l_weblinks.append(QString("%1 [%2] %3 using: %4").arg(client == this ? (m_version.type == ClientVersion::ClientType::NDS ? "[YOU] " : " ➤ ") : (m_version.type == ClientVersion::ClientType::NDS ? " * " : " · "), QString::number(client->clientId()), client->characterName().isEmpty() ? client->name().isEmpty() ? "[Unknown]" : client->name() : client->characterName(), client->character()));
            else if (client->m_current_iniswap.toLower() != client->character().toLower()){
                QStringList m_name(client->character());
                if (!client->characterName().isEmpty() || !client->name().isEmpty())
                    m_name.append("(" + QString(client->characterName().isEmpty() ? client->name() : client->characterName()) + ")");
                l_weblinks.append(QString("%1 [%2] %3 using: %4").arg(client == this ? (m_version.type == ClientVersion::ClientType::NDS ? "[YOU] " : " ➤ ") : (m_version.type == ClientVersion::ClientType::NDS ? " * " : " · "), QString::number(client->clientId()), m_name.join(' '), client->m_current_iniswap));
            }
        }
    }
    sendServerMessage(l_weblinks.isEmpty() ? "[Webfiles] theres nothing on the list." : QString("\n=== [Webfiles] ===\n%1\n=== total: %2 ===\nIf you want to download any char or BG head to: %3").arg(l_weblinks.join('\n'), QString::number(l_weblinks.size()), ConfigManager::ServerWebdownloaderURL().toString()));
}

void AOClient::cmdMedievalMode(const int argc, const QStringList &argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    auto l_area = server->getAreaById(areaId());
    if (l_area.isNull())
        return;

    l_area->toggleMedievalMode();
    sendServerMessageArea("Hear ye, hear ye! Medieval Mode is now " + QString(l_area->isMedievalMode() ? "enabled." : "disabled."));
}
