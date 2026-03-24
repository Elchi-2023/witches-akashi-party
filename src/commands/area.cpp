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
#include "server.h"

// This file is for commands under the area category in aoclient.h
// Be sure to register the command in the header before adding it here!

void AOClient::cmdCM(int argc, QStringList argv){
    AreaData *current_area = server->getAreaById(areaId());
    
    switch (argc){ /* switch were better */
    case 0:
        if (m_authenticated){ /* bypassed for an moderator/authenticated */
            if (current_area->owners().isEmpty()){  /*  no one owns this area, and it's not protected */
                current_area->addOwner(clientId());
                sendServerMessageArea(QString("[M][%1] %2 is now CM in this area.").arg(QString::number(clientId()), name()));
                arup(ARUPType::CM, true);
            }
            else if (current_area->owners().contains(clientId())) /* there is already a CM, and it isn't us */
                sendServerMessage("You are already a CM in this area.");
            else
                sendServerMessage("You cannot become a CM in this area.");
        }
        else if (current_area->isProtected())
            sendServerMessage("This area is protected, you may not become CM.");
        else if (current_area->owners().isEmpty()){  /*  no one owns this area, and it's not protected */
            current_area->addOwner(clientId());
            sendServerMessageArea(QString("[%1] %2 is now CM in this area.").arg(QString::number(clientId()), name()));
            arup(ARUPType::CM, true);
        }
        else if (current_area->owners().contains(clientId())) /* there is already a CM, and it isn't us */
            sendServerMessage("You are already a CM in this area.");
        else
            sendServerMessage("You cannot become a CM in this area.");
        break;
    case 1: default: /* doesn't hurts when argc >= 1, right?.. */
        bool vaild;
        const auto owner_candidate = server->getClientByID(argv[0].toInt(&vaild)); /* why const?.. because not been modified if just get info */
        
        if (m_authenticated){
            if (!vaild)
                sendServerMessage("That doesn't look like a valid ID.");
            else if (!checkPermission(ACLRole::CM)) /* preventing user for use this if their not an moderator/authenticated or in area owner */
                sendServerMessage("You must become a CM to use this command. (or you don't have \"CM\" permission");
            else if (owner_candidate.isNull())
                sendServerMessage("Unable to find client with ID " + argv[0] + ".");
            else if (current_area->owners().contains(owner_candidate->clientId()))
                sendServerMessage(QString("%1 already a CM in this area.").arg(owner_candidate->clientId() == clientId() ? "You are" : "User is"));
            else{
                current_area->addOwner(owner_candidate->clientId());
                sendServerMessageArea(QString("[%1] %2 is now CM in this area.").arg(QString::number(owner_candidate->clientId()), owner_candidate->name().isEmpty() ? owner_candidate->m_current_char.isEmpty() ? "Spectator" : owner_candidate->m_current_char : owner_candidate->name())); /* to fix "is now CM in this area.".. we just get target chars instead, target ooc-name otherwise. */
                arup(ARUPType::CM, true);
            }
        }
        else if (current_area->isProtected())
            sendServerMessage("This area is protected, you may not uses this Commands nor become CM.");
        else if (!vaild)
            sendServerMessage("That doesn't look like a valid ID.");
        else if (!checkPermission(ACLRole::CM)) /* preventing user for use this if their not an moderator/authenticated or in area owner */
            sendServerMessage("You must become a CM to use this command.");
        else if (owner_candidate == nullptr)
            sendServerMessage("Unable to find client with ID " + argv[0] + ".");
        else if (current_area->owners().contains(owner_candidate->clientId()))
            sendServerMessage(QString("%1 already a CM in this area.").arg(owner_candidate->clientId() == clientId() ? "You are" : "User is"));
        else{
            current_area->addOwner(owner_candidate->clientId());
            sendServerMessageArea(QString("[%1] %2 is now CM in this area.").arg(QString::number(owner_candidate->clientId()), owner_candidate->name().isEmpty() ? owner_candidate->m_current_char.isEmpty() ? "Spectator" : owner_candidate->m_current_char : owner_candidate->name())); /* to fix "is now CM in this area.".. we just get target chars instead, target ooc-name otherwise. */
            arup(ARUPType::CM, true);
        }
        break;
    }
}

void AOClient::cmdUnCM(int argc, QStringList argv){
    AreaData *current_area = server->getAreaById(areaId());

    if (current_area->owners().isEmpty())
        sendServerMessage("There are no CMs in this area.");
    else{
        switch (argc){ /* switch were winner again.. :) */
        case 0:
            if (current_area->removeOwner(clientId()))
                arup(ARUPType::LOCKED, true);
            sendServerMessage("You are no longer CM in this area.");
            arup(ARUPType::CM, true);
            break;
        case 1: default:
            if (!m_authenticated && !checkPermission(ACLRole::UNCM)){
                sendServerMessage("You do not have permission to unCM others.");
                return;
            }

            bool vaild_uid;
            int uid = argv[0].toInt(&vaild_uid);

            if (!vaild_uid)
                sendServerMessage("Invalid user ID.");
            else if (!current_area->owners().contains(uid))
                sendServerMessage("That user weren't CMed.");
            else if (server->getClientByID(uid).isNull())
                sendServerMessage("No client with that ID found.");
            else if (uid == clientId()){ /* imagine if someone want been uncm themself */
                if (current_area->removeOwner(clientId()))
                    arup(ARUPType::LOCKED, true);
                sendServerMessage("You are no longer CM in this area.");
                arup(ARUPType::CM, true);
            }
            else{
                auto target = server->getClientByID(uid);
                sendServerMessage(QString("[%1] %2 was successfully unCMed.").arg(QString::number(target->clientId()), target->name().isEmpty() ? target->m_current_char.isEmpty() ? "Spectator" : target->m_current_char : target->name()));
                target->sendServerMessage(QString("You have been unCMed by %1.").arg(m_authenticated ? "a moderator" : "a others CMs")); /* let target know who uncmed them between moderator and others cms */
                if (current_area->removeOwner(target->clientId()))
                    arup(ARUPType::LOCKED, true);
                arup(ARUPType::CM, true);
            }
        }
    }
}

void AOClient::cmdInvite(int argc, QStringList argv){
    Q_UNUSED(argc);

    bool VaildID;
    int l_invited_id = argv[0].toInt(&VaildID);
    AreaData *l_area = server->getAreaById(areaId());
    if (VaildID){
        auto target_client = server->getClientByID(l_invited_id);
        if (target_client.isNull())
            sendServerMessage("No client with that ID found.");
        else if (l_area->invite(l_invited_id)){
            sendServerMessage("You invited ID " + QString::number(target_client->clientId()));
            target_client->sendServerMessage("You were invited and given access to " + l_area->name());
        }
        else
            sendServerMessage("That ID is already on the invite list.");
    }
    else
        sendServerMessage("That does not look like a valid ID.");
}

void AOClient::cmdUnInvite(int argc, QStringList argv)
{
    Q_UNUSED(argc);

    bool VaildID;
    int l_uninvited_id = argv[0].toInt(&VaildID);
    AreaData *l_area = server->getAreaById(areaId());
    if (VaildID){
        auto target_client = server->getClientByID(l_uninvited_id);
        if (target_client.isNull())
            sendServerMessage("No client with that ID found.");
        else if (l_area->owners().contains(target_client->clientId()))
            sendServerMessage("You cannot uninvite a CM!");
        else if (l_area->uninvite(target_client->clientId())){
            sendServerMessage("You uninvited ID " + QString::number(target_client->clientId()));
            target_client->sendServerMessage("You were uninvited from " + l_area->name());
        }
        else
            sendServerMessage("That ID is not on the invite list.");
    }
    else
        sendServerMessage("That does not look like a valid ID.");
}

void AOClient::cmdLock(int argc, QStringList argv){
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    AreaData *area = server->getAreaById(areaId());
    if (area->lockStatus() == AreaData::LockStatus::LOCKED)
        sendServerMessage("This area is already locked.");
    else{
        sendServerMessageArea("This area is now locked.");
        area->lock();
        for (const int I : area->joinedIDs()){
            const auto Joined_Client = server->getClientByID(I);
            if (Joined_Client.isNull() || !Joined_Client->hasJoined())
                continue;
            area->invite(Joined_Client->clientId());
        }
        arup(ARUPType::LOCKED, true);
    }
}

void AOClient::cmdSpectatable(int argc, QStringList argv){
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    AreaData *l_area = server->getAreaById(areaId());
    if (l_area->lockStatus() == AreaData::LockStatus::SPECTATABLE)
        sendServerMessage("This area is already in spectate mode.");
    else{
        sendServerMessageArea("This area is now spectatable.");
        l_area->spectatable();
        for (const int I : l_area->joinedIDs()){
            const auto Joined_Client = server->getClientByID(I);
            if (Joined_Client.isNull() || !Joined_Client->hasJoined())
                continue;
            l_area->invite(Joined_Client->clientId());
        }
        arup(ARUPType::LOCKED, true);
    }
}

void AOClient::cmdUnLock(int argc, QStringList argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    AreaData *l_area = server->getAreaById(areaId());
    if (l_area->lockStatus() == AreaData::LockStatus::FREE)
        sendServerMessage("This area is not locked.");
    else{
        sendServerMessageArea("This area is now unlocked.");
        l_area->unlock();
        arup(ARUPType::LOCKED, true);
    }
}

void AOClient::cmdGetAreas(int argc, QStringList argv){
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    QStringList l_entries;
    l_entries.append("\n== Currently Online: " + QString::number(server->getPlayerCount()) + " ==");
    for (const AreaData *Area : server->getAreas()){
        if (Area->playerCount() >= 1)
            l_entries.append(buildAreaList(Area->index()));
    }
    sendServerMessage(l_entries.join("\n"));
}

void AOClient::cmdGetArea(int argc, QStringList argv){
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    sendServerMessage(buildAreaList(areaId()).join("\n"));
}

void AOClient::cmdArea(int argc, QStringList argv){
    Q_UNUSED(argc);

    bool VaildAreaID;
    int l_new_area = argv[0].toInt(&VaildAreaID);
    if (VaildAreaID && l_new_area >= 0 && l_new_area < server->getAreaCount())
        changeArea(l_new_area);
    else
        sendServerMessage("That does not look like a valid area ID.");
}

void AOClient::cmdAreaKick(int argc, QStringList argv){
    Q_UNUSED(argc);

    AreaData *l_area = server->getAreaById(areaId());

    bool VaildID;
    int l_idx = argv[0].toInt(&VaildID);
    if (VaildID){
        auto Target_kick = server->getClientByID(l_idx);

        if (Target_kick.isNull())
            sendServerMessage("No client with that ID found.");
        else if (server->getAreaById(Target_kick->areaId()) != l_area)
            sendServerMessage("That client is not in this area.");
        else if (l_area->owners().contains(Target_kick->clientId()))
            sendServerMessage("You cannot kick another CM!");
        else{
            Target_kick->changeArea(0);
            l_area->uninvite(Target_kick->clientId());
            sendServerMessage("Client ID " + QString::number(Target_kick->clientId()) + " kicked back to area 0.");
        }
    }
    else
        sendServerMessage("That does not look like a valid ID.");
}

void AOClient::cmdSetBackground(int argc, QStringList argv){
    Q_UNUSED(argc);

    QString f_background = argv.join(" ");
    AreaData *area = server->getAreaById(areaId());
    if (m_authenticated || !area->bgLocked()) {
        if (area->lockStatus() == AreaData::LockStatus::SPECTATABLE && !area->invited().contains(clientId()) && !checkPermission(ACLRole::BYPASS_LOCKS)) {
            sendServerMessage("Spectators are blocked from changing the background.");
            return;
        }
        if (server->getBackgrounds().contains(f_background, Qt::CaseInsensitive) || area->ignoreBgList() == true) {
            area->setBackground(f_background);
            server->broadcast(PacketFactory::createPacket("BN", {f_background, area->side()}), areaId());
            QString ambience_name = ConfigManager::ambience()->value(f_background + "/ambience").toString();
            server->broadcast(ambience_name.isEmpty() ? PacketFactory::createPacket("MC", {"~stop.mp3", "-1", characterName(), "0", "1"}) : PacketFactory::createPacket("MC", {ambience_name, "-1", characterName(), "1", "1"}), areaId());
            sendServerMessageArea(QString("[%1] %2 changed the background to %3").arg(QString::number(clientId()), character().isEmpty() ? "Spectator" : character(), f_background));
        }
        else
            sendServerMessage("Invalid background name.");
    }
    else
        sendServerMessage("This area's background is locked.");
}

void AOClient::cmdSetSide(int argc, QStringList argv){
    Q_UNUSED(argc);

    AreaData *area = server->getAreaById(areaId());
    if (area->bgLocked())
        sendServerMessage("This area's background is locked.");
    else{
        const QString side = argv.join(" ");
        area->setSide(side);
        server->broadcast(PacketFactory::createPacket("BN", {area->background(), side}), areaId());
        sendServerMessageArea(QString("[%1] %2 %3").arg(QString::number(clientId()), character().isEmpty() ? "Spectator" : character(), side.isEmpty() ? "unlocked the background side" : "locked the background side to " + side));
    }
}

void AOClient::cmdBgLock(int argc, QStringList argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    AreaData *l_area = server->getAreaById(areaId());

    if (!l_area->bgLocked())
        l_area->toggleBgLock();

    sendServerMessageArea(QString("[%1] %2 locked the background.").arg(QString::number(clientId()), character().isEmpty() ? "Spectator" : character()));
}

void AOClient::cmdBgUnlock(int argc, QStringList argv){
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    AreaData *l_area = server->getAreaById(areaId());

    if (l_area->bgLocked())
        l_area->toggleBgLock();

    sendServerMessageArea(QString("[%1] %2 unlocked the background.").arg(QString::number(clientId()), character().isEmpty() ? "Spectator" : character()));
}

void AOClient::cmdStatus(int argc, QStringList argv){
    Q_UNUSED(argc);

    AreaData *l_area = server->getAreaById(areaId());
    const QString l_arg = argv[0].toLower();

    if (l_area->changeStatus(l_arg)) {
        arup(ARUPType::STATUS, true);
        sendServerMessageArea(QString("[%1] %2 changed status to %3.").arg(QString::number(clientId()), character().isEmpty() ? "Spectator" : character(), l_arg.toUpper()));
    }
    else
        sendServerMessage("That does not look like a valid status. Valid statuses are: [" + AreaData::map_statuses.keys().join(", ") + "]");
}

void AOClient::cmdJudgeLog(int argc, QStringList argv){
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    AreaData *l_area = server->getAreaById(areaId());
    if (l_area->judgelog().isEmpty())
        sendServerMessage("There have been no judge actions in this area.");
    else
        sendServerMessage(checkPermission(ACLRole::KICK) || checkPermission(ACLRole::BAN) ? l_area->judgelog().join("\n") : l_area->judgelog().replaceInStrings(QRegularExpression("[(].*[)]"), "").join('\n'));
}

void AOClient::cmdIgnoreBgList(int argc, QStringList argv){
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    AreaData *l_area = server->getAreaById(areaId());
    l_area->toggleIgnoreBgList();
    sendServerMessage("BG list in this area is now " + QString(l_area->ignoreBgList() ? "ignored." : "enforced."));
}

void AOClient::cmdAreaMessage(int argc, QStringList argv){
    Q_UNUSED(argc)

    AreaData *l_area = server->getAreaById(areaId());
    if (argv.isEmpty())
        sendServerMessage(l_area->areaMessage());
    else{
        l_area->changeAreaMessage(argv.join(" "));
        sendServerMessage("Updated this area's message.");
    }
}

void AOClient::cmdToggleAreaMessageOnJoin(int argc, QStringList argv){
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    AreaData *l_area = server->getAreaById(areaId());
    l_area->toggleAreaMessageJoin();
    sendServerMessage("Sending message on area join is now " + QString(l_area->sendAreaMessageOnJoin() ? "enabled." : "disabled."));
}

void AOClient::cmdToggleWtce(int argc, QStringList argv){
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    AreaData *l_area = server->getAreaById(areaId());
    l_area->toggleWtceAllowed();
    sendServerMessage("Using testimony animations is now " + QString(l_area->isWtceAllowed() ? "enabled." : "disabled."));
}

void AOClient::cmdToggleShouts(int argc, QStringList argv){
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    AreaData *l_area = server->getAreaById(areaId());
    l_area->toggleShoutAllowed();
    sendServerMessage("Using shouts is now " + QString(l_area->isShoutAllowed() ? "enabled." : "disabled."));
}

void AOClient::cmdClearAreaMessage(int argc, QStringList argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    AreaData *l_area = server->getAreaById(areaId());
    l_area->clearAreaMessage();
    if (l_area->sendAreaMessageOnJoin())              // Turn off the automatic sending.
        cmdToggleAreaMessageOnJoin(0, QStringList{}); // Dummy values.
}

void AOClient::cmdWebfiles(int argc, QStringList argv){
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    QStringList l_weblinks;
    const auto clients = server->getAreaById(areaId())->joinedIDs();
    for (int Index : clients){
        if (!server->getClientByID(Index).isNull() && !server->getClientByID(Index)->character().isEmpty()){ /* it just me.. or AOClient::isSpectator() buggy?.. unsure yet.. */
            const auto l_client = server->getClientByID(Index);
            if (l_client->m_current_iniswap.isEmpty())
                l_weblinks.append(QString("%1 [%2] %3 using: %4").arg(l_client == this ? " ➤ " : " · ", QString::number(l_client->clientId()), l_client->characterName().isEmpty() ? l_client->name().isEmpty() ? "[Unknown]" : l_client->name() : l_client->characterName(), l_client->character()));
            else if (l_client->m_current_iniswap.toLower() != l_client->character().toLower()){
                QStringList m_name(l_client->character());
                if (!l_client->characterName().isEmpty() || !l_client->name().isEmpty())
                    m_name.append("(" + QString(l_client->characterName().isEmpty() ? l_client->name() : l_client->characterName()) + ")");
                l_weblinks.append(QString("%1 [%2] %3 using: %4").arg(l_client == this ? " ➤ " : " · ", QString::number(l_client->clientId()), m_name.join(' '), l_client->m_current_iniswap));
            }
        }
    }
    if (l_weblinks.isEmpty())
        sendPacket("CT", {"[Webfiles]", "theres nothing on the list.", "1"});
    else
        sendServerMessage(QString("\n=== [Webfiles] ===\n%1\n=== total: %2 ===\nIf you want to download any char or BG head to: %3").arg(l_weblinks.join('\n'), QString::number(l_weblinks.size()), ConfigManager::ServerWebdownloaderURL().toString()));
}

void AOClient::cmdMedievalMode(int argc, QStringList argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    AreaData *l_area = server->getAreaById(areaId());
    l_area->toggleMedievalMode();
    sendServerMessageArea("Hear ye, hear ye! Medieval Mode is now " + QString(l_area->isMedievalMode() ? "enabled." : "disabled."));
}
