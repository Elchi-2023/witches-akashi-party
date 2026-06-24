#ifndef PACKET_VOICE_H
#define PACKET_VOICE_H

#include "network/aopacket.h"
#include <QByteArray>
#include <QMutex>
#include <QMutexLocker>

/* === [Ganty's note] ===
 * i don't know if i can do better than this since..
 * my goal is just make akashi could do the same like nyathena (voice-chat) but eh..
 * so yeah.. pardon me if this 'odd/ugy' code (since human coding)..
 * ======================*/

namespace PacketVoice{

/* > <voice join> <*/
class Join : public AOPacket{
public:
    Join(QStringList &contents);
    virtual PacketInfo getPacketInfo() const;
    virtual void handlePacket(AreaData *area, AOClient &client) const;
};

/* > <voice leave> <*/
class Leave : public AOPacket{
public:
    Leave(QStringList &contents);
    virtual PacketInfo getPacketInfo() const;
    virtual void handlePacket(AreaData *area, AOClient &client) const;
};

/* > <voice audio frame> <*/
class AudioFrame : public AOPacket{
public:
    AudioFrame(QStringList &contents);
    virtual PacketInfo getPacketInfo() const;
    virtual void handlePacket(AreaData *area, AOClient &client) const;
};

/* > <voice "speak" state> <*/
class SpeakState : public AOPacket{
public:
    SpeakState(QStringList &contents);
    virtual PacketInfo getPacketInfo() const;
    virtual void handlePacket(AreaData *area, AOClient &client) const;
};

}

#endif // PACKET_VOICE_H
