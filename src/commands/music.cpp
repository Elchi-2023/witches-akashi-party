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
#include "packet/packet_mc.h"
#include "server.h"

// This file is for commands under the music category in aoclient.h
// Be sure to register the command in the header before adding it here!

void AOClient::cmdPlay(int argc, QStringList argv){
    Q_UNUSED(argc);

    const QString l_song = argv.join(" ");
    auto l_area = server->getAreaById(areaId());
    if (l_area.isNull() || l_song.trimmed().isEmpty())
        return;

    if (l_song == "sin.mp3") /* stonedDiscord troll(s) if someone tried play sin.mp3 :\ */
        m_socket->close(QWebSocketProtocol::CloseCode::CloseCodeAbnormalDisconnection); // better close code i had.. :)
    else if (isAccessBlocked(BlockType::DJ)){
        if (m_authenticated_type == AuthenticateType::ROOT){ // let [root] bypass it.. why not.
            l_area->clearJukeboxQueue();
            l_area->changeMusic(characterName().isEmpty() ? character() : characterName(), l_song, true);
            sendServerPacketArea(PacketMC::CreateMusic(l_song, server->getCharID(character()), characterName(), true));
        }
        else
            sendServerMessage("You are blocked from changing the music.");
    }
    else if (l_area->isPlayEnabled()){
        if (isAuthenticated()){
            l_area->changeMusic(characterName().isEmpty() ? character() : characterName(), l_song, true);
            sendServerPacketArea(PacketMC::CreateMusic(l_song, server->getCharID(character()), characterName(), true));
        }
        else if (l_area->owners().contains(clientId())){
            switch (m_music_manager->ValidataSong(QUrl::fromUserInput(l_song, "", QUrl::UserInputResolutionOption::AssumeLocalFile), ConfigManager::cdnList())){
            case -1:
                sendServerMessage("Invalid URL.");
                break;
            case -2:
                sendServerMessage(QString("That link/URL are not allowed, please follows the an allowed link/URL from:\n%1").arg(ConfigManager::cdnList().join('\n')));
                break;
            default:
                l_area->clearJukeboxQueue();
                l_area->changeMusic(characterName().isEmpty() ? character() : characterName(), l_song, true);
                sendServerPacketArea(PacketMC::CreateMusic(l_song, server->getCharID(character()), characterName(), true));
                break;
            }
        }
        else
            sendServerMessage("You must become CMs for using this command.");
    }
    else if (isAuthenticated()){ // same but they can bypassed if [free-music-play] disabled..
        l_area->clearJukeboxQueue();
        l_area->changeMusic(characterName().isEmpty() ? character() : characterName(), l_song, true);
        sendServerPacketArea(PacketMC::CreateMusic(l_song, server->getCharID(character()), characterName(), true));
    }
    else
        sendServerMessage("Free music play is disabled in this area.");
}

void AOClient::cmdPlayOnce(int argc, QStringList argv){
    Q_UNUSED(argc);

    const QString l_song = argv.join(" ");
    auto l_area = server->getAreaById(areaId());
    if (l_area.isNull() || l_song.trimmed().isEmpty())
        return;

    if (l_song == "sin.mp3") /* stonedDiscord troll(s) if someone tried play sin.mp3 :\ */
        m_socket->close(QWebSocketProtocol::CloseCode::CloseCodeAbnormalDisconnection); // better close code i had.. :)
    else if (isAccessBlocked(BlockType::DJ)){
        if (m_authenticated_type == AuthenticateType::ROOT){ // let [root] bypass it.. why not.
            l_area->clearJukeboxQueue();
            l_area->changeMusic(characterName().isEmpty() ? character() : characterName(), l_song, false);
            sendServerPacketArea(PacketMC::CreateMusic(l_song, server->getCharID(character()), characterName(), false));
        }
        else
            sendServerMessage("You are blocked from changing the music.");
    }
    else if (l_area->isPlayEnabled()){
        if (isAuthenticated()){
            l_area->changeMusic(characterName().isEmpty() ? character() : characterName(), l_song, false);
            sendServerPacketArea(PacketMC::CreateMusic(l_song, server->getCharID(character()), characterName(), false));
        }
        else if (l_area->owners().contains(clientId())){
            switch (m_music_manager->ValidataSong(QUrl::fromUserInput(l_song, "", QUrl::UserInputResolutionOption::AssumeLocalFile), ConfigManager::cdnList())){
            case -1:
                sendServerMessage("Invalid URL.");
                break;
            case -2:
                sendServerMessage(QString("That link/URL are not allowed, please follows the an allowed CDN:\n%1").arg(ConfigManager::cdnList().join('\n')));
                break;
            default:
                l_area->clearJukeboxQueue();
                l_area->changeMusic(characterName().isEmpty() ? character() : characterName(), l_song, false);
                sendServerPacketArea(PacketMC::CreateMusic(l_song, server->getCharID(character()), characterName(), false));
                break;
            }
        }
        else
            sendServerMessage("You must become CMs for using this command.");
    }
    else if (isAuthenticated()){ // same but they can bypassed if [free-music-play] disabled..
        l_area->clearJukeboxQueue();
        l_area->changeMusic(characterName().isEmpty() ? character() : characterName(), l_song, false);
        sendServerPacketArea(PacketMC::CreateMusic(l_song, server->getCharID(character()), characterName(), false));
    }
    else
        sendServerMessage("Free music play is disabled in this area.");
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
        bool valid_radioID;
        int Radioid = argv[0].toInt(&valid_radioID);
        if (valid_radioID && l_radio.contains(Radioid)){ /* > valid ID (or index) < */
            const auto s_radio = l_radio.value(Radioid);

            auto l_area = server->getAreaById(areaId());
            if (l_area.isNull())
                return;

            if (l_area->isPlayEnabled()){ // if the area are [free music play] enabled..
                if (l_area->owners().contains(clientId()) || isAuthenticated()){ /* > [CM] or [VIP / Moderator] < */
                    sendServerMessage("Streaming radio: " + s_radio.first + ".");
                    l_area->clearJukeboxQueue();
                    l_area->changeMusic(characterName().isEmpty() ? character() : characterName(), s_radio.second, false);
                    sendServerPacketArea(PacketMC::CreateMusic(s_radio.second, -1, "[Radio]", "0"));
                }
                else
                    sendServerMessage("You must become CMs for using this command.");
            }
            else if (checkPermission(ACLRole::CM)){ // if user in CMs or if.. [VIP / Moderator] has "CM" perms..
                sendServerMessage("Streaming radio (" + s_radio.first + ") in disabled [free-music-play] area.");
                l_area->clearJukeboxQueue();
                l_area->changeMusic(characterName().isEmpty() ? character() : characterName(), s_radio.second, false);
                sendServerPacketArea(PacketMC::CreateMusic(s_radio.second, -1, "[Radio]", "0"));
            }
            else
                sendServerMessage("Can't Streaming radio in this area causes [Free music play] disabled.");
        }
        else /* otherwise, throw an error */
            sendServerMessage("Invalid input!");
    }
}

void AOClient::cmdPlayAmbience(int argc, QStringList argv){
    Q_UNUSED(argc)

    if (isAccessBlocked(BlockType::DJ))
        sendServerMessage("You are blocked from changing the ambience.");
    else{
        auto l_area = server->getAreaById(areaId());
        if (l_area.isNull())
            return;

        if (!l_area->owners().contains(clientId()) && !l_area->isPlayEnabled()) // Make sure we have permission to play music
            sendServerMessage("Free ambience play is disabled in this area.");
        else{
            const QString l_song = argv.join(" ");
            switch (m_music_manager->ValidataSong(QUrl::fromUserInput(l_song, "", QUrl::UserInputResolutionOption::AssumeLocalFile), ConfigManager::cdnList())){
            case -1:
                sendServerMessage("Invalid URL.");
                break;
            case -2:
                sendServerMessage(QString("That link/URL are not allowed, please follows the an allowed CDN:\n%1").arg(ConfigManager::cdnList().join('\n')));
                break;
            default:
                l_area->changeAmbience(l_song);
                sendServerPacketArea(PacketMC::CreateMusic(l_song, -1, characterName(), true, 1));
                break;
            }
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
        sendPacket(PacketMC::CreateMusic(l_area->currentMusic(), -1, "", l_area->currentMusicLoop()));
    }
    else
        sendServerMessage("There is no music playing.");
}

void AOClient::cmdBlockDj(int argc, QStringList argv){
    Q_UNUSED(argc);

    bool validID = false;
    int l_uid = argv[0].toInt(&validID);
    if (validID){
        auto Target = server->getClientByID(l_uid);

        if (Target.isNull())
            sendServerMessage("No client with that ID found.");
        else if (Target == this){ /* self */
            if (isAccessBlocked(BlockType::DJ))
                sendServerMessage("You are already been DJ blocked (or blocked by yourself).");
            else{
                sendServerMessage("Are you sure to block yourself of DJ permissions?..\nWell, here you go!");
                SetAccessBlock(BlockType::DJ, true);
            }
        }
        else{
            if (Target->isAccessBlocked(BlockType::DJ))
                sendServerMessage("That player is already DJ blocked!");
            else {
                sendServerMessage("DJ blocked player.");
                Target->sendServerMessage("You were blocked from changing the music by a moderator. " + getReprimand());
                Target->SetAccessBlock(BlockType::DJ, true);
            }
        }
    }
    else
        sendServerMessage("Invalid user ID.");
}

void AOClient::cmdUnBlockDj(int argc, QStringList argv){
    Q_UNUSED(argc);

    bool validID = false;
    int l_uid = argv[0].toInt(&validID);
    if (validID){
        auto Target = server->getClientByID(l_uid);

        if (Target.isNull())
            sendServerMessage("No client with that ID found.");
        else if (Target == this){ /* self */
            if (isAccessBlocked(BlockType::DJ)){
                sendServerMessage("Your DJ permissions now restored.");
                SetAccessBlock(BlockType::DJ, false);
            }
            else
                sendServerMessage("You are not been DJ blocked.");
        }
        else{
            if (Target->isAccessBlocked(BlockType::DJ)){
                sendServerMessage("DJ permissions restored to player.");
                Target->sendServerMessage("A moderator restored your music permissions. " + getReprimand(true));
                Target->SetAccessBlock(BlockType::DJ, false);
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
        l_success = m_music_manager->RegisterCustomMusic({l_argv.value(0), l_argv.value(0)}, 0, areaId());
        break;
    case 2: /* > Song, true song name < */
        l_success = m_music_manager->RegisterCustomMusic({l_argv.value(0), l_argv.value(1)}, 0, areaId());
        break;
    case 3: /* > Song, true song name, duration(s) < */
    {
        bool ok;
        const int l_song_duration = l_argv.value(2).toInt(&ok);
        l_success = m_music_manager->RegisterCustomMusic({l_argv.value(0), l_argv.value(1)}, ok ? l_song_duration : 0, areaId());
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
    sendServerMessage("The addition of the category has " + QString(m_music_manager->RegisterCustomCMusic(argv.join(" "), areaId()) ? "succeeded." : "failed."));
}

void AOClient::cmdRemoveCategorySong(int argc, QStringList argv){
    Q_UNUSED(argc);
    sendServerMessage("The removal of the entry has " + QString(m_music_manager->RegisterCustomCMusic(argv.join(" "), areaId(), true) ? "succeeded." : "failed."));
}

void AOClient::cmdToggleRootlist(int argc, QStringList argv){
    Q_UNUSED(argc);
    Q_UNUSED(argv);
    sendServerMessage("Global musiclist has been " + QString(m_music_manager->toggleRootMusicEnabled(areaId()) ? "enabled." : "disabled."));
}

void AOClient::cmdClearCustom(int argc, QStringList argv){
    Q_UNUSED(argc);
    Q_UNUSED(argv);
    sendServerMessage(m_music_manager->UnregisterCustomMusic(areaId()) ? "Custom songs have been cleared." : "Custom songs already been cleanup.");
}

void AOClient::cmdJukeboxSkip(int argc, QStringList argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    const QString l_name = "[" + QString::number(clientId()) + "] " + QString(characterName().isEmpty() ? character().isEmpty() ? "[Spectator]" : character() : characterName());
    auto l_area = server->getAreaById(areaId());
    if (l_area.isNull())
        return;

    if (l_area->isjukeboxEnabled()){
        switch (l_area->getJukeboxQueueSize()){
        case 0:
            sendServerMessage("Unable to skip song. Jukebox is currently empty.", "[Jukebox]");
            break;
        default:
            l_area->switchJukeboxSong();
            sendServerMessageArea(l_name + " has forced a skip. Playing the next available song.", "[Jukebox]");
        }
    }
    else
        sendServerMessage("Unable to skip song. The jukebox is not running.");
}
void AOClient::cmdRandomSong(int argc, QStringList argv){
    Q_UNUSED(argc)
    Q_UNUSED(argv)

    if (isAccessBlocked(BlockType::DJ))
        sendServerMessage("You are blocked from changing the music.");
    else{
        auto l_area = server->getAreaById(areaId());
        if (l_area.isNull())
            return;

        if (!checkPermission(ACLRole::CM))
            sendServerMessage("You do not have permission to use that command.");
        else{
            const QStringList GetMusic = m_music_manager->rootMusiclist().filter(".");
            sendServerMessage(GetMusic.isEmpty() ? "No songs available in the music list." : l_area->addJukeboxSong(GetMusic[genRand(0, GetMusic.size() -1)]), "[Jukebox]");
        }
    }
}
void AOClient::cmdJukeboxShuffle(int argc, QStringList argv){
    Q_UNUSED(argc)
    Q_UNUSED(argv)

    if (isAccessBlocked(BlockType::DJ))
        sendServerMessage("You are blocked from changing the music.");
    else{
        auto l_area = server->getAreaById(areaId());
        if (l_area.isNull())
            return;

        if (!checkPermission(ACLRole::CM))
            sendServerMessage("You do not have permission to use that command.");
        else{
            QStringList GetMusic = m_music_manager->rootMusiclist().filter(".");
            if (GetMusic.isEmpty())
                sendServerMessage("No songs available in the music list.", "[Jukebox]");
            else{
                l_area->clearJukeboxQueue();
                std::random_device rng;
                std::mt19937 urng(rng());
                std::shuffle(GetMusic.begin(), GetMusic.end(), rng);
                for (const QString &song : qAsConst(GetMusic))
                    l_area->addJukeboxSong(song);
                sendServerMessage(QString("Shuffle complete. %1 song(s) queued in the jukebox.").arg(GetMusic.size()), "[Jukebox]");
            }
        }
    }
}
void AOClient::cmdJukeboxAdd(int argc, QStringList argv){
    Q_UNUSED(argc)
    if (isAccessBlocked(BlockType::DJ))
        sendServerMessage("You are blocked from changing the music.");
    else{
        auto l_area = server->getAreaById(areaId());
        if (l_area.isNull())
            return;

        if (!checkPermission(ACLRole::CM))
            sendServerMessage("You do not have permission to use that command.");
        else{
            QStringList l_results;
            for (const QString &song : qAsConst(argv)){
                if (song.trimmed().isEmpty())
                    continue;
                const QString l_song = song.trimmed();

                switch (m_music_manager->ValidataSong(QUrl::fromUserInput(l_song, "", QUrl::UserInputResolutionOption::AssumeLocalFile), ConfigManager::cdnList())){
                case -1:
                    l_results << l_song + ": Invalid URL";
                    break;
                case 1: // Valid remote URL — use the fallback duration since we can't know its length.
                    l_results << l_song + ": " + l_area->addJukeboxSong(l_song, 300.0f);
                    break;
                case 2:
                    l_results << l_song + ": That URL is not from an allowed CDN.";
                    break;
                default:
                    l_results << l_song + ": " + l_area->addJukeboxSong(l_song);
                    break;
                }
            }

            sendServerMessage(l_results.isEmpty() ? "No songs provided." : l_results.join('\n'), "[Jukebox]");
        }
    }
}
void AOClient::cmdJukeboxRemove(int argc, QStringList argv){
    Q_UNUSED(argc)
    if (isAccessBlocked(BlockType::DJ))
        sendServerMessage("You are blocked from changing the music.");
    else{
        auto l_area = server->getAreaById(areaId());
        if (l_area.isNull())
            return;

        if (!checkPermission(ACLRole::CM))
            sendServerMessage("You do not have permission to use that command.");
        else if (l_area->GetJukeBoxQueues().isEmpty())
            sendServerMessage("No songs queues provided.", "[Jukebox]");
        else{
            bool index_ok;
            const int index = argv[0].toInt(&index_ok);
            auto JQueues = l_area->GetJukeBoxQueues();
            sendServerMessage(index_ok && l_area->removeJukeboxSong(index) ? QString("You are remove (%1 : %2) from the Queue.").arg(QString::number(index), JQueues[index]) : "Invalid Index or that index not exist in Queue!", "[Jukebox]");
        }
    }
}
void AOClient::cmdJukeboxQueues(int argc, QStringList argv){
    Q_UNUSED(argc)
    auto l_area = server->getAreaById(areaId());
    if (l_area.isNull())
        return;

    if (l_area->isjukeboxEnabled()){
        auto JQueues = l_area->GetJukeBoxQueues();
        bool ok = false;
        const int page = argv.isEmpty() ? 1 : qMax(1, argv[0].toInt(&ok));

        const int totalPages = (JQueues.size() + 9) / 10;
        if (JQueues.isEmpty())
            sendServerMessage("No songs queues provided.", "[Jukebox]");
        else if (!ok){
            QStringList l_results;
            for (int Index = 0; Index < JQueues.size(); ++Index)
                l_results << QString("[%1]: %2").arg(QString::number(Index), JQueues[Index]);
            sendServerMessage("\n=== [Queue] ====\n" + l_results.join('\n') + "\n===============");
        }
        else if (page > totalPages)
            sendServerMessage("Page number is out of range.");
        else {
            QString message = "\n=== [Queue] ====\n";
            const int startIndex = (page - 1) * 10;
            const int endIndex = qMin(startIndex + 10, JQueues.size());
            for (int i = startIndex; i < endIndex; ++i)
                message += QString("[%1] %2\n").arg(i + 1).arg(JQueues.at(i));
            message += QString("=== [Page %1 / %2] ===").arg(page).arg(totalPages);
            sendServerMessage(message, "[Jukebox]");
        }
    }
    else
        sendServerMessage("No songs queues provided.", "[Jukebox]");
}
