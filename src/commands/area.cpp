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
        const AOClient *owner_candidate = server->getClientByID(argv[0].toInt(&vaild)); /* why const?.. because not been modified if just get info */
        
        if (m_authenticated){
            if (!vaild)
                sendServerMessage("That doesn't look like a valid ID.");
            else if (!checkPermission(ACLRole::CM)) /* preventing user for use this if their not an moderator/authenticated or in area owner */
                sendServerMessage("You must become a CM to use this command. (or you don't have \"CM\" permission");
            else if (owner_candidate == nullptr)
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
            else if (server->getClientByID(uid) == nullptr)
                sendServerMessage("No client with that ID found.");
            else if (uid == clientId()){ /* imagine if someone want been uncm themself */
                if (current_area->removeOwner(clientId()))
                    arup(ARUPType::LOCKED, true);
                sendServerMessage("You are no longer CM in this area.");
                arup(ARUPType::CM, true);
            }
            else{
                AOClient *target = server->getClientByID(uid);
                sendServerMessage(QString("[%1] %2 was successfully unCMed.").arg(QString::number(target->clientId()), target->name().isEmpty() ? target->m_current_char.isEmpty() ? "Spectator" : target->m_current_char : target->name()));
                target->sendServerMessage(QString("You have been unCMed by %1.").arg(m_authenticated ? "a moderator" : "a others CMs")); /* let target know who uncmed them between moderator and others cms */
                if (current_area->removeOwner(target->clientId()))
                    arup(ARUPType::LOCKED, true);
                arup(ARUPType::CM, true);
            }
        }
    }
}

void AOClient::cmdInvite(int argc, QStringList argv)
{
    Q_UNUSED(argc);

    AreaData *l_area = server->getAreaById(areaId());
    bool ok;
    int l_invited_id = argv[0].toInt(&ok);
    if (!ok) {
        sendServerMessage("That does not look like a valid ID.");
        return;
    }

    AOClient *target_client = server->getClientByID(l_invited_id);
    if (target_client == nullptr) {
        sendServerMessage("No client with that ID found.");
        return;
    }
    else if (!l_area->invite(l_invited_id)) {
        sendServerMessage("That ID is already on the invite list.");
        return;
    }
    sendServerMessage("You invited ID " + argv[0]);
    target_client->sendServerMessage("You were invited and given access to " + l_area->name());
}

void AOClient::cmdUnInvite(int argc, QStringList argv)
{
    Q_UNUSED(argc);

    AreaData *l_area = server->getAreaById(areaId());
    bool ok;
    int l_uninvited_id = argv[0].toInt(&ok);
    if (!ok) {
        sendServerMessage("That does not look like a valid ID.");
        return;
    }

    AOClient *target_client = server->getClientByID(l_uninvited_id);
    if (target_client == nullptr) {
        sendServerMessage("No client with that ID found.");
        return;
    }
    else if (l_area->owners().contains(l_uninvited_id)) {
        sendServerMessage("You cannot uninvite a CM!");
        return;
    }
    else if (!l_area->uninvite(l_uninvited_id)) {
        sendServerMessage("That ID is not on the invite list.");
        return;
    }
    sendServerMessage("You uninvited ID " + argv[0]);
    target_client->sendServerMessage("You were uninvited from " + l_area->name());
}

void AOClient::cmdLock(int argc, QStringList argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    AreaData *area = server->getAreaById(areaId());
    if (area->lockStatus() == AreaData::LockStatus::LOCKED) {
        sendServerMessage("This area is already locked.");
        return;
    }
    sendServerMessageArea("This area is now locked.");
    area->lock();
    const QVector<AOClient *> l_clients = server->getClients();
    for (AOClient *l_client : l_clients) {
        if (l_client->areaId() == areaId() && l_client->hasJoined()) {
            area->invite(l_client->clientId());
        }
    }
    arup(ARUPType::LOCKED, true);
}

void AOClient::cmdSpectatable(int argc, QStringList argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    AreaData *l_area = server->getAreaById(areaId());
    if (l_area->lockStatus() == AreaData::LockStatus::SPECTATABLE) {
        sendServerMessage("This area is already in spectate mode.");
        return;
    }
    sendServerMessageArea("This area is now spectatable.");
    l_area->spectatable();
    const QVector<AOClient *> l_clients = server->getClients();
    for (AOClient *l_client : l_clients) {
        if (l_client->areaId() == areaId() && l_client->hasJoined()) {
            l_area->invite(l_client->clientId());
        }
    }
    arup(ARUPType::LOCKED, true);
}

void AOClient::cmdUnLock(int argc, QStringList argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    AreaData *l_area = server->getAreaById(areaId());
    if (l_area->lockStatus() == AreaData::LockStatus::FREE) {
        sendServerMessage("This area is not locked.");
        return;
    }
    sendServerMessageArea("This area is now unlocked.");
    l_area->unlock();
    arup(ARUPType::LOCKED, true);
}

void AOClient::cmdGetAreas(int argc, QStringList argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    QStringList l_entries;
    l_entries.append("\n== Currently Online: " + QString::number(server->getPlayerCount()) + " ==");
    for (int i = 0; i < server->getAreaCount(); i++) {
        if (server->getAreaById(i)->playerCount() > 0) {
            QStringList l_cur_area_lines = buildAreaList(i);
            l_entries.append(l_cur_area_lines);
        }
    }
    sendServerMessage(l_entries.join("\n"));
}

void AOClient::cmdGetArea(int argc, QStringList argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    QStringList l_entries = buildAreaList(areaId());
    sendServerMessage(l_entries.join("\n"));
}

void AOClient::cmdArea(int argc, QStringList argv)
{
    Q_UNUSED(argc);

    bool ok;
    int l_new_area = argv[0].toInt(&ok);
    if (!ok || l_new_area >= server->getAreaCount() || l_new_area < 0) {
        sendServerMessage("That does not look like a valid area ID.");
        return;
    }
    changeArea(l_new_area);
}

void AOClient::cmdAreaKick(int argc, QStringList argv)
{
    Q_UNUSED(argc);

    AreaData *l_area = server->getAreaById(areaId());

    bool ok;
    int l_idx = argv[0].toInt(&ok);
    if (!ok) {
        sendServerMessage("That does not look like a valid ID.");
        return;
    }
    if (server->getAreaById(areaId())->owners().contains(l_idx)) {
        sendServerMessage("You cannot kick another CM!");
        return;
    }
    AOClient *l_client_to_kick = server->getClientByID(l_idx);
    if (l_client_to_kick == nullptr) {
        sendServerMessage("No client with that ID found.");
        return;
    }
    else if (l_client_to_kick->areaId() != areaId()) {
        sendServerMessage("That client is not in this area.");
        return;
    }
    l_client_to_kick->changeArea(0);
    l_area->uninvite(l_client_to_kick->clientId());

    sendServerMessage("Client " + argv[0] + " kicked back to area 0.");
}

void AOClient::cmdSetBackground(int argc, QStringList argv)
{
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
            if (ambience_name != "") {
                server->broadcast(PacketFactory::createPacket("MC", {ambience_name, "-1", characterName(), "1", "1"}), areaId());
            }
            else {
                server->broadcast(PacketFactory::createPacket("MC", {"~stop.mp3", "-1", characterName(), "0", "1"}), areaId());
            }
            sendServerMessageArea(character() + " changed the background to " + f_background);
        }
        else {
            sendServerMessage("Invalid background name.");
        }
    }
    else {
        sendServerMessage("This area's background is locked.");
    }
}

void AOClient::cmdSetSide(int argc, QStringList argv)
{
    Q_UNUSED(argc);

    AreaData *area = server->getAreaById(areaId());
    if (area->bgLocked()) {
        sendServerMessage("This area's background is locked.");
        return;
    }

    QString side = argv.join(" ");
    area->setSide(side);
    server->broadcast(PacketFactory::createPacket("BN", {area->background(), side}), areaId());
    if (side.isEmpty()) {
        sendServerMessageArea(character() + " unlocked the background side");
    }
    else {
        sendServerMessageArea(character() + " locked the background side to " + side);
    }
}

void AOClient::cmdBgLock(int argc, QStringList argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    AreaData *l_area = server->getAreaById(areaId());

    if (l_area->bgLocked() == false) {
        l_area->toggleBgLock();
    };

    server->broadcast(PacketFactory::createPacket("CT", {ConfigManager::serverTag(), character() + " locked the background.", "1"}), areaId());
}

void AOClient::cmdBgUnlock(int argc, QStringList argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    AreaData *l_area = server->getAreaById(areaId());

    if (l_area->bgLocked() == true) {
        l_area->toggleBgLock();
    };

    server->broadcast(PacketFactory::createPacket("CT", {ConfigManager::serverTag(), character() + " unlocked the background.", "1"}), areaId());
}

void AOClient::cmdStatus(int argc, QStringList argv)
{
    Q_UNUSED(argc);

    AreaData *l_area = server->getAreaById(areaId());
    QString l_arg = argv[0].toLower();

    if (l_area->changeStatus(l_arg)) {
        arup(ARUPType::STATUS, true);
        server->broadcast(PacketFactory::createPacket("CT", {ConfigManager::serverTag(), character() + " changed status to " + l_arg.toUpper(), "1"}), areaId());
    }
    else {
        const QStringList keys = AreaData::map_statuses.keys();
        sendServerMessage("That does not look like a valid status. Valid statuses are " + keys.join(", "));
    }
}

void AOClient::cmdJudgeLog(int argc, QStringList argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    AreaData *l_area = server->getAreaById(areaId());
    if (l_area->judgelog().isEmpty()) {
        sendServerMessage("There have been no judge actions in this area.");
        return;
    }
    QString l_message = l_area->judgelog().join("\n");
    // Judgelog contains an IPID, so we shouldn't send that unless the caller has appropriate permissions
    if (checkPermission(ACLRole::KICK) || checkPermission(ACLRole::BAN)) {
        sendServerMessage(l_message);
    }
    else {
        QString filteredmessage = l_message.remove(QRegularExpression("[(].*[)]")); // Filter out anything between two parentheses. This should only ever be the IPID
        sendServerMessage(filteredmessage);
    }
}

void AOClient::cmdIgnoreBgList(int argc, QStringList argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    AreaData *l_area = server->getAreaById(areaId());
    l_area->toggleIgnoreBgList();
    QString l_state = l_area->ignoreBgList() ? "ignored." : "enforced.";
    sendServerMessage("BG list in this area is now " + l_state);
}

void AOClient::cmdAreaMessage(int argc, QStringList argv)
{
    AreaData *l_area = server->getAreaById(areaId());
    if (argc == 0) {
        sendServerMessage(l_area->areaMessage());
        return;
    }

    if (argc >= 1) {
        l_area->changeAreaMessage(argv.join(" "));
        sendServerMessage("Updated this area's message.");
    }
}

void AOClient::cmdToggleAreaMessageOnJoin(int argc, QStringList argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    AreaData *l_area = server->getAreaById(areaId());
    l_area->toggleAreaMessageJoin();
    QString l_state = l_area->sendAreaMessageOnJoin() ? "enabled." : "disabled.";
    sendServerMessage("Sending message on area join is now " + l_state);
}

void AOClient::cmdToggleWtce(int argc, QStringList argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    AreaData *l_area = server->getAreaById(areaId());
    l_area->toggleWtceAllowed();
    QString l_state = l_area->isWtceAllowed() ? "enabled." : "disabled.";
    sendServerMessage("Using testimony animations is now " + l_state);
}

void AOClient::cmdToggleShouts(int argc, QStringList argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    AreaData *l_area = server->getAreaById(areaId());
    l_area->toggleShoutAllowed();
    QString l_state = l_area->isShoutAllowed() ? "enabled." : "disabled.";
    sendServerMessage("Using shouts is now " + l_state);
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

void AOClient::cmdWebfiles(int argc, QStringList argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    const QVector<AOClient *> l_clients = server->getClients();
    QStringList l_weblinks;
    for (const auto *l_client : l_clients){
        if (l_client->areaId() != areaId() || l_client->character().isEmpty())
            continue;
        if (l_client->m_current_iniswap.isEmpty())
            l_weblinks.append(QString("[%1] %2 using: %3").arg(QString::number(l_client->clientId()), l_client->characterName().isEmpty() ? l_client->name() : l_client->characterName(), l_client->character()));
        else if (!l_client->m_current_iniswap.isEmpty())
            l_weblinks.append(QString("[%1] %2 using: %3").arg(QString::number(l_client->clientId()), l_client->characterName().isEmpty() ? l_client->name().isEmpty() ? l_client->character() : l_client->name() : l_client->characterName(), l_client->m_current_iniswap));
            
    }
    if (l_weblinks.isEmpty())
        sendServerMessage("[Webfiles]: theres nothing on the list.");
    else
        sendServerMessage(QString("\n=== [Webfiles] ===\n%1\n=== total: %2 ===\nIf you want to download any char or BG head to: http://umineko.online/webDownloader/dist/").arg(l_weblinks.join('\n'), QString::number(l_weblinks.size())));
}

void AOClient::cmdMedievalMode(int argc, QStringList argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    AreaData *l_area = server->getAreaById(areaId());
    l_area->toggleMedievalMode();
    QString l_state = l_area->isMedievalMode() ? "enabled." : "disabled.";
    sendServerMessageArea("Hear ye, hear ye! Medieval Mode is now " + l_state);
}
