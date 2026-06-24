#include "packet/packet_cc.h"
#include "akashiutils.h"
#include "config_manager.h"
#include "server.h"

#include <QDebug>

PacketCC::PacketCC(QStringList &contents) :
    AOPacket(contents)
{
}

PacketInfo PacketCC::getPacketInfo() const
{
    PacketInfo info{
        .acl_permission = ACLRole::Permission::NONE,
        .min_args = 2,
        .header = "CC"};
    return info;
}

void PacketCC::handlePacket(AreaData *area, AOClient &client) const{
    Q_UNUSED(area)

    if (client.hasJoined()){ // character selecting when you are joined.
        if (client.getServer().isNull() || QPointer<AreaData>(area).isNull())
            return; // safey first..

        // we needs validate the client CC (<char_id>) packet..
        bool charId_ok;
        int charId = m_content[1].toInt(&charId_ok);
        if (!charId_ok || charId < -1 || charId > client.getServer()->getCharacters().size() -1) // always be set spectator for invalid ranges..
            charId = client.SPECTATOR_ID;

        if (client.changeCharacter(charId) && area->owners().contains(client.clientId()))
            client.arup(AOClient::ARUPType::CM, true);
    }
}
