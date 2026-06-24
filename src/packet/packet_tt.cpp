#include "packet/packet_tt.h"
#include "config_manager.h"
#include "packet/packet_factory.h"
#include "server.h"


PacketTT::PacketTT(QStringList &contents) : AOPacket(contents){

}

PacketInfo PacketTT::getPacketInfo() const{
    PacketInfo info{
        .acl_permission = ACLRole::Permission::NONE,
        .min_args = 3,
        .header = "TT"};
    return info;
}

void PacketTT::handlePacket(AreaData *area, AOClient &client) const{
    Q_UNUSED(area);
    if (!client.m_joined || client.isSpectator()) /* Don't let outsider be mess this or don't let spectator mess this either */
        return;

    bool isIntVaild;
    Q_UNUSED(m_content[0].toInt(&isIntVaild));

    if (!isIntVaild || m_content[1].trimmed().isEmpty() || m_content[2].trimmed().isEmpty()) /* arg checker moment */
        return;

    /* > kfo behaviors < */
    if (m_content[1].compare(client.character()) != 0)
        client.m_current_iniswap = m_content[1];
    else if (!client.m_current_iniswap.isEmpty())
        client.m_current_iniswap.clear();

    client.getServer()->broadcast(PacketFactory::createPacket("TT", m_content), client.areaId());
}
