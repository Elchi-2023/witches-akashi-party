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
#include "packet/packet_factory.h"
#include "server.h"
#include "packet/packet_ct.h"

const QMap<QString, AOClient::CommandInfo> AOClient::COMMANDS{
    {"login", {{ACLRole::NONE}, 0, "authentication", &AOClient::cmdLogin}},
    {"getarea", {{ACLRole::NONE}, 0, "area", &AOClient::cmdGetArea}},
    {"getareas", {{ACLRole::NONE}, 0, "area", &AOClient::cmdGetAreas}},
    {"ban", {{ACLRole::BAN}, 3, "moderation", &AOClient::cmdBan}},
    {"kick", {{ACLRole::KICK}, 2, "moderation", &AOClient::cmdKick}},
    {"changeauth", {{ACLRole::SUPER}, 0, "authentication", &AOClient::cmdChangeAuth}},
    {"rootpass", {{ACLRole::SUPER}, 1, "authentication", &AOClient::cmdSetRootPass}},
    {"rootname", {{ACLRole::SUPER}, 0, "authentication", &AOClient::cmdChangeRootName}},
    {"background", {{ACLRole::NONE}, 1, "area", &AOClient::cmdSetBackground}},
    {"side", {{ACLRole::CM}, 0, "area", &AOClient::cmdSetSide}},
    {"lock_background", {{ACLRole::CM}, 0, "area", &AOClient::cmdBgLock}},
    {"unlock_background", {{ACLRole::CM}, 0, "area", &AOClient::cmdBgUnlock}},
    {"adduser", {{ACLRole::MODIFY_USERS}, 3, "authentication", &AOClient::cmdAddUser}},
    {"removeuser", {{ACLRole::MODIFY_USERS}, 1, "authentication", &AOClient::cmdRemoveUser}},
    {"listusers", {{ACLRole::MODIFY_USERS}, 0, "authentication", &AOClient::cmdListUsers}},
    {"setperms", {{ACLRole::MODIFY_USERS}, 2, "authentication", &AOClient::cmdSetPerms}},
    {"removeperms", {{ACLRole::MODIFY_USERS}, 1, "authentication", &AOClient::cmdRemovePerms}},
    {"listperms", {{ACLRole::NONE}, 0, "authentication", &AOClient::cmdListPerms}},
    {"logout", {{ACLRole::NONE}, 0, "authentication", &AOClient::cmdLogout}},
    {"pos", {{ACLRole::NONE}, 1, "messaging", &AOClient::cmdPos}},
    {"holiday", {{ACLRole::NONE}, 0, "messaging", &AOClient::cmdHoliday}},
    {"unholiday", {{ACLRole::NONE}, 0, "messaging", &AOClient::cmdUnHoliday}},
    {"pair", {{ACLRole::NONE}, 1, "messaging", &AOClient::cmdPair}},
    {"unpair", {{ACLRole::NONE}, 0, "messaging", &AOClient::cmdUnPair}},
    {"pair_order", {{ACLRole::NONE}, 1, "messaging", &AOClient::cmdPairOrder}},
    {"offset", {{ACLRole::NONE}, 0, "messaging", &AOClient::cmdOffset}},
    {"g", {{ACLRole::NONE}, 1, "messaging", &AOClient::cmdG}},
    {"need", {{ACLRole::NONE}, 1, "messaging", &AOClient::cmdNeed}},
    {"coinflip", {{ACLRole::NONE}, 0, "roleplay", &AOClient::cmdFlip}},
    {"roll", {{ACLRole::NONE}, 0, "roleplay", &AOClient::cmdRoll}},
    {"rolla", {{ACLRole::NONE}, 0, "roleplay", &AOClient::cmdRollA}},
    {"rollp", {{ACLRole::NONE}, 0, "roleplay", &AOClient::cmdRollP}},
    {"wheel", {{ACLRole::NONE}, 0, "roleplay", &AOClient::cmdWheel}},
    {"wheelp", {{ACLRole::NONE}, 0, "roleplay", &AOClient::cmdWheelP}},
    {"rps", {{ACLRole::NONE}, 1, "roleplay", &AOClient::cmdRps}},
    {"doc", {{ACLRole::NONE}, 0, "casing", &AOClient::cmdDoc}},
    {"cleardoc", {{ACLRole::NONE}, 0, "casing", &AOClient::cmdClearDoc}},
    {"cm", {{ACLRole::NONE}, 0, "area", &AOClient::cmdCM}},
    {"uncm", {{ACLRole::CM}, 0, "area", &AOClient::cmdUnCM}},
    {"invite", {{ACLRole::CM}, 1, "area", &AOClient::cmdInvite}},
    {"uninvite", {{ACLRole::CM}, 1, "area", &AOClient::cmdUnInvite}},
    {"area_lock", {{ACLRole::CM}, 0, "area", &AOClient::cmdLock}},
    {"area_spectate", {{ACLRole::CM}, 0, "area", &AOClient::cmdSpectatable}},
    {"area_unlock", {{ACLRole::CM}, 0, "area", &AOClient::cmdUnLock}},
    {"timer", {{ACLRole::NONE}, 0, "roleplay", &AOClient::cmdTimer}},
    {"area", {{ACLRole::NONE}, 1, "area", &AOClient::cmdArea}},
    {"play", {{ACLRole::NONE}, 1, "music", &AOClient::cmdPlay}},
    {"radio", {{ACLRole::NONE}, 0, "music", &AOClient::cmdRadio}},
    {"area_kick", {{ACLRole::CM}, 1, "area", &AOClient::cmdAreaKick}},
    {"randomchar", {{ACLRole::NONE}, 0, "messaging", &AOClient::cmdRandomChar}},
    {"switch", {{ACLRole::NONE}, 1, "messaging", &AOClient::cmdSwitch}},
    {"toggleglobal", {{ACLRole::NONE}, 0, "messaging", &AOClient::cmdToggleGlobal}},
    {"mods", {{ACLRole::NONE}, 0, "moderation", &AOClient::cmdMods}},
    {"commands", {{ACLRole::NONE}, 0, "misc", &AOClient::cmdCommands}},
    {"status", {{ACLRole::NONE}, 1, "area", &AOClient::cmdStatus}},
    {"forcepos", {{ACLRole::CM}, 2, "messaging", &AOClient::cmdForcePos}},
    {"currentmusic", {{ACLRole::NONE}, 0, "music", &AOClient::cmdCurrentMusic}},
    {"getmusic",  {{ACLRole::NONE}, 0, "music", &AOClient::cmdGetMusic}},
    {"pm", {{ACLRole::NONE}, 2, "messaging", &AOClient::cmdPM}},
    {"play_once", {{ACLRole::NONE}, 1, "music", &AOClient::cmdPlayOnce}},
    {"evidence_mod", {{ACLRole::EVI_MOD}, 1, "casing", &AOClient::cmdEvidenceMod}},
    {"motd", {{ACLRole::NONE}, 0, "misc", &AOClient::cmdMOTD}},
    {"set_motd", {{ACLRole::MOTD}, 1, "misc", &AOClient::cmdSetMOTD}},
    {"announce", {{ACLRole::ANNOUNCE}, 1, "messaging", &AOClient::cmdAnnounce}},
    {"m", {{ACLRole::MODCHAT}, 1, "messaging", &AOClient::cmdM}},
    {"gm", {{ACLRole::MODCHAT}, 1, "messaging", &AOClient::cmdGM}},
    {"mute", {{ACLRole::MUTE}, 1, "moderation", &AOClient::cmdMute}},
    {"unmute", {{ACLRole::MUTE}, 1, "moderation", &AOClient::cmdUnMute}},
    {"bans", {{ACLRole::BAN}, 0, "moderation", &AOClient::cmdBans}},
    {"unban", {{ACLRole::BAN}, 1, "moderation", &AOClient::cmdUnBan}},
    {"subtheme", {{ACLRole::CM}, 1, "misc", &AOClient::cmdSubTheme}},
    {"about", {{ACLRole::NONE}, 0, "misc", &AOClient::cmdAbout}},
    {"evidence_swap", {{ACLRole::CM}, 2, "casing", &AOClient::cmdEvidence_Swap}},
    {"notecard", {{ACLRole::NONE}, 1, "roleplay", &AOClient::cmdNoteCard}},
    {"notecard_reveal", {{ACLRole::CM}, 0, "roleplay", &AOClient::cmdNoteCardReveal}},
    {"notecard_clear", {{ACLRole::NONE}, 0, "roleplay", &AOClient::cmdNoteCardClear}},
    {"8ball", {{ACLRole::NONE}, 1, "roleplay", &AOClient::cmd8Ball}},
    {"lm", {{ACLRole::MODCHAT}, 1, "messaging", &AOClient::cmdLM}},
    {"judgelog", {{ACLRole::CM}, 0, "area", &AOClient::cmdJudgeLog}},
    {"allow_blankposting", {{ACLRole::MODCHAT}, 0, "moderation", &AOClient::cmdAllowBlankposting}},
    {"baninfo", {{ACLRole::BAN}, 1, "moderation", &AOClient::cmdBanInfo}},
    {"testify", {{ACLRole::CM}, 0, "casing", &AOClient::cmdTestify}},
    {"testimony", {{ACLRole::NONE}, 0, "casing", &AOClient::cmdTestimony}},
    {"examine", {{ACLRole::CM}, 0, "casing", &AOClient::cmdExamine}},
    {"pause", {{ACLRole::CM}, 0, "casing", &AOClient::cmdPauseTestimony}},
    {"delete", {{ACLRole::CM}, 0, "casing", &AOClient::cmdDeleteStatement}},
    {"update", {{ACLRole::CM}, 0, "casing", &AOClient::cmdUpdateStatement}},
    {"add", {{ACLRole::CM}, 0, "casing", &AOClient::cmdAddStatement}},
    {"reload", {{ACLRole::SUPER}, 0, "server", &AOClient::cmdReload}},
    {"force_noint_pres", {{ACLRole::CM}, 0, "moderation", &AOClient::cmdForceImmediate}},
    {"allow_iniswap", {{ACLRole::CM}, 0, "moderation", &AOClient::cmdAllowIniswap}},
    {"afk", {{ACLRole::NONE}, 0, "misc", &AOClient::cmdAfk}},
    {"savetestimony", {{ACLRole::NONE}, 1, "casing", &AOClient::cmdSaveTestimony}},
    {"loadtestimony", {{ACLRole::CM}, 1, "casing", &AOClient::cmdLoadTestimony}},
    {"permitsaving", {{ACLRole::MODCHAT}, 1, "casing", &AOClient::cmdPermitSaving}},
    {"mutepm", {{ACLRole::NONE}, 0, "messaging", &AOClient::cmdMutePM}},
    {"toggleadverts", {{ACLRole::NONE}, 0, "messaging", &AOClient::cmdToggleAdverts}},
    {"toggleafkmute", {{ACLRole::NONE}, 0, "messaging", &AOClient::cmdToggleAfkMute}},
    {"toggleafk_announce", {{ACLRole::NONE}, 0, "messaging", &AOClient::cmdToggleAfkannounce}},
    {"block_wtce", {{ACLRole::MUTE}, 1, "moderation", &AOClient::cmdBlockWtce}},
    {"unblock_wtce", {{ACLRole::MUTE}, 1, "moderation", &AOClient::cmdUnBlockWtce}},
    {"block_dj", {{ACLRole::MUTE}, 1, "music", &AOClient::cmdBlockDj}},
    {"unblock_dj", {{ACLRole::MUTE}, 1, "music", &AOClient::cmdUnBlockDj}},
    {"charcurse", {{ACLRole::MUTE}, 1, "messaging", &AOClient::cmdCharCurse}},
    {"uncharcurse", {{ACLRole::MUTE}, 1, "messaging", &AOClient::cmdUnCharCurse}},
    {"charselect", {{ACLRole::NONE}, 0, "messaging", &AOClient::cmdCharSelect}},
    {"force_charselect", {{ACLRole::FORCE_CHARSELECT}, 1, "messaging", &AOClient::cmdForceCharSelect}},
    {"togglemusic", {{ACLRole::CM}, 0, "music", &AOClient::cmdToggleMusic}},
    {"a", {{ACLRole::NONE}, 2, "messaging", &AOClient::cmdA}},
    {"s", {{ACLRole::NONE}, 0, "messaging", &AOClient::cmdS}},
    {"firstperson", {{ACLRole::NONE}, 0, "misc", &AOClient::cmdFirstPerson}},
    {"update_ban", {{ACLRole::BAN}, 3, "moderation", &AOClient::cmdUpdateBan}},
    {"changepass", {{ACLRole::NONE}, 1, "authentication", &AOClient::cmdChangePassword}},
    {"ignore_bglist", {{ACLRole::IGNORE_BGLIST}, 0, "area", &AOClient::cmdIgnoreBgList}},
    {"notice", {{ACLRole::SEND_NOTICE}, 1, "moderation", &AOClient::cmdNotice}},
    {"noticeg", {{ACLRole::SEND_NOTICE}, 1, "moderation", &AOClient::cmdNoticeGlobal}},
    {"togglejukebox", {{ACLRole::CM, ACLRole::JUKEBOX}, 0, "music", &AOClient::cmdToggleJukebox}},
    {"help", {{ACLRole::NONE}, 0, "misc", &AOClient::cmdHelp}},
    {"clearcm", {{ACLRole::KICK}, 0, "moderation", &AOClient::cmdClearCM}},
    {"togglemessage", {{ACLRole::CM}, 0, "area", &AOClient::cmdToggleAreaMessageOnJoin}},
    {"clearmessage", {{ACLRole::CM}, 0, "area", &AOClient::cmdClearAreaMessage}},
    {"areamessage", {{ACLRole::CM}, 0, "area", &AOClient::cmdAreaMessage}},
    {"webfiles", {{ACLRole::NONE}, 0, "misc", &AOClient::cmdWebfiles}},
    {"addsong", {{ACLRole::CM}, 1, "music", &AOClient::cmdAddSong}},
    {"addcategory", {{ACLRole::CM}, 1, "music", &AOClient::cmdAddCategory}},
    {"removeentry", {{ACLRole::CM}, 1, "music", &AOClient::cmdRemoveCategorySong}},
    {"toggleroot", {{ACLRole::CM}, 0, "music", &AOClient::cmdToggleRootlist}},
    {"clearcustom", {{ACLRole::CM}, 0, "music", &AOClient::cmdClearCustom}},
    {"toggle_wtce", {{ACLRole::CM}, 0, "area", &AOClient::cmdToggleWtce}},
    {"toggle_shouts", {{ACLRole::CM}, 0, "area", &AOClient::cmdToggleShouts}},
    {"kick_other", {{ACLRole::NONE}, 0, "misc", &AOClient::cmdKickOther}},
    {"jukebox_skip", {{ACLRole::CM}, 0, "music", &AOClient::cmdJukeboxSkip}},
    {"randomsong", {{ACLRole::CM}, 0, "music", &AOClient::cmdRandomSong}},
    {"jukebox_shuffle", {{ACLRole::NONE}, 0, "music", &AOClient::cmdJukeboxShuffle}},
    {"jukebox_add", {{ACLRole::NONE}, 1, "music", &AOClient::cmdJukeboxAdd}},
    {"jukebox_remove", {{ACLRole::NONE}, 1, "music", &AOClient::cmdJukeboxRemove}},
    {"jukebox", {{ACLRole::NONE}, 0, "music", &AOClient::cmdJukeboxQueues}},
    {"play_ambience", {{ACLRole::NONE}, 1, "music", &AOClient::cmdPlayAmbience}},
    {"medievalmode", {{ACLRole::MUTE}, 0, "area", &AOClient::cmdMedievalMode}},
    {"curses", {{ACLRole::MUTE}, 1, "moderation", &AOClient::cmdCurses}},
    {"uncurses", {{ACLRole::MUTE}, 1, "moderation", &AOClient::cmdUnCurses}},
    /* > lockdown command < */
    {"lockdown", {{ACLRole::BAN, ACLRole::SUPER}, 0, "server", &AOClient::cmdlockdown}},
    {"lockdownlist", {{ACLRole::BAN, ACLRole::SUPER}, 0, "server", &AOClient::cmdlockdownlist}},
    /* > voice-chat command < */
    {"vblock", {{ACLRole::MUTE}, 1, "voice", &AOClient::cmdVBlock}},
    {"vublock", {{ACLRole::MUTE}, 1, "voice", &AOClient::cmdVUBlock}},
    {"vkick", {{ACLRole::NONE}, 1, "voice", &AOClient::cmdVKick}}
}; // {[command], {{ALCRole(s)}, [min_argv], [category], &[AOClient-command]}

void AOClient::clientDisconnected(){
    qDebug().nospace() << "[I][AKASHI][NET-CLIENT][" << m_ipid << "]: disconnected.";
    if (m_joined) {
        auto current_area = server->getAreaById(areaId());
        current_area->removeClient(clientId());

        /*> broadcasting (based from the AuthenticateType) to same AuthenticateType in current area <
         * example: [root] broadcasting "hello world" to [Moderator/root]..
         */
        server->broadcastCAuth(PacketCT::CreateMessageS(QString("%1 disconnected.").arg(AOClient::NameWId(this))), AuthenticateType::NONE, current_area->index()); // [user]
        server->broadcastCAuth(PacketCT::CreateMessageS(QString("%1 %2").arg(AOClient::NameWId(this), QStringList({"disconnected.", "is went out!", "been send in #DOOM world!"})[m_disconnect_reason])), AuthenticateType::VIP, current_area->index()); // [vip]..
        server->broadcastCAuth(PacketCT::CreateMessageS(QString("%1 %2").arg(AOClient::NameWId(this) + " (" + getIpid() + ")", QStringList({"disconnected", "disconnected: (kicked)", "disconnected (banned)"})[m_disconnect_reason])), AuthenticateType::MODERATOR, current_area->index()); // [moderator/root]..

        if (current_area->checkPairSync(clientId()) && current_area->checkPairSync(clientId(), true) && current_area->get_pair_sync_clientID(clientId()) == current_area->get_pair_sync_clientID(clientId(), true)){ /* > when someone actually pair-synced with this client < */
            if (!server->getClientByID(current_area->get_pair_sync_clientID(clientId(), true)).isNull()) // let's notify about this client not longer exist in pair-sync..
                server->getClientByID(current_area->get_pair_sync_clientID(clientId(), true))->sendServerMessageArea(QString("You aren't pair synced with %1, reseting..").arg(AOClient::NameWId(this)));
            current_area->removePairSync(clientId(), current_area->get_pair_sync_clientID(clientId(), true)); /* freed both ids from pairs sync list.. */
        }

        if (current_area->checkPairSync(clientId())) /* double checks if user client id weren't on pairs sync list */
            current_area->removePairSync(clientId());

        if (current_area->RegisterVoice(clientId(), true)) /* unregister user from vc.. */
            server->broadcastVJoinLeave(clientId(), true, current_area->index());

        server->updateCharsTaken(current_area);
        server->RemoveDisconnectCA(clientId());
        arup(ARUPType::PLAYER_COUNT, true);
        arup(ARUPType::LOCKED, true);
        arup(ARUPType::CM, true);
    }
    Q_EMIT clientSuccessfullyDisconnected(this);
}

void AOClient::ForcedDisconnected(const QString &reason){
    if (!reason.isEmpty())
        sendPacket("KK", {reason});
    m_socket->close();
}

void AOClient::handlePacket(AOPacket *packet){
#ifdef NET_DEBUG
    /* well.. the output will be like this "[D][AKASHI][NET-PACKET][<client_id>:<client_ipid>]: [<header>] (<data/content (if persent)>)" but.. it'll much outputing in console.. */
    if (packet->getPacketInfo().header.compare("ch", Qt::CaseInsensitive) != 0 /* preventing outputing the <CH> packet from client.. (otherwise it'll bloating the console..) */)
        qDebug().noquote() << QString("[D][AKASHI][NET-PACKET][%1:%2]: [%3] %4").arg(QString::number(clientId()), getIpid(), packet->getPacketInfo().header, packet->getContent().isEmpty() ? "" : "(" + packet->getContent().join(", ") + ")");
#endif
    const QPair<int, int> ratelimts{ConfigManager::packetRateLimitSoft(), ConfigManager::packetRateLimitHard()};
    qint64 current_tick = QDateTime::currentSecsSinceEpoch();
    if (rate_limit_tick < current_tick) {
        rate_limit_tick = current_tick;
        packet_count = 0;
    }

    ++packet_count;

    if (ratelimts.second > 0 && packet_count >= ratelimts.second) {
        sendPacket("BD", {"You have been disconnected for sending messages too quickly."});
        m_socket->close();
        qInfo().noquote() << QString("[I][AKASHI][NET-CLIENT]: Kicking an [%1] (%2) due of rate-limts reached.").arg(QString::number(clientId()), m_ipid);
        return;
    }
    else if (ratelimts.first > 0 && packet_count >= ratelimts.first)
        sendServerMessage("You are sending messages too quickly. Please slow down.");

    auto l_area = server->getAreaById(areaId());

    if (packet->getContent().join("").size() > 16384 || !checkPermission(packet->getPacketInfo().acl_permission) || l_area.isNull())
        return;

    if (packet->getPacketInfo().header.compare("ch", Qt::CaseInsensitive) != 0 && m_joined) {
        if (UserAFK()){
            if (m_afk_announcement){
                for (const int client_id : l_area->joinedIDs()){
                    auto l_client = server->getClientByID(client_id);
                    if (l_client.isNull())
                        continue;

                    if (l_client == this) /* "this" ... current client (aka user) lol */
                        l_client->sendServerMessage("You are no longer AFK, Welcome back.");
                    else if (!l_client->isSpectator() && l_client->m_afk_received) /* lgnored spectator for moment.. */
                        l_client->sendServerMessage(QString("[%1] %2 are no longer AFK.").arg(QString::number(clientId()), character().isEmpty() ? "[Spectator]" : character()));
                }
            }
            else
                sendServerMessage("You are no longer AFK. (unannouncement)");
            ToggleAFK(false);
        }
        m_afk_timer->start(ConfigManager::afkTimeout() * 1000);
    }

    if (packet->getContent().length() < packet->getPacketInfo().min_args) {
#ifdef NET_DEBUG
        qDebug().nospace() << QString("[D][AKASHI][NET-PACKET]: \"Invalid packet args length for heeader %1 from client %2 (%3), Minimum is %4 but only %5 were given.\"").arg(packet->getPacketInfo().header, AOClient::NameWId(this), m_ipid, QString::number(packet->getPacketInfo().min_args), QString::number(packet->getContent().length()));
#endif
        return;
    }

    packet->handlePacket(l_area, *this);
}

void AOClient::changeArea(int new_area)
{
    auto target_area = server->getAreaById(new_area);
    auto previous_area = server->getAreaById(areaId());

    if (target_area.isNull() || previous_area.isNull())
        return;

    if (previous_area == target_area) {
        sendServerMessage("You are already in area " + target_area->name() + ".");
        return;
    }

    // check if we're allowed to enter the target area
    if (target_area->lockStatus() == AreaData::LockStatus::LOCKED && !target_area->invited().contains(clientId()) && !checkPermission(ACLRole::BYPASS_LOCKS)) {
        sendServerMessage("Area " + server->getAreaName(new_area) + " is locked.");
        return;
    }

    previous_area->removeClient(clientId());
    if (previous_area->RegisterVoice(clientId(), true)) /* unregister user from vc.. */
        server->broadcastVJoinLeave(clientId(), true, previous_area->index());

    server->updateCharsTaken(previous_area);

    if (target_area->charactersTaken().contains(server->getCharID(character()))){ /* our character is already being used here, so we'll have to spectate, until we pick a new one (or the other player leaves).. */
        changeCharacter(-1);
        sendPacket("DONE"); // If our character was taken, force us into character select
    }

    target_area->addClient(clientId(), m_char_id);
    setAreaId(new_area);

    arup(ARUPType::PLAYER_COUNT, true);

    sendEvidenceList(target_area);
    sendPacket("HP", {"1", QString::number(target_area->defHP())});
    sendPacket("HP", {"2", QString::number(target_area->proHP())});
    sendPacket("BN", {target_area->background(), target_area->side()});

    const QList<QTimer *> timers = target_area->timers();
    for (QTimer *timer : timers) {
        const int timer_id = target_area->timers().indexOf(timer) + 1;
        if (timer->isActive()) {
            sendPacket("TI", {QString::number(timer_id), "2"});
            sendPacket("TI", {QString::number(timer_id), "0", QString::number(timer->remainingTimeAsDuration().count())});
        }
        else {
            sendPacket("TI", {QString::number(timer_id), "3"});
        }
    }

    sendServerMessage("You moved to area " + server->getAreaName(areaId()) + ".");

    // > cleaning up pair sync from the old area.. <
    if (previous_area->checkPairSync(clientId())) {
        sendServerMessage("Your pair sync has been reset (you changed areas).");

        auto partner = server->getClientByID(previous_area->get_pair_sync_clientID(clientId()));
        if (!partner.isNull() && previous_area->removePairSync(partner->clientId(), clientId())) /* > let the partner/target know the user moved on < */
            partner->sendServerMessage(QString("You are no longer synced with %1 — they moved to another area.").arg(AOClient::NameWId(partner)));
        previous_area->removePairSync(clientId());
    }

    if (target_area->sendAreaMessageOnJoin() && !target_area->areaMessage().isEmpty())
        sendServerMessage(target_area->areaMessage());

    if (target_area->lockStatus() == AreaData::LockStatus::SPECTATABLE)
        sendServerMessage("Area " + server->getAreaName(areaId()) + " is spectate-only. To chat IC, you'll need an invitation from the CM.");
}

bool AOClient::changeCharacter(int char_id){
    auto l_area = server->getAreaById(areaId());
    const int index = qMax(-1, char_id), currentCount = server->getCharacterCount() -1;

    if (l_area.isNull() || index > currentCount || (isCursed(CurseType::CCURSE) && !m_charcurse_list.contains(index)))
        return false;

    const bool l_pass = l_area->changeCharacter(clientId(), index);

    if (l_pass){
        m_char_id = index;
        setCharacter(server->getCharacterById(index));
        m_pos.clear();
        sendPacket("PV", {QString::number(clientId()), "CID", QString::number(index)});
    }
    return l_pass;
}

void AOClient::changePosition(QString new_pos){
    if (m_pos == new_pos)
        sendServerMessage(QString("You already in %1 position.").arg(m_pos));
    else{
        m_pos = new_pos;
        sendServerMessage("Position changed to " + m_pos + ".");
        sendPacket("SP", {m_pos});
    }
}

void AOClient::handleCommand(const QPair<QString, QStringList> &command){
    QString l_target_command = command.first;
    if (l_target_command.isEmpty()){
        sendServerMessage("Please insert an full command param than just \"/\".");
        return;
    }
    QVector<ACLRole::Permission> l_permissions;

    // check for aliases
    const QList<CommandExtension> l_extensions = server->getCommandExtensionCollection()->getExtensions();
    for (const CommandExtension &i_extension : l_extensions) {
        if (i_extension.checkCommandNameAndAlias(command.first)) {
            l_target_command = i_extension.getCommandName();
            l_permissions = i_extension.getPermissions();
            break;
        }
    }

    const CommandInfo l_command = COMMANDS.value(l_target_command, {{ACLRole::NONE}, -1, QString(), &AOClient::cmdDefault});
    if (l_permissions.isEmpty())
        l_permissions.append(l_command.acl_permissions);

    bool l_has_permissions = false;
    for (const ACLRole::Permission i_permission : qAsConst(l_permissions)) {
        if (checkPermission(i_permission)) {
            l_has_permissions = true;
            break;
        }
    }

    if (l_has_permissions){
        if (command.second.size() >= l_command.minArgs)
            (this->*(l_command.action))(command.second.size(), command.second);
        else{
            sendServerMessage("Invalid command syntax.");
            if (!ConfigManager::commandHelp(command.first).usage.isEmpty())
                sendServerMessage("The expected syntax for this command is:\r\n" + ConfigManager::commandHelp(command.first).usage.replace('\n', "\r\n"));
        }
    }
    else
        sendServerMessage("You do not have permission to use that command.");
}

void AOClient::arup(ARUPType type, bool broadcast)
{
    QStringList l_arup_data;
    l_arup_data.append(QString::number(type));
    const QVector<QPointer<AreaData>> l_areas = server->getAreas();
    for (auto l_area : l_areas) {
        switch (type) {
        case ARUPType::PLAYER_COUNT:
            l_arup_data.append(QString::number(l_area.isNull() ? 0 : l_area->playerCount()));
            break;
        case ARUPType::STATUS:
            l_arup_data.append(l_area.isNull() ? "IDLE" : QVariant::fromValue(l_area->status()).toString().replace("_", "-")); // LOOKING_FOR_PLAYERS to LOOKING-FOR-PLAYERS
            break;
        case ARUPType::CM:
            if (l_area.isNull() || l_area->owners().isEmpty())
                l_arup_data.append("FREE");
            else{
                QStringList l_area_owners;
                const QList<int> l_owner_ids = l_area->owners();
                for (int l_owner_id : l_owner_ids){
                    auto l_owner = server->getClientByID(l_owner_id);
                    if (!l_owner.isNull())
                        l_area_owners.append("[" + QString::number(l_owner->clientId()) + "] " + l_owner->character());
                }
                l_arup_data.append(l_area_owners.join(", "));
            }
            break;
        case ARUPType::LOCKED:
            l_arup_data.append(l_area.isNull() ? "" : QVariant::fromValue(l_area->lockStatus()).toString());
            break;
        default:
            return;
        }
    }

    broadcast ? server->broadcast(PacketFactory::createPacket("ARUP", l_arup_data)) : sendPacket("ARUP", l_arup_data);
}

void AOClient::fullArup()
{
    arup(ARUPType::PLAYER_COUNT, false);
    arup(ARUPType::STATUS, false);
    arup(ARUPType::CM, false);
    arup(ARUPType::LOCKED, false);
}

void AOClient::sendPacket(AOPacket *packet)
{
    m_socket->write(packet);
}

void AOClient::sendPacket(QSharedPointer<AOPacket> packet){
    if (packet.isNull())
        return;
    m_socket->write(packet.get());
}

void AOClient::sendPacket(QString header, QStringList contents)
{
    sendPacket(PacketFactory::createPacket(header, contents));
}

void AOClient::sendPacket(QString header)
{
    sendPacket(PacketFactory::createPacket(header, {}));
}

void AOClient::sendPacket(AOPacket *packet, const AOClient::AuthenticateType AuthType){
    switch (AuthType){
    case AOClient::AuthenticateType::NONE:
        if (!isAuthenticated())
            sendPacket(packet);
        break;
    case AOClient::AuthenticateType::VIP:
        if (isVAuthenticated())
            sendPacket(packet);
        break;
    default:
        if (m_authenticated_type >= AuthType)
            sendPacket(packet);
        break;
    }
}
void AOClient::sendPacket(AOPacket *packet, const AOClient::AuthenticateType AuthType, const int areaID){
    const QPointer<AreaData> area = server->getAreaById(areaID);

    if (area.isNull() || area->index() != areaId() || !area->joinedIDs().contains(m_id))
        return;

    switch (AuthType){
    case AOClient::AuthenticateType::NONE:
        if (!isAuthenticated())
            sendPacket(packet);
        break;
    case AOClient::AuthenticateType::VIP:
        if (isVAuthenticated())
            sendPacket(packet);
        break;
    default:
        if (m_authenticated_type >= AuthType)
            sendPacket(packet);
        break;
    }
}

void AOClient::sendAudioFrame(const int c_from, const QByteArray &frame_byte, const int area_id){
    const QPointer<AreaData> area = server->getAreaById(area_id);
    if (area.isNull() || area->index() != areaId() || c_from == clientId())
        return;

    const auto current_vc = area->GetRegisteredVoiceMap();
    if (!current_vc.contains(clientId()) || !current_vc[clientId()])
        return; // not in vc, reject..

    sendPacket("VS_AUDIO", {QString::number(c_from), frame_byte});
}
void AOClient::sendAudioState(const int c_from, const bool state, const int area_id){
    const QPointer<AreaData> area = server->getAreaById(area_id);
    if (area.isNull() || area->index() != areaId() || !area->GetRegisteredVoiceID().contains(clientId()))
        return;

    sendPacket("VS_SPEAK", {QString::number(c_from), QString::number(state)});
}
void AOClient::sendAudioJoinLeave(const int c_from, const bool isleave, const int area_id){
    const QPointer<AreaData> area = server->getAreaById(area_id);
    if (!area.isNull() && area->index() == areaId()){
        if (isleave)
            sendPacket("VS_LEAVE", {QString::number(c_from)}); // isleave is true and c_from is this clientid.. well.. guess could counts as "refresh" after leave..
        else{
            sendPacket("VS_JOIN", {QString::number(c_from)});
            sendPacket("VS_PEERS", area->GetRegisteredVoice(true));
        }
    }
}

QString AOClient::calculateIpid()
{
    // TODO: add support for longer ipids?
    // This reduces the (fairly high) chance of
    // birthday paradox issues arising. However,
    // typing more than 8 characters might be a
    // bit cumbersome.

    QCryptographicHash hash(QCryptographicHash::Md5); // Don't need security, just hashing for uniqueness

    hash.addData(m_remote_ip.toString().toUtf8());

    return m_ipid = hash.result().toHex().right(8); // Use the last 8 characters (4 bytes)
}

QString AOClient::calculateIpid(const QHostAddress r_ip){
    QCryptographicHash hash(QCryptographicHash::Md5);
    hash.addData(r_ip.toString().toUtf8());
    return hash.result().toHex().right(8);
}

void AOClient::sendServerMessage(QString message, const QString cname)
{
    sendPacket(PacketCT::CreateMessageS(message, cname));
}

void AOClient::sendServerMessageArea(QString message, const QString CName)
{
    server->broadcast(PacketCT::CreateMessageS(message, CName), areaId());
}

void AOClient::sendServerBroadcast(QString message)
{
    server->broadcast(PacketCT::CreateMessageS(message));
}

void AOClient::sendServerPacketArea(AOPacket *packet){
    server->broadcast(packet, areaId());
}

void AOClient::sendServerMessageArea(const QString Message, const QString TMessage, const ClientVersion::ClientType Ttype, const QString CName){
    server->broadcast(PacketCT::CreateMessageS(Message, CName), PacketCT::CreateMessageS(TMessage, CName), Ttype, areaId());
}

bool AOClient::checkPermission(ACLRole::Permission f_permission) const{
    const QPointer<AreaData> l_area = server->getAreaById(areaId());
    const ACLRole l_role = server->getACLRolesHandler()->getRoleById(m_acl_role_id);

    switch (f_permission){
    case ACLRole::NONE:
        return true;
    case ACLRole::CM: // hack moment..
        return isAuthenticated() ? ConfigManager::authType() == DataTypes::AuthType::SIMPLE || m_authenticated_type == AuthenticateType::ROOT || l_role.checkPermission(ACLRole::SUPER) || l_role.checkPermission(f_permission) : (!l_area.isNull() && l_area->owners().contains(clientId()));
    case ACLRole::SUPER:
         return isAuthenticated() ? ConfigManager::authType() == DataTypes::AuthType::SIMPLE || m_authenticated_type == AuthenticateType::ROOT || l_role.checkPermission(ACLRole::SUPER) : false;
    default:
        return isAuthenticated() ? ConfigManager::authType() == DataTypes::AuthType::SIMPLE || m_authenticated_type == AuthenticateType::ROOT || l_role.checkPermission(ACLRole::SUPER) || l_role.checkPermission(f_permission) : false;
    }
}

QString AOClient::getIpid() const
{
    return m_ipid;
}

QString AOClient::getHwid() const
{
    return m_hwid;
}

bool AOClient::hasJoined() const
{
    return m_joined;
}

bool AOClient::isAuthenticated() const
{
    return m_authenticated_type > AuthenticateType::NONE;
}
bool AOClient::isMAuthenticated() const{
    return m_authenticated_type >= AuthenticateType::MODERATOR;
}
bool AOClient::isVAuthenticated() const{
    return m_authenticated_type == AuthenticateType::VIP;
}

QPointer<Server> AOClient::getServer()
{
    return server;
}

int AOClient::clientId() const
{
    return m_id;
}

QString AOClient::name() const
{
    return m_ooc_name;
}

void AOClient::setName(const QString &f_name)
{
    if (f_name != m_ooc_name) {
        m_ooc_name = f_name;
        Q_EMIT UpdateState(0);
    }
}

int AOClient::areaId() const
{
    return m_current_area;
}

void AOClient::setAreaId(const int f_area_id)
{
    if (f_area_id != m_current_area) {
        m_current_area = f_area_id;
        Q_EMIT UpdateState(3);
    }
}

QString AOClient::character() const
{
    return m_current_char;
}

void AOClient::setCharacter(const QString &f_character){
    if (f_character != m_current_char) {
        m_current_char = f_character;
        Q_EMIT UpdateState(1);
    }
}

QString AOClient::characterName() const
{
    return m_showname;
}

void AOClient::setCharacterName(const QString &f_showname)
{
    if (f_showname != m_showname) {
        m_showname = f_showname;
        Q_EMIT UpdateState(2);
    }
}

bool AOClient::UserAFK() const{
    return m_is_afk;
}

void AOClient::ToggleAFK(const bool afk){
    if (m_is_afk != afk){
        m_is_afk = afk;
        Q_EMIT UpdateState(1);
    }
}

bool AOClient::isSpectator() const
{
    return m_char_id < 0;
}

void AOClient::onAfkTimeout(){
    if (!UserAFK()) {
        if (m_afk_announcement){
            auto current_area = server->getAreaById(areaId());
            for (const int client_id : current_area->joinedIDs()){
                auto l_client = server->getClientByID(client_id);
                if (l_client.isNull())
                    continue;

                if (l_client == this) /* "this" ... current client (aka user) lol */
                    sendServerMessage("You are now AFK (due to inactivity).");
                else if (l_client->m_afk_received)
                    l_client->sendServerMessage(QString("[%1] %2 are now AFK (due to inactivity).").arg(QString::number(clientId()), character().isEmpty() ? "[Spectator]" : character()));
            }
        }
        else
            sendServerMessage("You are now AFK (due to inactivity). (unannouncement)");
        ToggleAFK();
    }
}

void AOClient::globalReminder(){
    const QStringList Getcustom_reminder = ConfigManager::CustomReminder();
    sendServerMessage(Getcustom_reminder.isEmpty() ? "Don't forget to take breaks and hydrate <3" : Getcustom_reminder[genRand(0, Getcustom_reminder.size() -1)]);
}

bool AOClient::isAccessBlocked(const AOClient::BlockType type) const{
    return m_client_block.testFlag(type);
}
void AOClient::SetAccessBlock(const AOClient::BlockType type, const bool toggle){
    m_client_block.setFlag(type, toggle);
}

bool AOClient::isCursed(const AOClient::CurseType type) const{
    return m_curses.testFlag(type);
}
void AOClient::SetCursed(const AOClient::CurseType type, const bool toggle){
    switch (type){
    case CurseType::FULL:
        if (m_curses.testFlag(AOClient::CurseType::DISEMVOWEL))
            m_curses.setFlag(AOClient::CurseType::DISEMVOWEL, false);
        if (m_curses.testFlag(AOClient::CurseType::GIMP))
            m_curses.setFlag(AOClient::CurseType::GIMP, false);
        if (m_curses.testFlag(AOClient::CurseType::MEDIEVAL))
            m_curses.setFlag(AOClient::CurseType::MEDIEVAL, false);
        if (m_curses.testFlag(AOClient::CurseType::SHAKE))
            m_curses.setFlag(AOClient::CurseType::SHAKE, false);
        if (m_curses.testFlag(AOClient::CurseType::UWUIFY))
            m_curses.setFlag(AOClient::CurseType::UWUIFY, false);
        if (m_curses.testFlag(AOClient::CurseType::PIGIFY))
            m_curses.setFlag(AOClient::CurseType::PIGIFY, false);
        m_curses.setFlag(type, toggle);
        break;
    default:
        m_curses.setFlag(type, toggle);
        break;
    }
}

QTimer *AOClient::GetVCBlockTimer(){
    return m_vcblock_left;
}
QString AOClient::GetVCBlockReason() const{
    return m_vcblock_reason;
}

QString AOClient::MessageShaked(const QString Message){
    QStringList l_parts = Message.split(QRegularExpression(R"([^A-Za-z0-9]+)"));

    std::random_device rng;
    std::mt19937 urng(rng());
    std::shuffle(l_parts.begin(), l_parts.end(), urng);


    return l_parts.join(' ');
}
QString AOClient::MessageDisemvowel(const QString Message){
    return QString(Message).remove(QRegularExpression("[aeiouáàäâãéèëêíìïîóòöôõúùüûæœaeiouáàäâãéèëêíìïîóòöôõúùüûæœаеиоуыэюяаеиоуыэюяαειουωαειουωاويع]+", QRegularExpression::CaseInsensitiveOption));
}
QString AOClient::MessageToGimped(const QString Message){
    const auto GetCurrentGimps = ConfigManager::gimpList();
    return GetCurrentGimps.isEmpty() ? Message : GetCurrentGimps[AOClient::genRand(0, GetCurrentGimps.size() -1)];
}
QString AOClient::MessageToMediveal(const QString Message){
    auto GetMedievalParser = std::unique_ptr<MedievalParser>(new MedievalParser); // std smart pointer..
    return GetMedievalParser->degrootify(Message);
}
QString AOClient::MessageToUwU(const QString Message)
{
    // UwU-ify a message using simple, case-sensitive replacements.
    // Order matters: longer patterns before shorter ones to avoid
    // partial overlaps (e.g., "ove" before "r"/"l").
    static const QList<QPair<QString, QString>> uwuTable = {
        //  n+vowel → ny+vowel (covers common cases)
        {"na", "nya"}, {"Na", "Nya"}, {"NA", "NYA"},
        {"ne", "nye"}, {"Ne", "Nye"}, {"NE", "NYE"},
        {"ni", "nyi"}, {"Ni", "Nyi"}, {"NI", "NYI"},
        {"no", "nyo"}, {"No", "Nyo"}, {"NO", "NYO"},
        {"nu", "nyu"}, {"Nu", "Nyu"}, {"NU", "NYU"},

        // th → d (the classic uwu lisp)
        {"th", "d"}, {"Th", "D"}, {"TH", "D"},

        // ove → uv (wuv, wuv)
        {"ove", "uv"}, {"Ove", "Uv"},

        // r/l → w (must come after the above to avoid breaking "th", "ove", etc.)
        {"r", "w"}, {"R", "W"},
        {"l", "w"}, {"L", "W"}
    };

    QString result = Message;
    for (const auto &pair : uwuTable)
        result.replace(pair.first, pair.second);

    return result;
}
QString AOClient::MessageToPigify(const QString Message)
{
    // latin vowels only.. includin accented ones for french, spanish, german etc~
    static const QString vowels =
        "aeiouáàäâãéèëêíìïîóòöôõúùüûæœ"
        "AEIOUÁÀÄÂÃÉÈËÊÍÌÏÎÓÒÖÔÕÚÙÜÛÆŒ";

    const QStringList words = Message.split(' ');

    QStringList result;
    for (QString word : words) {
        if (word.isEmpty())
            continue;

        // peel off any trailing punctuation.. save it for later~
        QString trailing;
        while (!word.isEmpty() && !word.back().isLetterOrNumber()) {
            trailing.prepend(word.back());
            word.chop(1);
        }

        // if it was just punctuation.. put it back n move on~
        if (word.isEmpty()) {
            result << trailing;
            continue;
        }

        // remember if it was capitalised.. then go lowercase for the rules~
        const bool wasCapitalised = word[0].isUpper();
        if (wasCapitalised)
            word[0] = word[0].toLower();

        // the pigify rule: sneak "way" in after the opening consonants~
        if (vowels.contains(word[0])) {
            // starts with vowel.. "way" goes right at the front~
            word = "way" + word;
        }
        else {
            // find the first vowel.. but keep "qu" together as one sound~
            int firstVowel = 0;
            while (firstVowel < word.size() && !vowels.contains(word[firstVowel])) {
                // "qu" is a pair.. they stay together~
                if (word[firstVowel] == 'q'
                    && firstVowel + 1 < word.size()
                    && word[firstVowel + 1] == 'u') {
                    firstVowel += 2;
                }
                else {
                    ++firstVowel;
                }
            }

            if (firstVowel >= word.size()) {
                // no vowel at all.. like "hmm" or "tsk".. just add "way" at the end~
                word += "way";
            }
            else {
                // tuck "way" right after the consonant cluster~
                word = word.left(firstVowel) + "way" + word.mid(firstVowel);
            }
        }

        // put the capitalisation back.. then reattach the punctuation~
        if (wasCapitalised)
            word[0] = word[0].toUpper();

        word += trailing;
        result << word;
    }

    return result.join(' ');
}

QString AOClient::NameWId(const QPointer<AOClient> client){
    return client.isNull() ? "" : QString("[%1] %2").arg(QString::number(client->clientId()), client->isSpectator() ? "[Spectator]" : client->character());
}
QByteArray AOClient::calcutateHashid(const QPointer<AOClient> &client){
    if (client.isNull())
        return {};

    QCryptographicHash hash(QCryptographicHash::Algorithm::Sha256);
    hash.addData(client->getIpid().toUtf8());
    hash.addData(client->getHwid().toUtf8());
    return hash.result().toHex().right(12);
}

AOClient::AOClient(Server *p_server, NetworkSocket *socket, QObject *parent, int user_id, MusicManager *p_manager) :
    QObject(parent),
    m_remote_ip(socket->peerAddress()),
    m_password(""),
    m_joined(false),
    m_socket(socket),
    m_music_manager(p_manager),
    m_last_wtce_time(0),
    m_id(user_id),
    m_current_area(0),
    m_current_char(""),
    server(p_server),
    rate_limit_tick(0),
    packet_count(0)
{
    m_afk_timer = new QTimer;
    m_afk_timer->setSingleShot(true);
    connect(m_afk_timer, &QTimer::timeout, this, &AOClient::onAfkTimeout);
    m_global_reminder_timer = new QTimer;
    connect(m_global_reminder_timer, &QTimer::timeout, this, &AOClient::globalReminder);
    m_global_reminder_timer->start(7200000);
    m_vcblock_left = new QTimer;
    m_vcblock_left->setSingleShot(true);
    connect(m_vcblock_left, &QTimer::timeout, this, [=]{
        if (isAccessBlocked(BlockType::VOICE)){
            SetAccessBlock(BlockType::VOICE, false);
            sendServerMessage("You are not longer voice-chat blocked/muted, you can use voice-chat.");
        }
        m_vcblock_reason.clear();
    });

    for (auto i = COMMANDS.constBegin(); i != COMMANDS.constEnd(); ++i)
        category_command.insert(i.value().category, {i.key(), i.value()});
}

AOClient::~AOClient()
{
    clientDisconnected();
    m_socket->deleteLater();
}
