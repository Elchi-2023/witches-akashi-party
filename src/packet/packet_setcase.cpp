#include "packet/packet_setcase.h"
#include "server.h"

#include <QDebug>

PacketSetcase::PacketSetcase(QStringList &contents) :
    AOPacket(contents)
{
}

PacketInfo PacketSetcase::getPacketInfo() const{
    return PacketInfo::CreateInfo("SETCASE", 7);
}

void PacketSetcase::handlePacket(AreaData *area, AOClient &client) const{
    Q_UNUSED(area)
    if (client.m_joined){
        const QStringList GetParam = m_content.mid(2);
        for (int I = 0; I < client.m_casing_preferences.size(); ++I){
            const QVariant Var = QVariant::fromValue(GetParam[I]);
            if (!Var.canConvert(QMetaType::Bool))
                continue; // skipping instead of killing it by return..
            client.m_casing_preferences[I] = Var.toBool();
        }
    }
    else
        client.m_socket->close(QWebSocketProtocol::CloseCodeProtocolError);
}
