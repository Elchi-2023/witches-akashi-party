#ifndef PACKET_TT_H
#define PACKET_TT_H

#include "network/aopacket.h"

class PacketTT : public AOPacket{
public:
    PacketTT(QStringList &contents);
    virtual PacketInfo getPacketInfo() const;
    virtual void handlePacket(AreaData *area, AOClient &client) const;
    virtual bool validatePacket() const;
private:
  AOPacket *validateTTPacket(AOClient &client) const;
};

#endif // PACKET_TT_H
