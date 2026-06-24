#include "packet/packet_pe.h"
#include "server.h"

#include <QDebug>
#include <QRegularExpression>

PacketPE::PacketPE(QStringList &contents) :
    AOPacket(contents)
{
}

PacketInfo PacketPE::getPacketInfo() const
{
    PacketInfo info{
        .acl_permission = ACLRole::Permission::NONE,
        .min_args = 3,
        .header = "PE"};
    return info;
}

void PacketPE::handlePacket(AreaData *area, AOClient &client) const
{
    if (client.checkEvidenceAccess(area)){
        bool isIndex;
        const int EviIndex = m_content[0].toInt(&isIndex);

        if (isIndex && EviIndex >= 0 && EviIndex < area->evidence().size()){ /* capture valid index */
            AreaData::Evidence evidence{
                .name = m_content[1],
                .description = m_content[2],
                .image = m_content[3],
            };
            static const QRegularExpression ownerRegex("<owner=(.*?)>");

            /* Automatically add <owner=all> for evidence in HIDDEN_CM mode areas
             * Check if owner tag already exists in description */
            if (area->eviMod() == AreaData::EvidenceMod::HIDDEN_CM && !ownerRegex.match(evidence.description).hasMatch())
                evidence.description = "<owner=all>\n" + evidence.description;
            area->appendEvidence(evidence);
            client.sendEvidenceList(area);
        }
    }
}
