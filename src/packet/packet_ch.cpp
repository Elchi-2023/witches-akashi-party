#include "packet/packet_ch.h"
#include "akashiutils.h"
#include "server.h"

#include <QDebug>

PacketCH::PacketCH(QStringList &contents) :
    AOPacket(contents)
{
}

PacketInfo PacketCH::getPacketInfo() const{
    return PacketInfo::CreateInfo("CH");
}

void PacketCH::handlePacket(AreaData *area, AOClient &client) const
{
    Q_UNUSED(area)
    /* === [Dev notes] ===
     * Why does this packet exist
     * At least Crystal made it useful
     * It is now used for ping measurement
     * =================== */

    client.sendPacket("CHECK");
}
