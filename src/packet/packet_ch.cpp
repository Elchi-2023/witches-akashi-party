#include "packet/packet_ch.h"
#include "akashiutils.h"
#include "server.h"

#include <QDebug>

PacketCH::PacketCH(QStringList &contents) :
    AOPacket(contents)
{
}

PacketInfo PacketCH::getPacketInfo() const
{
    PacketInfo info{
        .acl_permission = ACLRole::Permission::NONE,
        .min_args = 0,
        .header = "CH"};
    return info;
}

void PacketCH::handlePacket(AreaData *area, AOClient &client) const
{
    Q_UNUSED(area)
    /* === [Dev notes] ===
     * Why does this packet exist
     * At least Crystal made it useful
     * It is now used for ping measurement
     * =================== */

    if (m_content.isEmpty() || m_content[0].toInt() == client.m_char_id) // vaild check would be like.. "if" context is empty or one param are exacty same like user char_id..
        client.sendPacket("CHECK");
    // otherwise.. not send "check" back to client, guess.. let it disconnect by client itself..
}
