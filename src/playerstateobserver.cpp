#include "playerstateobserver.h"
#include <QPointer>

PlayerStateObserver::PlayerStateObserver(QObject *parent) :
    QObject{parent}
{}

PlayerStateObserver::~PlayerStateObserver() {}

void PlayerStateObserver::registerClient(QPointer<AOClient> client){
    if (!client || m_client_list.contains(client))
        return;

    /* > if ipid didn't generated < */
    if (client->m_ipid.isEmpty())
        client->calculateIpid();

    /* > broadcast to registered client about this client are added < */
    BroadcastRegister(client->clientId(), false);
    m_client_list.append(client);

    connect(m_client_list.last(), &AOClient::UpdateState, this, &PlayerStateObserver::UpdateSender, Qt::ConnectionType::QueuedConnection); // anti-spam emitting..
    connect(m_client_list.last(), &AOClient::ModeratorObserver, this, &PlayerStateObserver::ModeratorRequestsData);
    /* signal broadcast moment */
    connect(this, &PlayerStateObserver::BroadcastRegister, client, &AOClient::sendPlayerStateRegister, Qt::ConnectionType::QueuedConnection);
    connect(this, &PlayerStateObserver::BroadcastUpdate, client, &AOClient::sendPlayerStateUpdate, Qt::ConnectionType::QueuedConnection);

    for (AOClient *i_client : qAsConst(m_client_list)){
        client->sendPlayerStateRegister(i_client->clientId(), false);
        QStringList l_name;
        if (i_client->m_is_afk) l_name.prepend(client->m_version.type == AOClient::ClientVersion::NDS ? "[AFK]" : "[💤]");
        if (!i_client->name().isEmpty()) l_name << i_client->name();
        client->sendPlayerStateUpdate(i_client->clientId(), PacketPU::NAME, l_name.join(' '));
        client->sendPlayerStateUpdate(i_client->clientId(), PacketPU::CHARACTER, i_client->character());
        client->sendPlayerStateUpdate(i_client->clientId(), PacketPU::SHOWNAME, i_client->characterName());
        client->sendPlayerStateUpdate(i_client->clientId(), PacketPU::AREA_ID, i_client->areaId());
    }
}

bool PlayerStateObserver::unregisterClient(QPointer<AOClient> client){
    if (!client || !m_client_list.contains(client))
        return false; /* > if client is null or client aren't in the list < */

    /* > disconnect both side signals < */
    disconnect(client, nullptr, this, nullptr);
    disconnect(this, nullptr, client, nullptr);

    /* > remove client from the registers client < */
    m_client_list.removeAll(client);

    /* > broadcast to registered client about this client are removed < */
    BroadcastRegister(client->clientId(), true);

    return true;
}

void PlayerStateObserver::UpdateSender(const int type){
    const QPointer<AOClient> client_sender = qobject_cast<AOClient *>(sender());
    if (client_sender.isNull())
        return;

    switch (type){
    default:
        return;
    case PacketPU::CHARACTER:
        BroadcastUpdate(client_sender->clientId(), PacketPU::CHARACTER, client_sender->character());
        break;
    case PacketPU::SHOWNAME:
        BroadcastUpdate(client_sender->clientId(), PacketPU::SHOWNAME, client_sender->characterName());
        break;
    case PacketPU::AREA_ID:
        BroadcastUpdate(client_sender->clientId(), PacketPU::AREA_ID, client_sender->areaId());
        break;
    case PacketPU::NAME:
        for (AOClient *client : qAsConst(m_client_list)){
            QStringList l_name(client_sender->name());
            if (client_sender->m_is_afk) l_name.prepend(client->m_version.type == AOClient::ClientVersion::NDS ? "[AFK]" : "[💤]");
            if (client->isMAuthenticated()) l_name.append(client->m_version.is_webao() ? "\n[" + client_sender->getIpid() + "]" : "[" + client_sender->getIpid() + "]");
            client->sendPlayerStateUpdate(client_sender->clientId(), PacketPU::NAME, l_name.join(' '));
        }
        break;
    }
}

void PlayerStateObserver::ModeratorRequestsData(){
    QPointer<AOClient> client_sender = qobject_cast<AOClient *>(sender());
    if (client_sender.isNull())
        return;

    for (AOClient *client : qAsConst(m_client_list)){
        QStringList l_name;
        if (client->m_is_afk) l_name.prepend(client_sender->m_version.type == AOClient::ClientVersion::NDS ? "[AFK]" : "[💤]");
        if (!client->name().isEmpty()) l_name << client->name();
        if (client_sender->isMAuthenticated()) l_name << "[" + client->m_ipid + "]";

         client_sender->sendPlayerStateUpdate(client->clientId(), PacketPU::NAME, l_name.join(' '));
    }
}
