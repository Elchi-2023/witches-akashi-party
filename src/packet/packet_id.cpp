#include "packet/packet_id.h"

#include "config_manager.h"
#include "server.h"

#include <QDebug>

PacketID::PacketID(QStringList &contents) :
    AOPacket(contents){
}

PacketInfo PacketID::getPacketInfo() const
{
    PacketInfo info{
        .acl_permission = ACLRole::Permission::NONE,
        .min_args = 2,
        .header = "ID"};
    return info;
}

bool isRealBrowser(const QString &userAgent){
    // a proper browser almost always whispers this pattern..
    static const QRegularExpression re(QLatin1String(R"(^Mozilla/5\.0\s\([^)]+\)\s.*\b(AppleWebKit|Chrome/|Firefox/|Gecko/|Goanna/|Presto/|KHTML/|NetFront/|Servo/|Edge/)\b)"), QRegularExpression::CaseInsensitiveOption);

    // if the pattern fits gently, it might just be a real browser..
    return re.match(userAgent).hasMatch();
}
bool isPhoneBrowser(const QString &userAgent) {
    // first, it needs to feel like a real browser..
    if (!isRealBrowser(userAgent))
        return false;

    // tiny signs that it lives on a phone or tablet..
    static const QRegularExpression re(QLatin1String(R"(\b(iPhone|iPad|iPod|Android|Mobile|BlackBerry|IEMobile|Opera Mini|Opera Mobi|webOS|Windows Phone|Silk/)\b)"), QRegularExpression::CaseInsensitiveOption);

    // if it carries one of these little marks, it's probably resting in small hands..
    return re.match(userAgent).hasMatch();
}

void PacketID::handlePacket(AreaData *area, AOClient &client) const
{
    Q_UNUSED(area)
    
    if (client.m_version.release == 2) {
        // No double sending of the ID packet!
        client.sendPacket("BD", {"A protocol error has been encountered. Packet : ID"});
        client.m_socket->close(QWebSocketProtocol::CloseCode::CloseCodeProtocolError);
        return;
    }
    const QMap<QString, AOClient::ClientVersion::ClientType> MatcherVer{
        {"ao2", {AOClient::ClientVersion::ClientType::NORMAL}},
        {"ao-nds", {AOClient::ClientVersion::ClientType::NDS}},
        {"ao2xp", {AOClient::ClientVersion::ClientType::XP}},
        {"webao", {AOClient::ClientVersion::ClientType::WEBAO}},
        {"dro", {AOClient::ClientVersion::DRO}}
    };

    const AOClient::ClientVersion::ClientType current_ver = MatcherVer.value(m_content[0].toLower(), isRealBrowser(client.m_socket->GetUseragent()) ? AOClient::ClientVersion::ClientType::WEBAO : AOClient::ClientVersion::ClientType::NORMAL);
    switch (current_ver){
    case AOClient::ClientVersion::ClientType::DRO:
        client.sendPacket("BD", {"This server doesn't supported DRO Client (yet)."});
        client.m_socket->close(QWebSocketProtocol::CloseCode::CloseCodeProtocolError);
        return;
    case AOClient::ClientVersion::ClientType::WEBAO:
        if (ConfigManager::webaoEnabled()){
            QRegularExpression rx("\\b(\\d+)\\.(\\d+)\\.(\\d+)\\b"); // matches X.X.X (e.g. 2.9.0, 2.4.10, etc.)
            QRegularExpressionMatch l_match = rx.match(m_content[1]);
            if (l_match.hasMatch()) {
                client.m_version.release = l_match.captured(1).toInt();
                client.m_version.major = l_match.captured(2).toInt();
                client.m_version.minor = l_match.captured(3).toInt();
                client.m_version.type = isPhoneBrowser(client.m_socket->GetUseragent()) ? AOClient::ClientVersion::ClientType::WEBAOPHONE :  AOClient::ClientVersion::ClientType::WEBAO;
            }
            
            if (client.m_version.release != 2) {
                client.sendPacket("BD", {"A protocol error has been encountered. Packet : ID\nRelease version not recognised."});
                client.m_socket->close();
                return;
            }
        }
        else{
            client.sendPacket("BD", {"WebAO is disabled on this server."});
            client.m_socket->close(QWebSocketProtocol::CloseCode::CloseCodeProtocolError);
            return;
        }
        break;
    default:
        QRegularExpression rx("\\b(\\d+)\\.(\\d+)\\.(\\d+)\\b"); // matches X.X.X (e.g. 2.9.0, 2.4.10, etc.)
        QRegularExpressionMatch l_match = rx.match(m_content[1]);
        if (l_match.hasMatch()) {
            client.m_version.release = l_match.captured(1).toInt();
            client.m_version.major = l_match.captured(2).toInt();
            client.m_version.minor = l_match.captured(3).toInt();
            client.m_version.type = current_ver;
        }
        
        if (client.m_version.release != 2) {
            client.sendPacket("BD", {"A protocol error has been encountered. Packet : ID\nRelease version not recognised."});
            client.m_socket->close();
            return;
        }
        break;
    }

    client.sendPacket("PN", {QString::number(client.getServer()->getPlayerCount()), QString::number(ConfigManager::maxPlayers()), ConfigManager::serverDescription()});

    QStringList l_feature_list = {
        "noencryption", "yellowtext", "prezoom",
        "flipping", "customobjections", "fastloading",
        "deskmod", "evidence", "cccc_ic_support",
        "arup", "casing_alerts", "modcall_reason",
        "looping_sfx", "additive", "effects",
        "y_offset", "expanded_desk_mods", "auth_packet", "custom_blips", "voice_packet"};
    client.sendPacket("FL", l_feature_list);

    if (ConfigManager::assetUrl().isValid()) {
        QByteArray l_asset_url = ConfigManager::assetUrl().toEncoded(QUrl::EncodeSpaces);
        client.sendPacket("ASS", {l_asset_url});
    }
}
