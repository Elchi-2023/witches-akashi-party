#pragma once

#include "aoclient.h"
#include "packet/packet_pr.h"

#include <QList>
#include <QObject>
#include <QString>

class PlayerStateObserver : public QObject
{
  public:
    explicit PlayerStateObserver(QObject *parent = nullptr);
    virtual ~PlayerStateObserver();

    void registerClient(AOClient *client);
    bool unregisterClient(QPointer<AOClient> client);

  private:
    QList<AOClient *> m_client_list;

    /**
     * @brief broadcasting Packet to listed clients
     *
     * @param Packet
     */
    void sendToClientList(const AOPacket &packet);
    /**
     * @brief same like sendToClientList but PacketPR only (and mods can receiving ipid when PacketPR::ADD)
     *
     * @param target client
     *
     * @param PacketPR
     */
    void UploadListStateToClients(const AOClient *client, const PacketPR &State);
    /**
     * @brief same like sendToClientList but mods can receiving ipid.
     *
     * @param sender client.
     *
     * @param AOPacket.
     */
    void UploadStateToClients(const AOClient *client, const AOPacket &packet);

  private Q_SLOTS:
    /**
     * @brief broadcasing the sender client state to everyone..
     */
    void UpdateSender(const int type);
    /**
     * @brief [Moderator] - send everyone cllent state to the [M] sender.
     */
    void ModeratorRequestsData();
};
