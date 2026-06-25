#include "packet/packet_hi.h"

#include "config_manager.h"
#include "db_manager.h"
#include "server.h"

#include <QDebug>

PacketHI::PacketHI(QStringList &contents) :
    AOPacket(contents)
{
}

PacketInfo PacketHI::getPacketInfo() const
{
    PacketInfo info{
        .acl_permission = ACLRole::Permission::NONE,
        .min_args = 1,
        .header = "HI"};
    return info;
}

void PacketHI::handlePacket(AreaData *area, AOClient &client) const
{
    Q_UNUSED(area)

    const QString incoming_hwid = m_content[0];
    if (client.m_hwid.isEmpty()){ // check if this client are 'new'..
        if (incoming_hwid.isEmpty()) // reject an <empty> hwids..
            client.m_socket->close(QWebSocketProtocol::CloseCodeProtocolError, "A protocol error has been encountered.");
        else{
            client.m_hwid = incoming_hwid;
            const auto hashedid = AOClient::calculateHashid(&client);
            emit client.getServer()->logConnectionAttempt(client.m_remote_ip.toString(), client.m_ipid, client.m_hwid);
            if (client.getServer()->ClientWhitelisted(hashedid)){
                auto ban = client.getServer()->getDatabaseManager()->isHDIDBanned(client.m_hwid);
                if (ban.first){
                    QString ban_duration(qMax(-1ll, ban.second.duration) > -1 ? QDateTime::fromSecsSinceEpoch(ban.second.time).addSecs(ban.second.duration).toString("MM/dd/yyyy, hh:mm") : "Permanently.");
                    client.sendPacket("BD", {"Reason: " + ban.second.reason + "\nBan ID: " + QString::number(ban.second.id) + "\nUntil: " + ban_duration});
                    client.m_socket->close();
                }
                else // check if client are reached of client-limts by they hdid..
                    client.getServer()->RegisterClienthwid(client.clientId()) ? client.sendPacket("ID", {QString::number(client.clientId()), "akashi", QCoreApplication::applicationVersion()}) : client.m_socket->close();
            }
            else{
                auto ld_timeout = client.getServer()->lockdown_timeout;
                if (ld_timeout->isActive() && ld_timeout->remainingTime() > 1){
                    QString Message("The server is lockdown for " + AOClient::EpochToString(ld_timeout->remainingTimeAsDuration()) + ".");
                    if (!hashedid.isEmpty())
                        Message.append(QString("\nPlease contact to server owner if you want be whitelisted.\nYour ID: %1").arg(QString::fromUtf8(hashedid)));
                    client.sendPacket("BD", {Message});
                    client.m_socket->close();
                }
                else{
                    QString Message("The server is lockdown.");
                    if (!hashedid.isEmpty())
                        Message.append(QString("\nPlease contact to server owner if you want be whitelisted.\nYour ID: %1").arg(QString::fromUtf8(hashedid)));
                    client.sendPacket("BD", {Message});
                    client.m_socket->close();
                }
            }
        }
    }
    else{ // rejects double sending or not same hwids..
        client.sendPacket("BD", {"A protocol error has been encountered. Packet : HI"});
        client.m_socket->close(QWebSocketProtocol::CloseCodeProtocolError);
    }
}
