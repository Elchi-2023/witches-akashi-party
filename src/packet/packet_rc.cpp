#include "packet/packet_rc.h"
#include "server.h"

#include <QDebug>

PacketRC::PacketRC(QStringList &contents) :
    AOPacket(contents)
{
}

PacketInfo PacketRC::getPacketInfo() const{
    return PacketInfo::CreateInfo("RC");
}

void PacketRC::handlePacket(AreaData *area, AOClient &client) const
{
    Q_UNUSED(area)

    client.sendPacket("SC", client.getServer()->getCharacters());
}
