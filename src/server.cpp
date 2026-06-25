//////////////////////////////////////////////////////////////////////////////////////
//    akashi - a server for Attorney Online 2                                       //
//    Copyright (C) 2020  scatterflower                                             //
//                                                                                  //
//    This program is free software: you can redistribute it and/or modify          //
//    it under the terms of the GNU Affero General Public License as                //
//    published by the Free Software Foundation, either version 3 of the            //
//    License, or (at your option) any later version.                               //
//                                                                                  //
//    This program is distributed in the hope that it will be useful,               //
//    but WITHOUT ANY WARRANTY; without even the implied warranty of                //
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                 //
//    GNU Affero General Public License for more details.                           //
//                                                                                  //
//    You should have received a copy of the GNU Affero General Public License      //
//    along with this program.  If not, see <https://www.gnu.org/licenses/>.        //
//////////////////////////////////////////////////////////////////////////////////////
#include "server.h"

#include "acl_roles_handler.h"
#include "aoclient.h"
#include "area_data.h"
#include "command_extension.h"
#include "config_manager.h"
#include "db_manager.h"
#include "discord.h"
#include "logger/u_logger.h"
#include "music_manager.h"
#include "network/network_socket.h"
#include "packet/packet_factory.h"
#include "packet/packet_ct.h"
#include "serverpublisher.h"

Server::Server(int p_ws_port, QObject *parent) :
    QObject(parent),
    m_port(p_ws_port),
    m_player_count(0)
{
    timer = new QTimer(this);
    lockdown_timeout = new QTimer(this);
    lockdown_timeout->setSingleShot(true);

    db_manager = new DBManager;
    medieval_parser = new MedievalParser;

    acl_roles_handler = new ACLRolesHandler(this);
    acl_roles_handler->loadFile("config/acl_roles.ini");

    command_extension_collection = new CommandExtensionCollection;
    command_extension_collection->setCommandNameWhitelist(AOClient::COMMANDS.keys());
    command_extension_collection->loadFile("config/command_extensions.ini");

    // We create it, even if its not used later on.
    discord = new Discord(this);

    logger = new ULogger(this);
    connect(this, &Server::logConnectionAttempt, logger, &ULogger::logConnectionAttempt);
    connect(lockdown_timeout, &QTimer::timeout, this, [=](){
        if (m_lockdown_mode){
            m_lockdown_mode = false;
            broadcast(PacketCT::CreateMessageS("The lockdown now expired."), AOClient::AuthenticateType::MODERATOR);
        }
    });
    connect(this, &QObject::destroyed, this, [=]{
        Q_EMIT this->Forcedcloseclients("You have been disconnected due of the server closed.");
    });

    AOPacket::registerPackets();
}

void Server::start()
{
    QString bind_ip = ConfigManager::bindIP();
    const QHostAddress bind_addr = bind_ip == "all" ? QHostAddress::Any : QHostAddress(bind_ip);

    if (bind_addr != QHostAddress::Any && bind_addr.protocol() == QAbstractSocket::NetworkLayerProtocol::UnknownNetworkLayerProtocol)
        qDebug() << "[W][AKASHI]: " << bind_ip << "is an invalid IP address to listen on! Server not starting, check your config.";

    server = new QWebSocketServer("Akashi", QWebSocketServer::NonSecureMode, this);
    if (server->listen(bind_addr, m_port)){
        connect(server, &QWebSocketServer::newConnection, this, &Server::clientConnected);
        qInfo().noquote() << "[I][AKASHI][Socket]: Server listening on"  << (server->serverAddress() == QHostAddress::Any ? "(ALL of)" : server->serverAddress().toString()) << server->serverPort();
        connect(server, &QWebSocketServer::acceptError, this, [=](QAbstractSocket::SocketError socketError){
            qDebug() << "[D][AKASHI][Socket]: Server error:" << socketError << ":" << server->errorString();
        });connect(server, &QWebSocketServer::peerVerifyError, this, [=](const QSslError &error){
            qDebug() << "[D][AKASHI][Socket]: Server peer error:" << error.errorString();
        });
    }
    else
        qDebug() << "[D][AKASHI][Socket]: Server error:" << server->errorString();

    // Checks if any Discord webhooks are enabled.
    handleDiscordIntegration();

    // Construct modern advertiser if enabled in config
    server_publisher = new ServerPublisher(server->serverPort(), &m_player_count, this);

    // Get characters from config file
    qInfo() << "[I][AKASHI]: Registering Characters..";
    m_characters = ConfigManager::characterlistVerbose();

    // Get backgrounds from config file
    qInfo() << "[I][AKASHI]: Registering Backgrounds..";
    m_backgrounds = ConfigManager::backgrounds();

    // Build our music manager.
    qInfo() << "[I][AKASHI]: Registering Music..";
    MusicList l_musiclist = ConfigManager::musiclist();
    music_manager = new MusicManager(ConfigManager::cdnList(), l_musiclist, ConfigManager::ordered_songs(), this);
    connect(music_manager, &MusicManager::sendFMPacket, this, &Server::unicast);
    connect(music_manager, &MusicManager::sendAreaFMPacket, this, QOverload<AOPacket *, int>::of(&Server::broadcast));

    // Get musiclist from config file
    m_music_list = music_manager->rootMusiclist();

    // Assembles the area list
    qInfo() << "[I][AKASHI]: Registering & Assembles Areas..";
    m_area_names = ConfigManager::sanitizedAreaNames();
    for (int i = 0; i < m_area_names.length(); i++) {
        QString area_name = QString::number(i) + ":" + m_area_names[i];
        m_areas.append(new AreaData(area_name, i, music_manager));
        AreaData *l_area = m_areas.last();
        connect(l_area, &AreaData::sendAreaPacket, this, QOverload<AOPacket *, int>::of(&Server::broadcast));
        connect(l_area, &AreaData::sendAreaPacketClient, this, &Server::unicast);
        connect(l_area, &AreaData::userJoinedArea, music_manager, &MusicManager::userJoinedArea);
        connect(this, &Server::RemoveDisconnectCA, l_area, &AreaData::RemoveDClient);
        connect(this, &Server::ReloadAreas, l_area, &AreaData::UpdateName);
        music_manager->registerArea(i);
    }

    // Loads the command help information. This is not stored inside the server.
    qInfo() << "[I][AKASHI]: Registering & Assembles command-help (commandhelp.json)..";
    ConfigManager::loadCommandHelp();

    // Get IP bans
    qInfo() << "[I][AKASHI]: Registering IPBans..";
    m_ipban_list = ConfigManager::iprangeBans();

    // Rate-Limiter for IC-Chat
    m_message_floodguard_timer = new QTimer(this);
    m_message_floodguard_timer->setSingleShot(true);
    connect(m_message_floodguard_timer, &QTimer::timeout, this, &Server::allowMessage);

    // Prepare player IDs and reference hash.
    const int GetMaxPlayer = qMax(1, ConfigManager::maxPlayers());
    while (m_available_ids.size() != GetMaxPlayer)
        m_available_ids.push(m_clients_ids.insert(m_available_ids.size(), nullptr).key());
    std::reverse(m_available_ids.begin(), m_available_ids.end()); // reversing order from 0..1..2.. to like 100.. 99.. 98.. and so on..
    qInfo() << "[I][AKASHI]: Software started.";
}

QVector<QPointer<AOClient>> Server::getClients()
{
    return m_clients;
}

void Server::clientConnected(){
    QWebSocket *socket = server->nextPendingConnection();

    // Too many players. Reject connection!
    // This also enforces the maximum playercount.
    if (m_available_ids.empty()) {
        socket->sendTextMessage(PacketFactory::createPacket("BD", {"Maximum playercount has been reached."})->toUtf8());
        socket->close();
        return;
    }

    auto Getban = db_manager->isIPBanned(AOClient::calculateIpid(socket->peerAddress()));
    if (Getban.first){ // check if this client are in ban list by ipids..
        const QString ban_duration = qMax(-1ll, Getban.second.duration) > -1 ? QDateTime::fromSecsSinceEpoch(Getban.second.time).addSecs(Getban.second.duration).toString("MM/dd/yyyy, hh:mm") : "Permanently.";

        socket->sendTextMessage(PacketFactory::createPacket("BD", {"Reason: " + Getban.second.reason + "\nBan ID: " + QString::number(Getban.second.id) + "\nUntil: " + ban_duration})->toUtf8());
        socket->close(QWebSocketProtocol::CloseCodeNormal);
        qInfo().noquote() << QString("[I][AKASHI][NET-BAN]: an client %1 attempting to connecting when the client are banned by ipids for %2, rejected.").arg(AOClient::calculateIpid(socket->peerAddress()), ban_duration);
    }
    else if (isIPBanned(parseToIPv4(socket->peerAddress()))){ // check if this client are in ban list by [IPs]..
        socket->sendTextMessage(PacketFactory::createPacket("BD", {"Your IP has been banned by a moderator."})->toUtf8());
        socket->close();
        qInfo().noquote() << QString("[I][AKASHI][NET-BAN]: an client %1 attempting to connecting when the client are banned by ip-range, rejected.").arg(AOClient::calculateIpid(parseToIPv4(socket->peerAddress())));
    }
    else{ // otherwise.. client is about to joined..
        NetworkSocket *l_socket = new NetworkSocket(socket, socket);
        QPointer<AOClient> client(m_client_ips.insert(l_socket->peerAddress(), new AOClient(this, l_socket, l_socket, m_available_ids.pop(), music_manager)).value());
        connect(l_socket, &NetworkSocket::clientDisconnected, l_socket, &NetworkSocket::deleteLater);

        if (m_client_ips.count(client->m_remote_ip) > ConfigManager::multiClientLimit() && !client->m_remote_ip.isLoopback()){ // check if this client is reached the multiclient-limter..
            m_client_ips.remove(client->m_remote_ip, client);
            m_available_ids.push(client->clientId());
            l_socket->close(QWebSocketProtocol::CloseCodeNormal);
        }
        else{ // otherwise.. let's registering the client in..
            /* > register the client < */
            m_clients.append(m_clients_ids.insert(client->clientId(), m_client_ipids.insert(client->calculateIpid(), client).value()).value());
            m_player_state_observer.registerClient(m_clients.last());

            /* > connecting the client to signals < */
            connect(l_socket, &NetworkSocket::handlePacket, client, &AOClient::handlePacket);

            /* === [Devs notes] ===
             * This is the infamous workaround for tsuserver4.
             * It should disable fantacrypt completely in any client 2.4.3 or newer
             * ==================== */
            client->sendPacket(PacketFactory::createPacket("decryptor", {"NOENCRYPT"}));
            hookupAOClient(client);
#ifdef NET_DEBUG
            qInfo().noquote() << QString("[I][AKASHI][NET-CLIENT]: %1 connected and registered as ID %2.").arg(client->m_ipid, QString::number(client->clientId()));
#endif
        }
    }
}

void Server::updateCharsTaken(AreaData *area){
    QVector<QString> chars_taken;
    chars_taken.reserve(m_characters.size());
    chars_taken.fill("0", m_characters.size());

    /* heavy loop characters [took] checker */
    auto current_taken = area->charactersTaken();
    current_taken.removeAll(-1);
    for (int index : current_taken){
        if (index >= 0 && index <= chars_taken.size() -1)
            chars_taken[index] = "-1";
    }

    for (int I : area->joinedIDs()){
        auto client = getClientByID(I);
        if (client.isNull())
            continue;

        client->sendPacket("CharsCheck", Server::SetCCTaken(client, chars_taken.toList()));
    }
}
QStringList Server::SetCCTaken(QPointer<AOClient> client, const QStringList &chars_taken){
    if (!client.isNull() && client->isCursed(AOClient::CCURSE)){
        QStringList cursed = QStringList(chars_taken).replaceInStrings("0", "-1");
        for (int I : client->m_charcurse_list){
            if (I >= 0 && I <= cursed.size() -1)
                cursed[I] = "0";
        }
        return cursed;
    }
    return chars_taken;
}

bool Server::isMessageAllowed() const
{
    return m_can_send_ic_messages;
}

bool Server::isLockdownState() const{
    return m_lockdown_mode;
}
bool Server::ClientWhitelisted(const QByteArray &c_hashid){
    return ((m_lockdown_mode && m_lockdown_whitelist.contains(c_hashid)) || !m_lockdown_mode);
}
QVector<QByteArray> Server::Getwhitelistclient(){
    return m_lockdown_whitelist;
}
bool Server::LockdownRegister(const QByteArray &c_hashid, const bool create){
    if (create && c_hashid.size() == 12 && !m_lockdown_whitelist.contains(c_hashid)){
        m_lockdown_whitelist << c_hashid;
        return true;
    }
    else if (!create && c_hashid.size() == 12 && m_lockdown_whitelist.contains(c_hashid)){
        m_lockdown_whitelist.removeAll(c_hashid);
        return true;
    }
    return false;
}
void Server::setlockdownstate(bool state){
    if (m_lockdown_mode != state){
        m_lockdown_mode = state;
        if (!m_lockdown_mode && lockdown_timeout->isActive())
            lockdown_timeout->stop();
    }
}
void Server::startlockdown(const long long time){
    if (time < 1)
        return;
    if (!m_lockdown_mode)
        m_lockdown_mode = true;
    lockdown_timeout->start(time);
}

void Server::startMessageFloodguard(int f_duration)
{
    m_can_send_ic_messages = false;
    m_message_floodguard_timer->start(f_duration);
}

QHostAddress Server::parseToIPv4(QHostAddress f_remote_ip){
    bool l_ok;
    const QHostAddress l_portedIP4(QHostAddress(f_remote_ip).toIPv4Address(&l_ok));
    return l_ok ? l_portedIP4 : f_remote_ip;
}

bool Server::RegisterClienthwid(const int c_index){
    auto client = getClientByID(c_index);
    if (client.isNull())
        return false;

    m_client_hwids.insert(client->getHwid(), client);
    if (m_client_hwids.count(client->getHwid()) > ConfigManager::multiClientLimit() && !client->m_remote_ip.isLoopback()) // check if this client is reached the multiclient-limter..
        return false;
    return true;
}

void Server::reloadSettings(){
    qInfo() << "[AKASHI]: reloading settings..";
    broadcast(PacketCT::CreateMessageS("internal reloading settings.."), AOClient::AuthenticateType::ROOT);
    ConfigManager::reloadSettings();
    ConfigManager::loadCommandHelp();
    emit reloadRequest(ConfigManager::serverName(), ConfigManager::serverDescription());
    emit updateHTTPConfiguration();
    handleDiscordIntegration();
    logger->loadLogtext();
    m_ipban_list = ConfigManager::iprangeBans();
    acl_roles_handler->loadFile("config/acl_roles.ini");
    command_extension_collection->loadFile("config/command_extensions.ini");
    // === Voice ===
    qInfo() << "[AKASHI]: reloading voice parameters..";
    broadcast(PacketCT::CreateMessageS("internal reloading voice parameters.."), AOClient::AuthenticateType::ROOT);
    static const QVariantList VCParams = ConfigManager::GetVoiceParameters();
    broadcast(PacketFactory::createPacket("VS_CAPS", {QString::number(VCParams[ConfigManager::VoiceParameter::ENABLE].toBool()), QString::number(VCParams[ConfigManager::VoiceParameter::PTT].toBool()), VCParams[ConfigManager::VoiceParameter::MAXPEERSAREA].toString(), VCParams[ConfigManager::VoiceParameter::VCODEC].toString(), VCParams[ConfigManager::VoiceParameter::VHZ].toString(), VCParams[ConfigManager::VoiceParameter::VFRAME_MS].toString(), VCParams[ConfigManager::VoiceParameter::MAXBYTES].toString()}));
    // === Data ===
    qInfo() << "[AKASHI]: reloading data..";
    broadcast(PacketCT::CreateMessageS("internal reloading data.."), AOClient::AuthenticateType::ROOT);
    auto GetArea = ConfigManager::sanitizedAreaNames();
    if (m_area_names != GetArea){
        m_area_names = GetArea;
        qInfo() << "[INTERNAL][AKASHI][RELOAD]: reloading areas..";

        Q_EMIT this->ReloadAreas(m_area_names); // update names of area by qstringlist[area-index]..

        while (m_areas.size() != m_area_names.size()){ // <while> the list are not same..
            if (m_areas.size() < m_area_names.size()){ /* > create area < */
                QString area_name(QString::number(m_areas.size()) + ":" + m_area_names[m_areas.size() -1]);
                AreaData *l_area = new AreaData(area_name, m_areas.size(), music_manager);
                m_areas.insert(l_area->index(), l_area);
                connect(l_area, &AreaData::sendAreaPacket, this, QOverload<AOPacket *, int>::of(&Server::broadcast));
                connect(l_area, &AreaData::sendAreaPacketClient, this, &Server::unicast);
                connect(l_area, &AreaData::userJoinedArea, music_manager, &MusicManager::userJoinedArea);
                connect(this, &Server::RemoveDisconnectCA, l_area, &AreaData::RemoveDClient);
                connect(this, &Server::ReloadAreas, l_area, &AreaData::UpdateName);
                music_manager->registerArea(l_area->index());
            }
            else{ /* > remove area < */
                auto area = m_areas.last();
                for (auto existing_client : area->joinedIDs()){
                    auto client = getClientByID(existing_client);
                    if (client.isNull())
                        continue;
                    client->changeArea(0);
                }
                music_manager->unregisterArea(area->index());
                m_areas.removeAll(area);
            }
        }

        broadcast(PacketFactory::createPacket("FA", m_area_names));
        Q_EMIT this->ArupClient();
        broadcast(PacketCT::CreateMessageS("internal fetching the changes areas.."), AOClient::AuthenticateType::ROOT);
        qInfo() << "[INTERNAL][AKASHI][RELOAD]: reloaded areas..";
    }
    music_manager->reloadRequest();
    if (m_music_list != music_manager->rootMusiclist()){
        m_music_list = music_manager->rootMusiclist();
        qInfo() << "[INTERNAL][AKASHI][RELOAD]: reloading musics..";
        for (auto area : m_areas)
            broadcast(PacketFactory::createPacket("FM", music_manager->musiclist(area->index())), area->index()); // based from the [/toggleroot]..
        qInfo() << "[INTERNAL][AKASHI][RELOAD]: reloaded musics..";
        broadcast(PacketCT::CreateMessageS("internal fetching the changes musics.."), AOClient::AuthenticateType::ROOT);
    }
    const QStringList GetChangedCharacters = ConfigManager::characterlist();
    if (m_characters != GetChangedCharacters){
        m_characters = GetChangedCharacters;
        broadcast(PacketCT::CreateMessageS("internal applying the changes characters.."), AOClient::AuthenticateType::ROOT);
        Q_EMIT this->Forcedcloseclients("The server characters now are updated.\nYou can re-joining the server.");
        qInfo() << "[INTERNAL][AKASHI][RELOAD]: reloading characters..";
    }
    else
        broadcast(PacketCT::CreateMessageS("Server reloaded settings & data."), AOClient::AuthenticateType::ROOT);
    qInfo() << "[AKASHI]: reloaded settings & data..";
}

void Server::broadcast(AOPacket *packet, int area_index)
{
    auto GetArea = m_areas.value(area_index);
    if (GetArea.isNull())
        return;

    QVector<int> l_client_ids = GetArea->joinedIDs();
    for (const int l_client_id : qAsConst(l_client_ids)){
        auto client = getClientByID(l_client_id);
        if (client.isNull())
            continue;

        client->sendPacket(packet);
    }
}

void Server::broadcast(AOPacket *packet)
{
    for (auto l_client : qAsConst(m_clients)){
        if (l_client.isNull())
            continue;

        l_client->sendPacket(packet);
    }
}

void Server::broadcast(AOPacket *packet, TARGET_TYPE target)
{
    for (auto l_client : qAsConst(m_clients)){
        if (l_client.isNull())
            continue;

        switch (target) {
        case TARGET_TYPE::MODCHAT:
            if (l_client->checkPermission(ACLRole::MODCHAT))
                l_client->sendPacket(packet);
            break;
        case TARGET_TYPE::ADVERT:
            if (l_client->m_advert_enabled)
                l_client->sendPacket(packet);
            break;
        case TARGET_TYPE::AFKSTATUS:
            if (l_client->m_afk_received)
                l_client->sendPacket(packet);
            break;
        default:
            break;
        }
    }
}

void Server::broadcast(AOPacket *packet, const AOClient::AuthenticateType type){
    for (auto l_client : qAsConst(m_clients)){
        if (l_client.isNull())
            continue;

        switch (type){
        case AOClient::AuthenticateType::NONE:
            if (!l_client->isAuthenticated())
                l_client->sendPacket(packet);
            break;
        case AOClient::AuthenticateType::VIP:
            if (l_client->isVAuthenticated())
                l_client->sendPacket(packet);
            break;
        default:
            if (l_client->isMAuthenticated()) // [ROOT] included..
                l_client->sendPacket(packet);
            break;
        case AOClient::AuthenticateType::ROOT:
            if (l_client->m_authenticated_type == AOClient::AuthenticateType::ROOT)
                l_client->sendPacket(packet);
            break;
        }
    }
}

void Server::broadcast(AOPacket *packet, const AOClient::AuthenticateType type, const int area_index){
    const auto target_area = getAreaById(area_index);
    if (target_area.isNull())
        return;

    for (const int c_index : target_area->joinedIDs()){
        auto l_client = getClientByID(c_index);
        if (l_client.isNull())
            continue;

        switch (type){
        case AOClient::AuthenticateType::NONE:
            if (!l_client->isAuthenticated())
                l_client->sendPacket(packet);
            break;
        case AOClient::AuthenticateType::VIP:
            if (l_client->isVAuthenticated())
                l_client->sendPacket(packet);
            break;
        default:
            if (l_client->isMAuthenticated()) // [ROOT] included..
                l_client->sendPacket(packet);
            break;
        case AOClient::AuthenticateType::ROOT:
            if (l_client->m_authenticated_type == AOClient::AuthenticateType::ROOT)
                l_client->sendPacket(packet);
            break;
        }
    }
}

void Server::broadcast(AOPacket *packet, int area_index, TARGET_TYPE target)
{
    auto GetArea = m_areas.value(area_index);
    if (GetArea.isNull())
        return;

    QVector<int> l_client_ids = GetArea->joinedIDs();

    for (const int l_client_id : std::as_const(l_client_ids)){
        auto l_client = getClientByID(l_client_id);
        if (l_client.isNull())
            continue;

        switch (target) {
        case TARGET_TYPE::MODCHAT:
            if (l_client->checkPermission(ACLRole::MODCHAT))
                l_client->sendPacket(packet);
            break;
        case TARGET_TYPE::ADVERT:
            if (l_client->m_advert_enabled)
                l_client->sendPacket(packet);
            break;
        case TARGET_TYPE::AFKSTATUS:
            if (l_client->m_afk_received)
                l_client->sendPacket(packet);
            break;
        default:
            break;
        }
    }
}

void Server::broadcast(AOPacket *packet, AOPacket *other_packet, TARGET_TYPE target)
{
    switch (target) {
    case TARGET_TYPE::AUTHENTICATED:
        for (AOClient *l_client : qAsConst(m_clients)){
            if (QPointer<AOClient>(l_client).isNull())
                continue;

            if (l_client->isMAuthenticated())
                l_client->sendPacket(other_packet);
            else
                l_client->sendPacket(packet);
        }
        break;
    default:
        // Unimplemented, so not handled.
        break;
    }
}
void Server::broadcast(AOPacket *packet, AOPacket *other_packet, AOClient::ClientVersion::ClientType t_target){
    for (AOClient *l_client : qAsConst(m_clients)){
        if (QPointer<AOClient>(l_client).isNull())
            continue;
        l_client->sendPacket(l_client->m_version.type == t_target ? other_packet : packet);
    }
}
void Server::broadcast(AOPacket *packet, AOPacket *other_packet, AOClient::ClientVersion::ClientType t_target, int area_index){
    auto GetArea = m_areas.value(area_index);
    if (GetArea.isNull())
        return;

    for (int CIndex : GetArea->joinedIDs()){
        auto l_client = getClientByID(CIndex);
        if (l_client.isNull())
            continue;
        l_client->sendPacket(l_client->m_version.type == t_target ? other_packet : packet);
    }

}
void Server::unicast(AOPacket *f_packet, int f_client_id){
    auto l_client = getClientByID(f_client_id);
    if (l_client.isNull()) /* This should never happen, but safety first. */
        return;
    l_client->sendPacket(f_packet);
}

QPointer<AOClient> Server::getClient(QString ipid){
    const QList<QPointer<AOClient>> list = getClientsByIpid(ipid);
    if (list.isEmpty())
        return QPointer<AOClient>();
    return list[0];
}

QList<QPointer<AOClient>> Server::getClientsByIpid(QString ipid){
    return m_client_ipids.values(ipid);
}

QList<QPointer<AOClient>> Server::getClientsByHwid(QString f_hwid){
    return m_client_hwids.values(f_hwid);
}

QPointer<AOClient> Server::getClientByID(int id)
{
    return m_clients_ids.value(id);
}

int Server::getPlayerCount()
{
    return m_player_count;
}

QStringList Server::getCharacters()
{
    return m_characters;
}

int Server::getCharacterCount()
{
    return m_characters.length();
}

QString Server::getCharacterById(int f_chr_id){
    return f_chr_id >= 0 && f_chr_id < m_characters.size() -1 ? m_characters[f_chr_id] : QString();
}

int Server::getCharID(QString char_name)
{
    for (int i = 0; i < m_characters.length(); i++){
        if (m_characters[i].compare(char_name, Qt::CaseInsensitive) == 0)
            return i;
    }

    return -1; // character does not exist
}

QVector<QPointer<AreaData>> Server::getAreas()
{
    return m_areas;
}

int Server::getAreaCount()
{
    return m_areas.length();
}

QPointer<AreaData> Server::getAreaById(int f_area_id){
    return m_areas.value(f_area_id, nullptr);
}

QQueue<QString> Server::getAreaBuffer(const QString &f_areaName)
{
    return logger->buffer(f_areaName);
}

QStringList Server::getAreaNames()
{
    return m_area_names;
}

QString Server::getAreaName(int f_area_id){
    return m_area_names.value(f_area_id, QString());
}

QStringList Server::getMusicList()
{
    return m_music_list;
}

QStringList Server::getBackgrounds()
{
    return m_backgrounds;
}

DBManager *Server::getDatabaseManager()
{
    return db_manager;
}

MedievalParser *Server::getMedievalParser()
{
    return medieval_parser;
}

ACLRolesHandler *Server::getACLRolesHandler()
{
    return acl_roles_handler;
}

CommandExtensionCollection *Server::getCommandExtensionCollection()
{
    return command_extension_collection;
}

void Server::allowMessage()
{
    m_can_send_ic_messages = true;
}

void Server::handleDiscordIntegration()
{
    // Prevent double connecting by preemtively disconnecting them.
    disconnect(this, nullptr, discord, nullptr);

    if (ConfigManager::discordWebhookEnabled()) {
        if (ConfigManager::discordModcallWebhookEnabled())
            connect(this, &Server::modcallWebhookRequest, discord, &Discord::onModcallWebhookRequested);

        if (ConfigManager::discordBanWebhookEnabled()){
            connect(this, &Server::banWebhookRequest, discord, &Discord::onBanWebhookRequested);
            connect(this, &Server::UnbanWebhookRequested, discord, &Discord::onUnbanWebhookRequested);
        }
    }
    return;
}

void Server::markIDFree(AOClient *f_client){
    if (f_client->m_joined)
        decreasePlayerCount();
    m_clients.removeAll(f_client);
    m_player_state_observer.unregisterClient(f_client);
    /* remove current client from all of QMultiHash(s) */
    m_client_ips.remove(f_client->m_remote_ip, f_client);
    m_client_ipids.remove(f_client->m_ipid, f_client);
    if (!f_client->m_hwid.isEmpty())
        m_client_hwids.remove(f_client->m_hwid, f_client);
    /* > freed ids < */
    m_available_ids.push(m_clients_ids.insert(f_client->clientId(), nullptr).key());
    f_client->deleteLater();
}

void Server::hookupAOClient(AOClient *client){
    /* > connection event < */
    connect(client, &AOClient::joined, this, &Server::increasePlayerCount);
    connect(this, &Server::Forcedcloseclients, client, &AOClient::ForcedDisconnected);
    /* > Logger < */
    connect(client, &AOClient::logIC, logger, &ULogger::logIC);
    connect(client, &AOClient::logOOC, logger, &ULogger::logOOC);
    connect(client, &AOClient::logMusic, logger, &ULogger::logMusic);
    connect(client, &AOClient::logLogin, logger, &ULogger::logLogin);
    connect(client, &AOClient::logCMD, logger, &ULogger::logCMD);
    connect(client, &AOClient::logBan, logger, &ULogger::logBan);
    connect(client, &AOClient::logKick, logger, &ULogger::logKick);
    connect(client, &AOClient::logModcall, logger, &ULogger::logModcall);
    /* > client event < */
    connect(client, &AOClient::clientSuccessfullyDisconnected, this, &Server::markIDFree);
    connect(this, &Server::ArupClient, client, &AOClient::fullArup);
    /* > broadcast packet signals < */
    connect(this, QOverload<AOPacket *, const AOClient::AuthenticateType>::of(&Server::broadcastCAuth), client, QOverload<AOPacket *, const AOClient::AuthenticateType>::of(&AOClient::sendPacket));
    connect(this, QOverload<AOPacket *, const AOClient::AuthenticateType, const int>::of(&Server::broadcastCAuth), client, QOverload<AOPacket *, const AOClient::AuthenticateType, const int>::of(&AOClient::sendPacket));
    /* > broadcast (voice) packet signals < */
    connect(this, &Server::broadcastVFrame, client, &AOClient::sendAudioFrame);
    connect(this, &Server::broadcastVState, client, &AOClient::sendAudioState);
    connect(this, &Server::broadcastVJoinLeave, client, &AOClient::sendAudioJoinLeave);
}

void Server::increasePlayerCount(){
    Q_EMIT playerCountUpdated(m_player_count++);
}

void Server::decreasePlayerCount(){
    Q_EMIT playerCountUpdated(m_player_count--);
}

bool Server::isIPBanned(QHostAddress f_remote_IP)
{
    bool l_match_found = false;
    for (const QString &l_ipban : qAsConst(m_ipban_list)) {
        if (f_remote_IP.isInSubnet(QHostAddress::parseSubnet(l_ipban))) {
            l_match_found = true;
            break;
        }
    }
    return l_match_found;
}

Server::~Server(){
    server->deleteLater();
    discord->deleteLater();
    acl_roles_handler->deleteLater();

    delete db_manager;
}
