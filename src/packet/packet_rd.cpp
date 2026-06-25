#include "packet/packet_rd.h"
#include "config_manager.h"
#include "server.h"

#include <QDebug>

PacketRD::PacketRD(QStringList &contents) :
    AOPacket(contents)
{
}

PacketInfo PacketRD::getPacketInfo() const
{
    PacketInfo info{
        .acl_permission = ACLRole::Permission::NONE,
        .min_args = 0,
        .header = "RD"};
    return info;
}

void PacketRD::handlePacket(AreaData *area, AOClient &client) const{
    if (client.m_hwid.isEmpty()) // No early connecting!
        client.m_socket->close(QWebSocketProtocol::CloseCodeAbnormalDisconnection);
    else if (!client.m_joined){
        QPointer current_server(client.getServer());
        client.m_joined = true;
        current_server->updateCharsTaken(area);
        client.sendEvidenceList(area);
        client.sendPacket("HP", {"1", QString::number(area->defHP())});
        client.sendPacket("HP", {"2", QString::number(area->proHP())});
        client.sendPacket("FA", client.getServer()->getAreaNames());
        // Here lies OPPASS, the genius of FanatSors who send the modpass to everyone in plain text.
        client.sendPacket("DONE");
        client.sendPacket("BN", {area->background(), area->side()});

        static const QString Motd = QString(ConfigManager::motd()).replace('\n', "\r\n");
        if (!Motd.isEmpty())
            client.sendServerMessage("=== MOTD ===\r\n" + Motd + "\r\n=============");
        static const QVariantList VCParams = ConfigManager::GetVoiceParameters();
        client.sendPacket("VS_CAPS", {QString::number(VCParams[ConfigManager::VoiceParameter::ENABLE].toBool()), QString::number(VCParams[ConfigManager::VoiceParameter::PTT].toBool()), VCParams[ConfigManager::VoiceParameter::MAXPEERSAREA].toString(), VCParams[ConfigManager::VoiceParameter::VCODEC].toString(), VCParams[ConfigManager::VoiceParameter::VHZ].toString(), VCParams[ConfigManager::VoiceParameter::VFRAME_MS].toString(), VCParams[ConfigManager::VoiceParameter::MAXBYTES].toString()});

        client.fullArup(); // Give client all the area data
        const QVector<QTimer *> GetTimer(QVector<QTimer *>({client.getServer()->timer}) << area->timers().toVector()); // i know.. this kinda odd but worth i guess..
        for (auto timer : GetTimer){
            const int timer_id = GetTimer.indexOf(timer);
            if (timer->isActive()){
                client.sendPacket("TI", {QString::number(timer_id), "2"});
                client.sendPacket("TI", {QString::number(timer_id), "3", QString::number(timer->remainingTimeAsDuration().count())});
            }
            else
                client.sendPacket("TI", {QString::number(timer_id), "3"});
        }

        emit client.joined();
        area->addClient(client.clientId());
        client.arup(client.ARUPType::PLAYER_COUNT, true); // Tell everyone there is a new player
    }
}
