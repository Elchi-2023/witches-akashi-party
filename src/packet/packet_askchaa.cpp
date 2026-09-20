#include "packet/packet_askchaa.h"
#include "config_manager.h"
#include "server.h"

#include <QDebug>

PacketAskchaa::PacketAskchaa(QStringList &contents) :
    AOPacket(contents)
{
}

PacketInfo PacketAskchaa::getPacketInfo() const{
    return PacketInfo::CreateInfo("askchaa");
}

void PacketAskchaa::handlePacket(AreaData *area, AOClient &client) const{
    Q_UNUSED(area)
    /* === [akashi devs notes] ===
     * Evidence isn't loaded during this part anymore
     * As a result, we can always send "0" for evidence length
     * Client only cares about what it gets from LE
     * =========================== */
    client.sendPacket("SI", {QString::number(client.getServer()->getCharacterCount()), "0", QString::number(client.getServer()->getAreaCount() + client.getServer()->getMusicList().length())});
}
