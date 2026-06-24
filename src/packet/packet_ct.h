#ifndef PACKET_CT_H
#define PACKET_CT_H

#include "network/aopacket.h"

class PacketCT : public AOPacket
{
  public:
    PacketCT(QStringList &contents);
    virtual PacketInfo getPacketInfo() const;
    virtual void handlePacket(AreaData *area, AOClient &client) const;

    /**
     * @brief Create PacketFactory::createPacket of "CT".
     * @param The message.
     * @param The custom of name if persent.
     * @return PacketFactory::createPacket of "CT" has created,
     */
    static AOPacket *CreateMessage(const QString &Message, const QString &cname = QString());
    /**
     * @brief Create PacketFactory::createPacket of "CT" (as server).
     * @param The message.
     * @param The custom of name if persent.
     * @return PacketFactory::createPacket of "CT" has created,
     */
    static AOPacket *CreateMessageS(const QString &Message, const QString &cname = QString());
};
#endif
