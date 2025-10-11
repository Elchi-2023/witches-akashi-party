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
    if (!client.m_joined) /* Don't let outsider be mess this */
        return;

    AOPacket *validated_packet = validateTTPacket(client);
    if (validated_packet->getPacketInfo().header == "INVALID")
        return;

    client.getServer()->broadcast(validated_packet, client.areaId());
    /*
     * area->startMessageFloodguard(ConfigManager::messageFloodguard());
     * client.getServer()->startMessageFloodguard(ConfigManager::globalMessageFloodguard());}
     *
     * i don't thinks that should needs, unless if be needs.
    */
}

bool PacketTT::validatePacket() const{
    return true;
}

AOPacket *PacketTT::validateTTPacket(AOClient &client) const{
    AOPacket *l_invalid = PacketFactory::createPacket("INVALID", {});
    QStringList l_args;

    if (!client.m_is_spectator) /* don't let spectator mess this either */
        return l_invalid;

    QList<QVariant> l_incoming_args;
    for (const QString &l_arg : m_content)
        l_incoming_args.append(QVariant(l_arg)); /* get all packet args */

    /* args checker moment */
    bool validType;
    l_args << QString::number(l_incoming_args[0].toInt(&validType)) << l_incoming_args[1].toString() << l_incoming_args[2].toString();
    if (!validType || l_args[1].trimmed().isEmpty() || l_args[2].trimmed().isEmpty())
        return l_invalid;

    if (l_args[1].toLower() != client.character().toLower()) /* kfo behaviors */
        client.m_current_iniswap = l_args[1];
    else if (!client.m_current_iniswap.isEmpty())
        client.m_current_iniswap.clear();

    return PacketFactory::createPacket("TT", l_args);
}
