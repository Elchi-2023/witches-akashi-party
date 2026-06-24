#include "playerstateobserver.h"
#include <QPointer>

PlayerStateObserver::PlayerStateObserver(QObject *parent) :
    QObject{parent}
{}

PlayerStateObserver::~PlayerStateObserver() {}

void PlayerStateObserver::registerClient(AOClient *client)
{
    Q_ASSERT(!m_client_list.contains(client));

    if (client->m_ipid.isEmpty())
        client->calculateIpid();
    UploadListStateToClients(client, PacketPR(client->clientId(), PacketPR::ADD));

    m_client_list.append(client);

    connect(client, &AOClient::UpdateState, this, &PlayerStateObserver::UpdateSender, Qt::ConnectionType::QueuedConnection); // anti-spam emitting..
    connect(client, &AOClient::ModeratorObserver, this, &PlayerStateObserver::ModeratorRequestsData);

    QVector<QSharedPointer<AOPacket>> packets; /* why QSharedPointer (smart pointer)?.. because we need release packets after. (unlike the originally code) */
    for (AOClient *i_client : qAsConst(m_client_list)) {
        packets.append(QSharedPointer<PacketPR>::create(i_client->clientId(), PacketPR::ADD));
        QStringList l_name;
        if (i_client->m_is_afk) l_name.prepend(client->m_version.type == AOClient::ClientVersion::NDS ? "[AFK]" : "[💤]");
        if (!i_client->name().isEmpty()) l_name << i_client->name();
        packets.append(QSharedPointer<PacketPU>::create(i_client->clientId(), PacketPU::NAME, l_name.join(' ')));
        packets.append(QSharedPointer<PacketPU>::create(i_client->clientId(), PacketPU::CHARACTER, i_client->character()));
        packets.append(QSharedPointer<PacketPU>::create(i_client->clientId(), PacketPU::SHOWNAME, i_client->characterName()));
        packets.append(QSharedPointer<PacketPU>::create(i_client->clientId(), PacketPU::AREA_ID, i_client->areaId()));
    }

    for (auto packet : qAsConst(packets))
        client->sendPacket(packet);
}

bool PlayerStateObserver::unregisterClient(QPointer<AOClient> client){
    if (!client || !m_client_list.contains(client))
        return false;

    disconnect(client, nullptr, this, nullptr);

    m_client_list.removeAll(client);

    UploadListStateToClients(client, PacketPR(client->clientId(), PacketPR::REMOVE));

    return true;
}

void PlayerStateObserver::UploadListStateToClients(const AOClient *client, const PacketPR &State){
    for (AOClient *clients : qAsConst(m_client_list)){
        clients->sendPacket(&const_cast<PacketPR &>(State));
        if (clients->isMAuthenticated() && const_cast<PacketPR &>(State).getContent()[1].toInt() == PacketPR::ADD && !client->m_ipid.isEmpty())
            clients->sendPacket(QSharedPointer<PacketPU>::create(client->clientId(), PacketPU::NAME, "(" + client->m_ipid + ")").get());
    }
}

void PlayerStateObserver::sendToClientList(const AOPacket &packet)
{
    for (AOClient *client : qAsConst(m_client_list))
        client->sendPacket(&const_cast<AOPacket &>(packet));
}

void PlayerStateObserver::UploadStateToClients(const AOClient *client, const AOPacket &packet){
    if (client == nullptr || const_cast<AOPacket &>(packet).getContent()[0].toInt() != client->clientId())
        return;

    for (AOClient *clients : qAsConst(m_client_list)){
        if (clients->isMAuthenticated()){
            const QStringList args = const_cast<AOPacket &>(packet).getContent();
            switch (args[1].toInt()){
            case PacketPU::NAME:
                clients->sendPacket(QSharedPointer<PacketPU>::create(args[0].toInt(), PacketPU::NAME, QStringList({args[2], "(" + client->m_ipid + ")"}).join(const_cast<AOClient *>(client)->m_version.is_webao() ? '\n' : ' ')).get()); /* see the reason at line 81.. */
                break;
            default:
                clients->sendPacket(&const_cast<AOPacket &>(packet));
                QStringList l_name("(" + client->m_ipid + ")");
                if (!client->name().isEmpty()){
                    if (const_cast<AOClient *>(client)->m_version.is_webao())
                        l_name.insert(1, client->name() + "\n"); /* you might asking to me about why "\n" for webao?.. cause webao having lacky of playerlist layout.. */
                    else
                        l_name[l_name.size() -1].prepend(client->name() + " ");
                }
                if (client->m_is_afk) l_name.prepend(clients->m_version.type == AOClient::ClientVersion::NDS ? "[AFK]" : "[💤]");
                clients->sendPacket(QSharedPointer<PacketPU>::create(args[0].toInt(), PacketPU::NAME, l_name.join(' ')).get());
                break;
            }
        }
        else
            clients->sendPacket(&const_cast<AOPacket &>(packet));
    }
}

void PlayerStateObserver::UpdateSender(const int type){
    const QPointer<AOClient> client_sender = qobject_cast<AOClient *>(sender());
    if (client_sender.isNull())
        return;

    for (AOClient *client : qAsConst(m_client_list)){
        switch (type){
        default:
            return;
        case PacketPU::CHARACTER:
            client->sendPacket(QSharedPointer<PacketPU>::create(client_sender->clientId(), PacketPU::CHARACTER, client_sender->character()));
            break;
        case PacketPU::SHOWNAME:
            client->sendPacket(QSharedPointer<PacketPU>::create(client_sender->clientId(), PacketPU::SHOWNAME, client_sender->characterName()));
            break;
        case PacketPU::AREA_ID:
            client->sendPacket(QSharedPointer<PacketPU>::create(client_sender->clientId(), PacketPU::AREA_ID, client_sender->areaId()));
            break;
        case PacketPU::NAME:
            QStringList l_name(client_sender->name());
            if (client_sender->m_is_afk) l_name.prepend(client->m_version.type == AOClient::ClientVersion::NDS ? "[AFK]" : "[💤]");
            if (client->isMAuthenticated()) l_name.append(client->m_version.is_webao() ? "\n(" + client_sender->getIpid() + ")" : "(" + client_sender->getIpid() + ")");
            client->sendPacket(QSharedPointer<PacketPU>::create(client_sender->clientId(), PacketPU::NAME, l_name.join(' ')));
            break;
        }
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
        if (client_sender->isMAuthenticated())
            client_sender->sendPacket(QSharedPointer<PacketPU>::create(client->clientId(), PacketPU::NAME, QStringList({l_name.join(' '), "(" + client->m_ipid + ")"}).join(client->m_version.is_webao() ? '\n' : ' ')).get());
        else
            client_sender->sendPacket(QSharedPointer<PacketPU>::create(client->clientId(), PacketPU::NAME, l_name.join(' ')).get());
    }
}
