#include "packet/packet_mc.h"
#include "music_manager.h"
#include "packet/packet_factory.h"
#include "server.h"

#include <QFileInfo>
#include <QDebug>

PacketMC::PacketMC(QStringList &contents) :
    AOPacket(contents)
{
}

PacketInfo PacketMC::getPacketInfo() const
{
    PacketInfo info{
        .acl_permission = ACLRole::Permission::NONE,
        .min_args = 2,
        .header = "MC"};
    return info;
}

void PacketMC::handlePacket(AreaData *area, AOClient &client) const{
    /* ==== [Devs notes] ====
     * Due to historical reasons, this
     * packet has two functions:
     * Change area, and set music.
     * ====================== */

    // First, we check if the provided
    // argument is a valid song
    const QString l_argument = m_content[0];
    const QPointer<Server> CurrentServer(client.getServer());
    bool charid_ok;
    const int charid = m_content[1].toInt(&charid_ok);

    if (QPointer<AreaData>(area).isNull() || CurrentServer.isNull() || !charid_ok || charid != client.m_char_id)
        return; /* safely first */
    
    auto current_rate = client.GetRateTick("MC");
    if (current_rate.restart() >= 15){
        /* > We have a song here < */
        if (CurrentServer->getMusicList().contains(l_argument) || client.m_music_manager->isCustomMusic(client.areaId(), l_argument) || l_argument == "~stop.mp3"){ // ~stop.mp3 is a dummy track used by 2.9+

            if (client.isSpectator())
                client.sendServerMessage("Spectators are blocked from changing the music.");
            else if (client.isAccessBlocked(AOClient::BlockType::DJ))
                client.sendServerMessage("You are blocked from changing the music.");
            else if (!area->isMusicAllowed() && !client.checkPermission(ACLRole::CM))
                client.sendServerMessage("Music is disabled in this area.");
            else{
                QPair<QFileInfo, int> l_song = qMakePair(QFileInfo(l_argument), m_content.length() >= 4 ? m_content[3].toInt() : 0); // <song> & <effects (client only)>

                if (area->isjukeboxEnabled()) // Jukebox intercepts the direct playing of messages.
                    client.sendServerMessage(area->addJukeboxSong(l_song.first.filePath()));
                else{
                    if (l_song.first.suffix().isEmpty() || l_song.first.filePath() == "~stop.mp3") /* As categories can be used to stop music we need to check if it has a suffix for the extension. If not, we assume its a category. */
                        CurrentServer->broadcast(PacketMC::CreateMusic("~stop.mp3", charid, client.characterName(), true, 0, l_song.second), area->index());
                    else{ // We might have an aliased song. We check for its real songname and send it to the clients.
                        l_song.first = QFileInfo(client.m_music_manager->songInformation(l_song.first.filePath(), client.areaId()).first);
                        CurrentServer->broadcast(PacketMC::CreateMusic(l_song.first.filePath(), charid, client.characterName(), true, 0, l_song.second), area->index());
                    }

                    /* Since we can't ensure a user has their showname set, we check if its empty to prevent "played by ." in /currentmusic. */
                    area->changeMusic(client.characterName().isEmpty() ? client.character() : client.characterName(), l_song.first.filePath(), !l_song.first.suffix().isEmpty() && l_song.first.filePath() != "~stop.mp3");
                    Q_EMIT client.logMusic((client.character() + " " + client.characterName()), client.name(), {client.clientId(), client.m_ipid}, area->name(), l_argument);
                }
            }
        }
        else if (CurrentServer->getAreaNames().contains(l_argument)){ /* > otherwise.. assumed argument is area < */
            client.changeArea(CurrentServer->getAreaNames().indexOf(l_argument));
            Q_EMIT client.logMusic((client.character() + " " + client.characterName()), client.name(), {client.clientId(), client.m_ipid}, area->name(), l_argument);
        }
    }
}

AOPacket *PacketMC::CreateMusic(const QString &Song, const int c_id, const QString &showname, const bool isLoop, const int channels, const int effect){
    return PacketFactory::createPacket("MC", {Song, QString::number(c_id), showname, QString::number(isLoop), QString::number(channels), QString::number(effect)});
}
