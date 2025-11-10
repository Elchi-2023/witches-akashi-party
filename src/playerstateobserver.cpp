#include "playerstateobserver.h"

PlayerStateObserver::PlayerStateObserver(QObject *parent) :
    QObject{parent}
{}

PlayerStateObserver::~PlayerStateObserver() {}

void PlayerStateObserver::registerClient(AOClient *client)
{
    Q_ASSERT(!m_client_list.contains(client));

    UploadListStateToClients(client, PacketPR(client->clientId(), PacketPR::ADD));

    m_client_list.append(client);

    connect(client, &AOClient::nameChanged, this, &PlayerStateObserver::notifyNameChanged);
    connect(client, &AOClient::characterChanged, this, &PlayerStateObserver::notifyCharacterChanged);
    connect(client, &AOClient::characterNameChanged, this, &PlayerStateObserver::notifyCharacterNameChanged);
    connect(client, &AOClient::areaIdChanged, this, &PlayerStateObserver::notifyAreaIdChanged);
    connect(client, &AOClient::ModeratorObserver, this, &PlayerStateObserver::ModeratorRequestsData);

    QVector<QSharedPointer<AOPacket>> packets; /* why QSharedPointer (smart pointer)?.. because we need release packets after. (unlike the originally code) */
    for (AOClient *i_client : qAsConst(m_client_list)) {
        packets.append(QSharedPointer<PacketPR>::create(i_client->clientId(), PacketPR::ADD));
        QStringList l_name(i_client->name());
        if (i_client->m_is_afk) l_name.prepend("[💤]");
        packets.append(QSharedPointer<PacketPU>::create(i_client->clientId(), PacketPU::NAME, l_name.join(' ')));
        packets.append(QSharedPointer<PacketPU>::create(i_client->clientId(), PacketPU::CHARACTER, i_client->character()));
        packets.append(QSharedPointer<PacketPU>::create(i_client->clientId(), PacketPU::CHARACTER_NAME, i_client->characterName()));
        packets.append(QSharedPointer<PacketPU>::create(i_client->clientId(), PacketPU::AREA_ID, i_client->areaId()));
    }

    for (auto packet : qAsConst(packets))
        client->sendPacket(packet.get());
}

void PlayerStateObserver::unregisterClient(AOClient *client)
{
    Q_ASSERT(m_client_list.contains(client));

    disconnect(client, nullptr, this, nullptr);

    m_client_list.removeAll(client);

    UploadListStateToClients(client, PacketPR(client->clientId(), PacketPR::REMOVE));
}

void PlayerStateObserver::UploadListStateToClients(const AOClient *client, const PacketPR &State){
    const QString get_ipid = client->m_ipid; /* avoiding "warning : unused client" */
    for (AOClient *clients : qAsConst(m_client_list)){
        clients->sendPacket(&const_cast<PacketPR &>(State));
        if (clients->isAuthenticated() && const_cast<PacketPR &>(State).getContent()[1].toInt() == PacketPR::ADD)
            clients->sendPacket(QSharedPointer<PacketPU>::create(const_cast<PacketPR &>(State).getContent()[0].toInt(), PacketPU::NAME, "(" + get_ipid + ")").get());
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
        if (client->isAuthenticated()){
            const QStringList args = const_cast<AOPacket &>(packet).getContent();
            if (args[1].toInt() == PacketPU::NAME)
                clients->sendPacket(QSharedPointer<PacketPU>::create(args[0].toInt(), PacketPU::NAME, QStringList({args[2], "(" + client->m_ipid + ")"}).join(' ')).get());
            else{
                clients->sendPacket(&const_cast<AOPacket &>(packet));
                clients->sendPacket(QSharedPointer<PacketPU>::create(args[0].toInt(), PacketPU::NAME, QStringList({args[2], "(" + client->m_ipid + ")"}).join(' ')).get());
            }
        }
        else
            clients->sendPacket(&const_cast<AOPacket &>(packet));
    }
}

void PlayerStateObserver::notifyNameChanged(const QString &name)
{
    UploadStateToClients(qobject_cast<AOClient *>(sender()), PacketPU(qobject_cast<AOClient *>(sender())->clientId(), PacketPU::NAME, name));
}

void PlayerStateObserver::notifyCharacterChanged(const QString &character)
{
    UploadStateToClients(qobject_cast<AOClient *>(sender()), PacketPU(qobject_cast<AOClient *>(sender())->clientId(), PacketPU::CHARACTER, character));
}

void PlayerStateObserver::notifyCharacterNameChanged(const QString &characterName)
{
    UploadStateToClients(qobject_cast<AOClient *>(sender()), PacketPU(qobject_cast<AOClient *>(sender())->clientId(), PacketPU::CHARACTER_NAME, characterName));
}

void PlayerStateObserver::notifyAreaIdChanged(int areaId)
{
    UploadStateToClients(qobject_cast<AOClient *>(sender()), PacketPU(qobject_cast<AOClient *>(sender())->clientId(), PacketPU::AREA_ID, areaId));
}

void PlayerStateObserver::ModeratorRequestsData(){
    AOClient *client_sender = qobject_cast<AOClient *>(sender());
    if (client_sender == nullptr)
        return;
    for (AOClient *client : qAsConst(m_client_list)){
        QStringList l_name(client->name());
        if (client->m_is_afk) l_name.prepend("[💤]");
        if (client_sender->m_authenticated)
            client_sender->sendPacket(QSharedPointer<PacketPU>::create(client->clientId(), PacketPU::NAME, QStringList({l_name.join(' '), "(" + client->m_ipid + ")"}).join(' ')).get());
        else
            client_sender->sendPacket(QSharedPointer<PacketPU>::create(client->clientId(), PacketPU::NAME, l_name.join(' ')).get());
    }
}
