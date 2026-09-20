#include "packet/packet_rm.h"
#include "server.h"

#include <QDebug>

PacketRM::PacketRM(QStringList &contents) :
    AOPacket(contents)
{
}

PacketInfo PacketRM::getPacketInfo() const{
    return PacketInfo::CreateInfo("RM");
}

void PacketRM::handlePacket(AreaData *area, AOClient &client) const
{
    Q_UNUSED(area)

    client.sendPacket("SM", client.getServer()->getAreaNames() + client.getServer()->getMusicList());
}
