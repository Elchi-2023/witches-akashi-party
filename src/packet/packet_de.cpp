#include "packet/packet_de.h"
#include "akashiutils.h"
#include "server.h"

#include <QDebug>

PacketDE::PacketDE(QStringList &contents) :
    AOPacket(contents)
{
}

PacketInfo PacketDE::getPacketInfo() const
{
    PacketInfo info{
        .acl_permission = ACLRole::Permission::NONE,
        .min_args = 1,
        .header = "DE"};
    return info;
}

void PacketDE::handlePacket(AreaData *area, AOClient &client) const
{
    if (client.checkEvidenceAccess(area)){
        bool isIndex;
        const int Index = m_content[0].toInt(&isIndex);

        if (isIndex && Index >= 0 && Index < area->evidence().size())
            area->deleteEvidence(Index);
        client.sendEvidenceList(area);
    }
}
