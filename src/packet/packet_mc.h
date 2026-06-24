#ifndef PACKET_MC_H
#define PACKET_MC_H

#include "network/aopacket.h"

class PacketMC : public AOPacket
{
  public:
    PacketMC(QStringList &contents);
    virtual PacketInfo getPacketInfo() const;
    virtual void handlePacket(AreaData *area, AOClient &client) const;

    /**
     * @brief Creates an MC packet for packet music.
     *
     * @param Song The song file path or name.
     * @param c_id Character ID
     * @param showname Display name of the client showname.
     * @param isLoop Whether the song loops.
     * @param channels Number of audio channels.
     * @param effect Effect applied to the music.
     *
     * @return Pointer to the created AOPacket.
     */
    static AOPacket *CreateMusic(const QString &Song, const int c_id, const QString &showname = "", const bool isLoop = false, const int channels = 0, const int effect = 0);
};
#endif
