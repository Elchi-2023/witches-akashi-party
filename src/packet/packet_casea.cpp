#include "packet/packet_casea.h"
#include "akashiutils.h"
#include "packet/packet_factory.h"
#include "server.h"

#include <QDebug>

PacketCasea::PacketCasea(QStringList &contents) :
    AOPacket(contents)
{
}

PacketInfo PacketCasea::getPacketInfo() const{
    return PacketInfo::CreateInfo("CASEA", 6);
}

void PacketCasea::handlePacket(AreaData *area, AOClient &client) const{
    static QVector<QString> Param = m_content.toVector(); // why convert to vector?.. cause m_content is in "protected" type..
    const QString casetitle = Param.takeFirst();
    const QStringList ListedRoles = {"defense attorney", "prosecutor", "judge", "jurors", "stenographer"};
    QStringList RequestedRoles;
    QVector<bool> MarkedRoles;
    for (int I = 0; I < Param.size(); ++I){
        const QVariant Var = QVariant::fromValue(Param[I]);
        if (!Var.canConvert(QMetaType::Bool))
            return;

        MarkedRoles.append(Var.toBool());
        if (MarkedRoles.last())
            RequestedRoles.append(ListedRoles[I]);
    }

    if (!RequestedRoles.isEmpty()){ /* only if there's a roles needs */
        const QString Message = QString("=== Case Announcement ===\r\n%1 needs [%2]\nfor %3%4").arg(AOClient::NameWId(&client), RequestedRoles.join(", "), casetitle.trimmed().isEmpty() ? "a case." : casetitle.trimmed() + "!", QPointer<AreaData>(area).isNull() ? "" : QString("\nin [%1] %2").arg(QString::number(area->index()), area->name()));
        const QVector<QPointer<AOClient>> l_clients = client.getServer()->getClients();

        /* heavy for-loop event here if much of clients.. */
        for (auto client : l_clients){
            if (client.isNull() || !client->hasJoined() || QSet<bool>(client->m_casing_preferences.begin(), client->m_casing_preferences.end()).intersect(QSet<bool>(MarkedRoles.begin(), MarkedRoles.end())).isEmpty())
                continue;
            client->sendPacket(PacketFactory::createPacket("CASEA", {Message, Param[0], Param[1], Param[2], Param[3], Param[4], "1"}));
            /* ======= [akashi devs note] =======
             * you may be thinking, "hey wait a minute the network protocol documentation doesn't mention that last argument!"
             * if you are in fact thinking that, you are correct! it is not in the documentation!
             * however for some inscrutable reason Attorney Online 2 will outright reject a CASEA packet that does not have
             * at least 7 arguments despite only using the first 6. Cera, i kneel. you have truly broken me.
             * ================================== */
        }
    }
}
