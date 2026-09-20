#pragma once

#include "aoclient.h"
#include "packet/packet_pr.h"

#include <QList>
#include <QObject>
#include <QString>

class PlayerStateObserver : public QObject
{
    Q_OBJECT
public:
    explicit PlayerStateObserver(QObject *parent = nullptr);
    virtual ~PlayerStateObserver();

    void registerClient(QPointer<AOClient> client);
    bool unregisterClient(QPointer<AOClient> client);

Q_SIGNALS:
    /**
     * @brief The broadcasing the target client state to every registered clients.
     */
    void BroadcastUpdate(const int c_from, const int type, const QVariant &value);
    /**
     * @brief The broadcasing the target client (un)registering state to every registered clients.
     */
    void BroadcastRegister(const int c_from, const bool remove);
private:
    /**
     * @brief The registered client list of playerstate observer.
     */
    QList<AOClient *> m_client_list;

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
