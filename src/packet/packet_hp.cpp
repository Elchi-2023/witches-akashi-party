#include "packet/packet_hp.h"
#include "akashiutils.h"
#include "packet/packet_factory.h"
#include "server.h"

#include <QDebug>

PacketHP::PacketHP(QStringList &contents) :
    AOPacket(contents)
{
}

PacketInfo PacketHP::getPacketInfo() const
{
    PacketInfo info{
        .acl_permission = ACLRole::Permission::NONE,
        .min_args = 2,
        .header = "HP"};
    return info;
}

void PacketHP::handlePacket(AreaData *area, AOClient &client) const{
    if (!client.m_joined)
        client.m_socket->close(QWebSocketProtocol::CloseCodeProtocolError);
    else if (client.isSpectator())
        client.sendServerMessage("Spectators are blocked from using the judge controls.");
    else{
        switch (area->lockStatus()){
        case AreaData::LockStatus::SPECTATABLE:
            if (client.isAccessBlocked(AOClient::WTCE))
                client.sendServerMessage("You are blocked from using the judge controls.");
            else if (area->invited().contains(client.clientId()) || client.checkPermission(ACLRole::BYPASS_LOCKS)){
                bool side_type_ok;
                const int side_type = m_content[0].toInt(&side_type_ok) -1;
                if (side_type_ok && side_type >= 0 && side_type <= 1){
                    bool penalty_bar_ok;
                    int l_newValue = m_content.at(1).toInt(&penalty_bar_ok);
                    if (penalty_bar_ok){
                        area->changeHP(QList<AreaData::Side>({AreaData::Side::DEFENCE, AreaData::Side::PROSECUTOR})[side_type], l_newValue);
                        client.getServer()->broadcast(PacketFactory::createPacket("HP", {"1", QString::number(area->defHP())}), area->index());
                        client.getServer()->broadcast(PacketFactory::createPacket("HP", {"2", QString::number(area->proHP())}), area->index());

                        client.updateJudgeLog(area, &client, "updated the penalties");
                    }
                }
            }
            else
                client.sendServerMessage("Spectators are blocked from using the judge controls.");
            break;
        case AreaData::LockStatus::LOCKED:
            if (client.isAccessBlocked(AOClient::WTCE))
                client.sendServerMessage("You are blocked from using the judge controls.");
            else if (area->invited().contains(client.clientId()) || client.checkPermission(ACLRole::BYPASS_LOCKS)){
                bool side_type_ok;
                const int side_type = m_content[0].toInt(&side_type_ok) -1;
                if (side_type_ok && side_type >= 0 && side_type <= 1){
                    bool penalty_bar_ok;
                    int l_newValue = m_content.at(1).toInt(&penalty_bar_ok);
                    if (penalty_bar_ok){
                        area->changeHP(QList<AreaData::Side>({AreaData::Side::DEFENCE, AreaData::Side::PROSECUTOR})[side_type], l_newValue);
                        client.getServer()->broadcast(PacketFactory::createPacket("HP", {"1", QString::number(area->defHP())}), area->index());
                        client.getServer()->broadcast(PacketFactory::createPacket("HP", {"2", QString::number(area->proHP())}), area->index());

                        client.updateJudgeLog(area, &client, "updated the penalties");
                    }
                }
            }
            else
                client.sendServerMessage("Uninvited clients are blocked from using the judge controls.");
            break;
        case AreaData::LockStatus::FREE:
            if (client.isAccessBlocked(AOClient::WTCE))
                client.sendServerMessage("You are blocked from using the judge controls.");
            else{
                bool side_type_ok;
                const int side_type = m_content[0].toInt(&side_type_ok) -1;
                if (side_type_ok && side_type >= 0 && side_type <= 1){
                    bool penalty_bar_ok;
                    int l_newValue = m_content.at(1).toInt(&penalty_bar_ok);
                    if (penalty_bar_ok){
                        area->changeHP(QList<AreaData::Side>({AreaData::Side::DEFENCE, AreaData::Side::PROSECUTOR})[side_type], l_newValue);
                        client.getServer()->broadcast(PacketFactory::createPacket("HP", {"1", QString::number(area->defHP())}), area->index());
                        client.getServer()->broadcast(PacketFactory::createPacket("HP", {"2", QString::number(area->proHP())}), area->index());

                        client.updateJudgeLog(area, &client, "updated the penalties");
                    }
                }
            }
            break;
        }
    }
}
