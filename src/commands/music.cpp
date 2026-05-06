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
#include "music_manager.h"
#include "packet/packet_factory.h"
#include "server.h"

#include <algorithm>
#include <random>

// This file is for commands under the music category in aoclient.h
// Be sure to register the command in the header before adding it here!

void AOClient::cmdPlay(int argc, QStringList argv)
{
    Q_UNUSED(argc);
    QString l_song = argv.join(" ");
    if (m_is_dj_blocked) {
        sendServerMessage("You are blocked from changing the music.");
        return;
    }
    if (l_song == "sin.mp3") {
        m_socket->close();
        return;
    }
    auto l_area = server->getAreaById(areaId());
    if (l_area.isNull())
        return;

    const ACLRole l_role = server->getACLRolesHandler()->getRoleById(m_acl_role_id);
    if (m_vip_authenticated || m_authenticated){ /* bypassed for vip and mods, no matter if area not free play or has cms on it*/
        l_area->clearJukeboxQueue();
        l_area->changeMusic(characterName().isEmpty() ? character() : characterName(), l_song, true);
        AOPacket *music_change = PacketFactory::createPacket("MC", {l_song, QString::number(server->getCharID(character())), characterName(), "1", "0"});
        server->broadcast(music_change, areaId());
        return;
    }
    else if (!l_area->owners().contains(clientId()) && !l_area->isPlayEnabled() && !l_role.checkPermission(ACLRole::CM)) { // Make sure we have permission to play music
        sendServerMessage("Free music play is disabled in this area.");
        return;
    }
    else{
        switch (m_music_manager->ValidataSong(QUrl::fromUserInput(l_song, "", QUrl::UserInputResolutionOption::AssumeLocalFile), ConfigManager::cdnList())){
        case -1:
            sendServerMessage("Invaild URL.");
            break;
        case -2:
            sendServerMessage("That link/URL wasn't Allowed.");
            break;
        default:
            l_area->clearJukeboxQueue();
            l_area->changeMusic(characterName().isEmpty() ? character() : characterName(), l_song, true);
            server->broadcast(PacketFactory::createPacket("MC", {l_song, QString::number(server->getCharID(character())), characterName(), "1", "0"}), areaId());
            break;
        }
    }
}

void AOClient::cmdPlayOnce(int argc, QStringList argv){
    Q_UNUSED(argc)

    const QString Song = argv.join(" ");
    if (Song.trimmed().isEmpty())
        return;
    else if (m_is_dj_blocked)
        sendServerMessage("You are blocked from changing the music.");
    else if (Song == "sin.mp3")
        m_socket->close();
    else{
        auto l_area = server->getAreaById(areaId());
        if (l_area.isNull())
            return;

        const ACLRole current_role = server->getACLRolesHandler()->getRoleById(m_acl_role_id);
        if (m_vip_authenticated || m_authenticated){ /* bypassed for vip and mods, no matter if area not free play or has cms on it*/
            l_area->clearJukeboxQueue();
            l_area->changeMusic(characterName().isEmpty() ? character() : characterName(), Song, false);
            AOPacket *music_change = PacketFactory::createPacket("MC", {Song, QString::number(server->getCharID(character())), characterName(), "0", "0"});
            server->broadcast(music_change, areaId());
        }
        else if (!l_area->owners().contains(clientId()) && !l_area->isPlayEnabled() && !current_role.checkPermission(ACLRole::CM)) // Make sure we have permission to play music
            sendServerMessage("Free music play is disabled in this area.");
        else{
            switch (m_music_manager->ValidataSong(QUrl::fromUserInput(Song, "", QUrl::UserInputResolutionOption::AssumeLocalFile), ConfigManager::cdnList())){
            case -1:
                sendServerMessage("Invaild URL.");
                break;
            case -2:
                sendServerMessage("That link/URL wasn't Allowed.");
                break;
            default:
                l_area->clearJukeboxQueue();
                l_area->changeMusic(characterName().isEmpty() ? character() : characterName(), Song, false);
                server->broadcast(PacketFactory::createPacket("MC", {Song, QString::number(server->getCharID(character())), characterName(), "0", "0"}), areaId());
                break;
            }
        }
    }
}

void AOClient::cmdRadio(int argc, QStringList argv)
{
    Q_UNUSED(argc);

    const auto& l_radio = ConfigManager::radiolist();

    if (argv.isEmpty()){ // if there are no arguments, send the radio list to ooc
        QStringList l_radio_list;
        for (auto i = l_radio.constBegin(); i != l_radio.constEnd(); i++)
            l_radio_list.append(QString("[%1]: %2").arg(i.key()).arg(i.value().first));

        sendServerMessage(l_radio_list.isEmpty() ? "The radio aren't available." : "\n=== [Radio] ===\n" + l_radio_list.join('\n') + "\n=========");
    }
    else{
        bool Vaild_radioID;
        int Radioid = argv[0].toInt(&Vaild_radioID);
        if (Vaild_radioID && l_radio.contains(Radioid)){ /* > Vaild ID (or index) < */
            const auto [Radio_name, Selected_Radio] = l_radio.value(Radioid);

            auto l_area = server->getAreaById(areaId());
            if (l_area.isNull())
                return;
            const ACLRole current_role = server->getACLRolesHandler()->getRoleById(m_acl_role_id);
            if (m_vip_authenticated || m_authenticated){
                sendServerMessage("Streaming radio: " + Radio_name + ".");
                l_area->clearJukeboxQueue();
                l_area->changeMusic(characterName().isEmpty() ? character() : characterName(), Selected_Radio, false);
                server->broadcast(PacketFactory::createPacket("MC", {Selected_Radio, QString::number(server->getCharID(character())), characterName(), "0", "0"}), areaId());
            }
            else if (!l_area->owners().contains(clientId()) && !l_area->isPlayEnabled() && !current_role.checkPermission(ACLRole::CM)) // Make sure we have permission to play music
                sendServerMessage("Can't Streaming radio cause this area aren't [Free music play] enabled.");
            else{
                sendServerMessage("Streaming radio: " + Radio_name + ".");
                l_area->clearJukeboxQueue();
                l_area->changeMusic(characterName().isEmpty() ? character() : characterName(), Selected_Radio, false);
                server->broadcast(PacketFactory::createPacket("MC", {Selected_Radio, QString::number(server->getCharID(character())), characterName(), "0", "0"}), areaId());
            }
        }
        else /* otherwise, throw an error */
            sendServerMessage("Invaild input!");
    }
}

void AOClient::cmdPlayAmbience(int argc, QStringList argv){
    Q_UNUSED(argc)

    if (m_is_dj_blocked)
        sendServerMessage("You are blocked from changing the ambience.");
    else{
        auto l_area = server->getAreaById(areaId());
        if (l_area.isNull())
            return;

        if (!l_area->owners().contains(clientId()) && !l_area->isPlayEnabled()) // Make sure we have permission to play music
            sendServerMessage("Free ambience play is disabled in this area.");
        else{
            const QString l_song = argv.join(" ");
            l_area->changeAmbience(l_song);
            sendServerPacketArea(PacketFactory::createPacket("MC", {l_song, "-1", characterName(), "1", "1"}));
        }
    }
}

void AOClient::cmdCurrentMusic(int argc, QStringList argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    auto l_area = server->getAreaById(areaId());
    if (l_area.isNull())
        return;

    if (!l_area->currentMusic().isEmpty() && !l_area->currentMusic().contains("~stop.mp3")) // dummy track for stopping music
        sendServerMessage("The current song is " + l_area->currentMusic() + " played by " + l_area->musicPlayerBy());
    else
        sendServerMessage("There is no music playing.");
}

void AOClient::cmdGetMusic(int argc, QStringList argv){
    Q_UNUSED(argc)
    Q_UNUSED(argv)

    const auto l_area = server->getAreaById(areaId());
    if (l_area.isNull())
        return;

    if (!l_area->currentMusic().isEmpty() || !l_area->currentMusic().contains("~stop.mp3")){ // dummy track for stopping music
        sendServerMessage("Playing the current song is " + l_area->currentMusic() + " played by " + l_area->musicPlayerBy());
        sendPacket("MC", {l_area->currentMusic(), "-1", characterName(), QString::number(l_area->currentMusicLoop()), "0"});
    }
    else
        sendServerMessage("There is no music playing.");
}

void AOClient::cmdBlockDj(int argc, QStringList argv){
    Q_UNUSED(argc);

    bool VaildID = false;
    int l_uid = argv[0].toInt(&VaildID);
    if (VaildID){
        auto Target = server->getClientByID(l_uid);

        if (Target.isNull())
            sendServerMessage("No client with that ID found.");
        else if (Target == this){ /* self */
            if (m_is_dj_blocked)
                sendServerMessage("You are already been DJ blocked (or blocked by yourself).");
            else{
                sendServerMessage("Are you sure to block yourself of DJ permissions?..\nWell, here you go!");
                m_is_dj_blocked = true;
            }
        }
        else{
            if (Target->m_is_dj_blocked)
                sendServerMessage("That player is already DJ blocked!");
            else {
                sendServerMessage("DJ blocked player.");
                Target->sendServerMessage("You were blocked from changing the music by a moderator. " + getReprimand());
            }
        }
    }
    else
        sendServerMessage("Invalid user ID.");
}

void AOClient::cmdUnBlockDj(int argc, QStringList argv){
    Q_UNUSED(argc);

    bool VaildID = false;
    int l_uid = argv[0].toInt(&VaildID);
    if (VaildID){
        auto Target = server->getClientByID(l_uid);

        if (Target.isNull())
            sendServerMessage("No client with that ID found.");
        else if (Target == this){ /* self */
            if (m_is_dj_blocked){
                sendServerMessage("Your DJ permissions now restored.");
                m_is_dj_blocked = false;
            }
            else
                sendServerMessage("You are not been DJ blocked.");
        }
        else{
            if (Target->m_is_dj_blocked){
                sendServerMessage("DJ permissions restored to player.");
                Target->sendServerMessage("A moderator restored your music permissions. " + getReprimand(true));
                Target->m_is_dj_blocked = false;
            }
            else
                sendServerMessage("That player is not DJ blocked!");
        }
    }
    else
        sendServerMessage("Invalid user ID.");
}

void AOClient::cmdToggleMusic(int argc, QStringList argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    auto l_area = server->getAreaById(areaId());
    if (l_area.isNull())
        return;

    l_area->toggleMusic();
    sendServerMessage("Music in this area is now " + QString(l_area->isMusicAllowed() ? "allowed." : "disallowed."));
}

void AOClient::cmdToggleJukebox(int argc, QStringList argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    auto l_area = server->getAreaById(areaId());
    if (l_area.isNull())
        return;

    l_area->toggleJukebox();
    sendServerMessageArea("The jukebox in this area has been " + QString(l_area->isjukeboxEnabled() ? "enabled." : "disabled."));
}

void AOClient::cmdAddSong(int argc, QStringList argv){
    Q_UNUSED(argc);

    // This needs some explanation.
    // Akashi has no concept of argument count,so any space is interpreted as a new element
    // in the QStringList. This works fine until someone enters something with a space.
    // Since we can't preencode those elements, we join all as a string and use a delimiter
    // that does not exist in file and URL paths. I decided on the ol' reliable ','.
    const QStringList l_argv = argv.join(" ").split(",");

    bool l_success = false;
    switch (l_argv.size()){
    case 1: /* > Song < */
        l_success = m_music_manager->addCustomSong(l_argv.value(0), l_argv.value(0), 0, areaId());
        break;
    case 2: /* > Song, true song name < */
        l_success = m_music_manager->addCustomSong(l_argv.value(0), l_argv.value(1), 0, areaId());
        break;
    case 3: /* > Song, true song name, duration(s) < */
        {
            bool ok;
            const int l_song_duration = l_argv.value(2).toInt(&ok);
            l_success = m_music_manager->addCustomSong(l_argv.value(0), l_argv.value(1), ok ? l_song_duration : 0, areaId());
        }
        break;
    case 4: default:
        sendServerMessage("Too many arguments. Addition of song has failed.");
        return;
    }

    sendServerMessage("The addition of the song has " + QString(l_success ? "succeeded." : "failed."));
}

void AOClient::cmdAddCategory(int argc, QStringList argv){
    Q_UNUSED(argc);
    bool l_success = m_music_manager->addCustomCategory(argv.join(" "), areaId());
    QString l_message = l_success ? "succeeded." : "failed.";
    sendServerMessage("The addition of the category has " + l_message);
}

void AOClient::cmdRemoveCategorySong(int argc, QStringList argv){
    Q_UNUSED(argc);
    bool l_success = m_music_manager->removeCategorySong(argv.join(" "), areaId());
    QString l_message = l_success ? "succeeded." : "failed.";
    sendServerMessage("The removal of the entry has " + QString(m_music_manager->removeCategorySong(argv.join(" "), areaId()) ? "succeeded." : "failed."));
}

void AOClient::cmdToggleRootlist(int argc, QStringList argv){
    Q_UNUSED(argc);
    Q_UNUSED(argv);
    sendServerMessage("Global musiclist has been " + QString(m_music_manager->toggleRootEnabled(areaId()) ? "enabled." : "disabled."));
}

void AOClient::cmdClearCustom(int argc, QStringList argv){
    Q_UNUSED(argc);
    Q_UNUSED(argv);
    m_music_manager->clearCustomList(areaId());
    sendServerMessage("Custom songs have been cleared.");
}

void AOClient::cmdJukeboxSkip(int argc, QStringList argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    const QString l_name = "[" + QString::number(clientId()) + "] " + QString(characterName().isEmpty() ? character().isEmpty() ? "Spectator" : character() : characterName());
    auto l_area = server->getAreaById(areaId());
    if (l_area.isNull())
        return;

    if (l_area->isjukeboxEnabled()){
        switch (l_area->getJukeboxQueueSize()){
        case 0:
            sendServerMessage("Unable to skip song. Jukebox is currently empty.");
            break;
        default:
            l_area->switchJukeboxSong();
            sendServerMessageArea(l_name + " has forced a skip. Playing the next available song.");
        }
    }
    else
        sendServerMessage("Unable to skip song. The jukebox is not running.");
}

// Returns only the actual playable songs from the root music list,
// filtering out category headers (which have no file extension).
static QStringList playableSongs(MusicManager *musicManager)
{
    QStringList l_songs;
    const QStringList l_all = musicManager->rootMusiclist();
    for (const QString &entry : qAsConst(l_all)) {
        if (entry.contains('.'))
            l_songs.append(entry);
    }
    return l_songs;
}

void AOClient::cmdRandomSong(int argc, QStringList argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    if (m_is_dj_blocked) {
        sendServerMessage("You are blocked from changing the music.");
        return;
    }

    auto l_area = server->getAreaById(areaId());
    if (l_area.isNull())
        return;

    if (!hasJukeboxCommandPermission()) {
        sendServerMessage("You do not have permission to use that command.");
        return;
    }

    const QStringList l_songs = playableSongs(m_music_manager);
    if (l_songs.isEmpty()) {
        sendServerMessage("No songs available in the music list.");
        return;
    }

    const int l_index = static_cast<int>(QRandomGenerator::system()->bounded(static_cast<quint32>(l_songs.size())));
    sendServerMessage(l_area->addJukeboxSong(l_songs.at(l_index)));
}

void AOClient::cmdShuffle(int argc, QStringList argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    if (m_is_dj_blocked) {
        sendServerMessage("You are blocked from changing the music.");
        return;
    }

    auto l_area = server->getAreaById(areaId());
    if (l_area.isNull())
        return;

    if (!hasJukeboxCommandPermission()) {
        sendServerMessage("You do not have permission to use that command.");
        return;
    }

    QStringList l_songs = playableSongs(m_music_manager);
    if (l_songs.isEmpty()) {
        sendServerMessage("No songs available to shuffle.");
        return;
    }

    // Clear the existing queue so shuffle replaces it rather than appending.
    l_area->clearJukeboxQueue();

    // Shuffle using a seeded Mersenne Twister for good randomness
    std::mt19937 rng(QRandomGenerator::system()->generate());
    std::shuffle(l_songs.begin(), l_songs.end(), rng);

    for (const QString &song : qAsConst(l_songs))
        l_area->addJukeboxSong(song);

    sendServerMessage(QString("Shuffle complete. %1 song(s) queued in the jukebox.").arg(l_songs.size()));
}

void AOClient::cmdPlaylistAdd(int argc, QStringList argv)
{
    Q_UNUSED(argc);

    if (m_is_dj_blocked) {
        sendServerMessage("You are blocked from changing the music.");
        return;
    }

    auto l_area = server->getAreaById(areaId());
    if (l_area.isNull())
        return;

    if (!hasJukeboxCommandPermission()) {
        sendServerMessage("You do not have permission to use that command.");
        return;
    }

    // Default fallback duration for custom URLs (5 minutes in seconds).
    static constexpr float URL_FALLBACK_DURATION = 300.0f;

    QStringList l_results;
    for (const QString &song : qAsConst(argv)) {
        const QString l_song = song.trimmed();
        if (l_song.isEmpty())
            continue;

        // Determine whether this entry is a remote URL or a local song name.
        const int l_validation = m_music_manager->ValidataSong(
            QUrl::fromUserInput(l_song, "", QUrl::UserInputResolutionOption::AssumeLocalFile),
            ConfigManager::cdnList());

        if (l_validation == -1) {
            l_results.append(l_song + ": Invalid URL.");
        }
        else if (l_validation == -2) {
            l_results.append(l_song + ": That URL is not from an allowed CDN.");
        }
        else if (l_validation == 1) {
            // Valid remote URL — use the fallback duration since we can't know its length.
            l_results.append(l_song + ": " + l_area->addJukeboxSong(l_song, URL_FALLBACK_DURATION));
        }
        else {
            // Local song name — look it up in the music list normally.
            l_results.append(l_song + ": " + l_area->addJukeboxSong(l_song));
        }
    }

    if (l_results.isEmpty())
        sendServerMessage("No songs provided.");
    else
        sendServerMessage(l_results.join('\n'));
}
