#include "packet/packet_rt.h"
#include "packet/packet_factory.h"
#include "server.h"

#include <QDebug>

PacketRT::PacketRT(QStringList &contents) :
    AOPacket(contents)
{
}

PacketInfo PacketRT::getPacketInfo() const
{
    PacketInfo info{
        .acl_permission = ACLRole::Permission::NONE,
        .min_args = 1,
        .header = "RT"};
    return info;
}

void PacketRT::handlePacket(AreaData *area, AOClient &client) const{
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
                if (area->isWtceAllowed()){
                    if (QDateTime::currentDateTime().toSecsSinceEpoch() - client.m_last_wtce_time <= 5)
                        return;

                    client.m_last_wtce_time = QDateTime::currentDateTime().toSecsSinceEpoch();
                    client.getServer()->broadcast(PacketFactory::createPacket("RT", m_content), client.areaId());
                    client.updateJudgeLog(area, &client, "WT/CE");
                }
                else
                    client.sendServerMessage("WTCE animations have been disabled in this area.");
            }
            else
                client.sendServerMessage("Spectators are blocked from using the judge controls.");
            break;
        case AreaData::LockStatus::LOCKED:
            if (client.isAccessBlocked(AOClient::WTCE))
                client.sendServerMessage("You are blocked from using the judge controls.");
            else if (area->invited().contains(client.clientId()) || client.checkPermission(ACLRole::BYPASS_LOCKS)){
                if (area->isWtceAllowed()){
                    if (QDateTime::currentDateTime().toSecsSinceEpoch() - client.m_last_wtce_time <= 5)
                        return;

                    client.m_last_wtce_time = QDateTime::currentDateTime().toSecsSinceEpoch();
                    client.getServer()->broadcast(PacketFactory::createPacket("RT", m_content), client.areaId());
                    client.updateJudgeLog(area, &client, "WT/CE");
                }
                else
                    client.sendServerMessage("WTCE animations have been disabled in this area.");
            }
            else
                client.sendServerMessage("Uninvited clients are blocked from using the judge controls.");
            break;
        default:
            if (client.isAccessBlocked(AOClient::WTCE))
                client.sendServerMessage("You are blocked from using the judge controls.");
            else if (area->isWtceAllowed()){
                if (QDateTime::currentDateTime().toSecsSinceEpoch() - client.m_last_wtce_time <= 5)
                    return;

                client.m_last_wtce_time = QDateTime::currentDateTime().toSecsSinceEpoch();
                client.getServer()->broadcast(PacketFactory::createPacket("RT", m_content), client.areaId());
                client.updateJudgeLog(area, &client, "WT/CE");
            }
            else
                client.sendServerMessage("WTCE animations have been disabled in this area.");
            break;
        }
    }
}
