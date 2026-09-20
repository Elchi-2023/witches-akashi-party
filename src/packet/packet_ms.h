#ifndef PACKET_MS_H
#define PACKET_MS_H

#include "network/aopacket.h"
#include <QScopedPointer>
/* MS Json?? */
#include <QJsonDocument>
#include <QJsonParseError>
#include <QJsonArray>
#include <QJsonObject>

class PacketMS : public AOPacket
{
  public:
    PacketMS(QStringList &contents);
    virtual PacketInfo getPacketInfo() const;
    virtual void handlePacket(AreaData *area, AOClient &client) const;

  private:
    /**
     * @brief Create the packet of IC-Message object.
     * @return Valid pointer, Nullptr otherwise.
     */
    QScopedPointer<AOPacket> CreatePacket(AOClient &client, AreaData *area, Server *server) const;
    QRegularExpressionMatch isTestimonyJumpCommand(QString message) const;
};
#endif
