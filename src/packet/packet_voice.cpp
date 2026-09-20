#include "packet/packet_voice.h"
#include "config_manager.h"
#include "packet/packet_factory.h"
#include "server.h"

namespace PacketVoice{

Join::Join(QStringList &contents) :
    AOPacket(contents){
}

PacketInfo Join::getPacketInfo() const{
    return PacketInfo::CreateInfo("VS_JOIN");
}

void Join::handlePacket(AreaData *area, AOClient &client) const{
    if (!client.m_joined) // eject this user if they not joined.
        client.m_socket->close();
    else if (ConfigManager::GetVoiceParameter().toBool()){
        if (client.isAccessBlocked(AOClient::BlockType::OOC)) {
            client.sendServerMessage("You cannot use voice-chat right now.");
            client.sendPacket("VS_LEAVE", {QString::number(client.clientId())}); // from what i seeking from lemmyao.. it didn't tell if user can join or not so..
        }
        else {
            const int userId = client.clientId();
            if (area->GetRegisteredVoiceMap().contains(userId)){ // already in? just resend the peer list so they can refresh..
                QStringList peers = area->GetRegisteredVoice();
                peers.removeAll(QString::number(userId));
                client.sendPacket("VS_PEERS", peers);
            }
            else if (!area->isVoiceChatAllowed()){ // tell user if this area are not <vc_enable>
                client.sendServerMessage("Voice-chat isn't permitted in this area.");
                client.sendPacket("VS_LEAVE", {QString::number(userId)});
            }
            else if (ConfigManager::GetVoiceParameter(ConfigManager::MAXPEERSAREA).toInt() > 0 && area->GetRegisteredVoiceID().size() >= ConfigManager::GetVoiceParameter(ConfigManager::MAXPEERSAREA).toInt()){ // tell user if this area vc are full
                client.sendServerMessage("Voice-chat users are full in this area.");
                client.sendPacket("VS_LEAVE", {QString::number(userId)});
            }
            else if (client.isAccessBlocked(AOClient::BlockType::VOICE)){ // this access_blocked still counts as "muted"/"block".. since i didn't want touch the db for the vc-ban stuff..
                QString Message("You are not permitted for using voice-chat");
                const QTimer *current_timer = client.GetVCBlockTimer();
                if (current_timer->isActive())
                    Message.append(QString(" (%1s remaining)").arg(AOClient::EpochToString(std::chrono::duration_cast<std::chrono::seconds>(current_timer->remainingTimeAsDuration()))));
                if (!client.GetVCBlockReason().isEmpty())
                    Message.append("\nReason: " + client.GetVCBlockReason());
                client.sendServerMessage(Message);
                client.sendPacket("VS_LEAVE", {QString::number(userId)});
            }
            else if (area->lockStatus() >= AreaData::LockStatus::LOCKED && !area->invited().contains(userId) && !client.checkPermission(ACLRole::BYPASS_LOCKS)){ // <area-lock> state applies here..
                client.sendServerMessage("Uninvited client cannot join voice-chat.");
                client.sendPacket("VS_LEAVE", {QString::number(userId)});
            }
            else if (area->RegisterVoice(userId)) // pass.. let's registering this user onto vc-user list..
                Q_EMIT client.getServer()->broadcastVJoinLeave(userId, false, area->index());
            else // otherwise.. tell user if they are already in.
                client.sendServerMessage("You are already join voice-chat in this area.");
        }
    }
    else // who'll know if user would got bloated by spamming this..
        client.sendServerMessage("Voice-chat is disabled on this server.");
}

Leave::Leave(QStringList &contents) :
    AOPacket(contents){
}

PacketInfo Leave::getPacketInfo() const{
    return PacketInfo::CreateInfo("VS_LEAVE");
}

void Leave::handlePacket(AreaData *area, AOClient &client) const{
    if (!client.m_joined)
        client.m_socket->close();
    else if (ConfigManager::GetVoiceParameter().toBool() && !QPointer<AreaData>(area).isNull()){
        const int user_id = client.clientId();
        if (area->RegisterVoice(user_id, true))
            Q_EMIT client.getServer()->broadcastVJoinLeave(user_id, true, area->index());
    }
}

AudioFrame::AudioFrame(QStringList &contents) :
    AOPacket(contents){
}

PacketInfo AudioFrame::getPacketInfo() const{
    return PacketInfo::CreateInfo("VS_FRAME", 1);
}

void AudioFrame::handlePacket(AreaData *area, AOClient &client) const{
    if (!client.m_joined)
        client.m_socket->close();
    else if (ConfigManager::GetVoiceParameter().toBool() && !QPointer<AreaData>(area).isNull()){
        const int user_id = client.clientId();
        const auto current_user = area->GetRegisteredVoiceMap();
        if (!current_user.contains(user_id) || !current_user[user_id])
            return; // not if this user are not in between <vc_user> or <vc_peers>..
        const QByteArray Data = QByteArray::fromBase64(m_content[0].toLatin1());
        if (Data.isEmpty() || Data.size() > ConfigManager::GetVoiceParameter(ConfigManager::MAXBYTES).toInt())
            return; // never pass if this <b64_encode> are empty or more than max..
        if (client.isAccessBlocked(AOClient::BlockType::VOICE))
            return; // never be excepting if this user can bypassing while they are in <vc_blocked> state..

        Q_EMIT client.getServer()->broadcastVFrame(user_id, Data, area->index());
    }
}

SpeakState::SpeakState(QStringList &contents) :
    AOPacket(contents){
}

PacketInfo SpeakState::getPacketInfo() const{
    return PacketInfo::CreateInfo("VS_SPEAK", 1);
}

void SpeakState::handlePacket(AreaData *area, AOClient &client) const{
    if (!client.m_joined)
        client.m_socket->close();
    else if (ConfigManager::GetVoiceParameter().toBool() && !QPointer<AreaData>(area).isNull()){
        const int user_id = client.clientId();
        const auto current_map = area->GetRegisteredVoiceMap();
        if (!current_map.contains(user_id))
            return; // connot pass if user aren't in <vc_user>..

        bool state_ok;
        const int toggle = m_content[0].toInt(&state_ok);
        if (!state_ok || toggle < 0 || toggle > 1)
            return; // reject an invalid arg..

        if (client.isAccessBlocked(AOClient::BlockType::VOICE)){
            if (current_map[user_id]){
                area->SetVoicePeerState(user_id, false);
                Q_EMIT client.getServer()->broadcastVState(user_id, false, area->index());
            }
            QString Message("You cannot speak in voice-chat while you are been blocked in voice-chat");
            const QTimer *current_timer = client.GetVCBlockTimer();
            if (current_timer->isActive())
                Message.append(QString(" (%1s remaining)").arg(AOClient::EpochToString(std::chrono::duration_cast<std::chrono::seconds>(current_timer->remainingTimeAsDuration()))));
            if (!client.GetVCBlockReason().isEmpty())
                Message.append("\nReason: " + client.GetVCBlockReason());
            client.sendServerMessage(Message);
        }
        else if (current_map[user_id] != toggle){ // preventing been calls 2x if the state are same..
            area->SetVoicePeerState(user_id, toggle);
            Q_EMIT client.getServer()->broadcastVState(user_id, toggle, area->index());
        }
    }
}

}
