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
#ifndef SERVER_H
#define SERVER_H

#include <QCoreApplication>
#include <QDebug>
#include <QFile>
#include <QMap>
#include <QSettings>
#include <QStack>
#include <QString>
#include <QTimer>
#include <QWebSocket>
#include <QWebSocketServer>
#include <QPointer> /* smart pointer (qobject) guards */

#include "medieval_parser.h"
#include "network/aopacket.h"
#include "playerstateobserver.h"

class ACLRolesHandler;
class ServerPublisher;
class AOClient;
class AreaData;
class CommandExtensionCollection;
class ConfigManager;
class DBManager;
class Discord;
class MusicManager;
class ULogger;

/**
 * @brief The class that represents the actual server as it is.
 */
class Server : public QObject
{
    Q_OBJECT

  public:
    /**
     * @brief Creates a Server instance.
     *
     * @param p_ws_port The port to listen for connections on.
     * @param parent Qt-based parent, passed along to inherited constructor from QObject.
     */
    Server(int p_ws_port, QObject *parent = nullptr);

    /**
     * @brief Destructor for the Server class.
     *
     * @details Marks every Client, the WSProxy, the underlying #server, and the database manager to be deleted later.
     */
    ~Server();

    /**
     * @brief Starts the server.
     *
     * @details Starts listening for incoming connections on the given port.
     *
     * Advertising is not done here -- see Advertiser::contactMasterServer() for that.
     */
    void start();

    /**
     * @brief Enum to specifc different targets to send altered packets to a specific usergroup.
     */
    enum class TARGET_TYPE
    {
        AUTHENTICATED,
        MODCHAT,
        ADVERT,
        AFKSTATUS
    };
    Q_ENUM(TARGET_TYPE)

    /**
     * @brief Returns a list of all clients currently in the server.
     *
     * @return A list of all clients currently in the server.
     */
    QVector<QPointer<AOClient>> getClients();

    /**
     * @brief Gets a pointer to a client by IPID.
     *
     * @param ipid The IPID to look for.
     *
     * @return A pointer to the client if found, a nullpointer if not.
     *
     * @see Server::getClientsByIpid() to get all clients ran by the same user.
     */
    QPointer<AOClient> getClient(QString ipid);

    /**
     * @brief Gets a list of pointers to all clients with the given IPID.
     *
     * @param ipid The IPID to look for.
     *
     * @return A list of clients whose IPID match. List may be empty.
     */
    QList<QPointer<AOClient>> getClientsByIpid(QString ipid);

    /**
     * @brief Gets a list of pointers to all clients with the given HWID.
     *
     * @param HWID The HWID to look for.
     *
     * @return A list of clients whose HWID match. List may be empty.
     */
    QList<QPointer<AOClient>> getClientsByHwid(QString f_hwid);

    /**
     * @brief Gets a pointer to a client by user ID.
     *
     * @param id The user ID to look for.
     *
     * @return A pointer to the client if found, a nullpointer if not.
     */
    QPointer<AOClient> getClientByID(int id);

    /**
     * @brief Returns the overall player count in the server.
     *
     * @return The overall player count in the server.
     */
    int getPlayerCount();

    /**
     * @brief Returns a list of the available characters on the server to use.
     *
     * @return A list of the available characters on the server to use.
     */
    QStringList getCharacters();

    /**
     * @brief Returns the count of available characters on the server to use.
     *
     * @return The count of available characters on the server to use.
     */
    int getCharacterCount();

    /**
     * @brief Get the available character by index.
     *
     * @param f_chr_id The index of the character.
     *
     * @return The character if it exist, otherwise an empty stirng.
     */
    QString getCharacterById(int f_chr_id);

    /**
     * @brief Updates which characters are taken in the given area, and sends out an update packet to
     * all clients present the area.
     *
     * @param area The area in which to update the list of characters.
     */
    void updateCharsTaken(AreaData *area);

    /* > === broadcast Packet === < */
    /**
     * @brief Sends a packet to all clients in a given area.
     *
     * @param packet The packet to send to the clients.
     *
     * @param area_index The index of the area to look for clients in.
     *
     * @note Does nothing if an area by the given index does not exist.
     */
    void broadcast(AOPacket *packet, int area_index);
    /**
     * @brief Sends a packet to all clients in the server.
     *
     * @param packet The packet to send to the clients.
     */
    void broadcast(AOPacket *packet);
    /**
     * @brief Sends a packet to a specific usergroup..
     *
     * @param The packet to send to the clients.
     *
     * @param ENUM to determine the targets of the altered packet.
     */
    void broadcast(AOPacket *packet, TARGET_TYPE target);
    /**
     * @brief Sends a packet to a specific usergroup..
     *
     * @param The packet to send to the clients.
     *
     * @param ENUM to determine the type of the altered authenticated packet.
     */
    void broadcast(AOPacket *packet, const AOClient::AuthenticateType type);
    /**
     * @brief Sends a packet to a specific usergroup..
     *
     * @param The packet to send to the clients.
     *
     * @param ENUM to determine the type of the altered authenticated packet.
     *
     * @param area_index The index of the area to look for clients in.
     */
    void broadcast(AOPacket *packet, const AOClient::AuthenticateType type, const int area_index);
    /**
     * @brief Sends a packet to a specific usergroup in a given area.
     *
     * @param The packet to send to the clients.
     *
     * @param area_index the area to send the packet to.
     *
     * @param ENUM to determine the targets of the altered packet.
     */
    void broadcast(AOPacket *packet, int area_index, TARGET_TYPE target);
    /**
     * @brief Sends a packet to clients, sends an altered packet to a specific usergroup.
     *
     * @param The packet to send to the clients.
     *
     * @param The altered packet to send to the other clients.
     *
     * @param ENUM to determine the targets of the altered packet.
     */
    void broadcast(AOPacket *packet, AOPacket *other_packet, enum TARGET_TYPE target);
    /**
      * @brief Same like Server::broadcast(AOPacket, AOPacket) but with client type.
      *
      * @param the packet to send at the clients (not target client type).
      *
      * @param the altered packet to send to the target client type.
      *
      * @param the target of client type.
      */
    void broadcast(AOPacket *packet, AOPacket *other_packet, AOClient::ClientVersion::ClientType t_target);
    /**
      * @brief Same like Server::broadcast(AOPacket, int area) and same like Server::broadcast(AOPacket, AOPacket, AOClient::ClientVersion::ClientType).
      *
      * @param same param as above of this functions.
      *
      * @param same param as above of this functions.
      *
      * @param same param as above of this functions.
      *
      * @param the area to send the packet to.
      */
    void broadcast(AOPacket *packet, AOPacket *other_packet, AOClient::ClientVersion::ClientType t_target, int area_index);
    /* > ================== < */

    /**
     * @brief Sends a packet to a single client.
     *
     * @param The packet send to the client.
     *
     * @param The temporary userID of the client.
     */
    void unicast(AOPacket *f_packet, int f_client_id);

    /**
     * @brief Returns the character's character ID (= their index in the character list).
     *
     * @param char_name The 'internal' name for the character whose character ID to look up. This is equivalent to
     * the name of the directory of the character.
     *
     * @return The character ID if a character with that name exists in the character selection list, `-1` if not.
     */
    int getCharID(QString char_name);

    /**
     * @brief Checks if an IP is in a subnet of the IPBanlist.
     **/
    bool isIPBanned(QHostAddress f_remote_IP);

    /**
     * @brief Returns the list of areas in the server.
     *
     * @return A list of areas.
     */
    QVector<QPointer<AreaData>> getAreas();

    /**
     * @brief Returns the number of areas in the server.
     */
    int getAreaCount();

    /**
     * @brief Returns a pointer to the area associated with the index.
     *
     * @param f_area_id The index of the area.
     *
     * @return A pointer to the area or null.
     */
    QPointer<AreaData> getAreaById(int f_area_id);

    /**
     * @brief Getter for an area specific buffer from the logger.
     */
    QQueue<QString> getAreaBuffer(const QString &f_areaName);

    /**
     * @brief The names of the areas on the server.
     *
     * @return A list of names.
     */
    QStringList getAreaNames();

    /**
     * @brief Returns the name of the area associated with the index.
     *
     * @param f_area_id The index of the area.
     *
     * @return The name of the area or empty.
     */
    QString getAreaName(int f_area_id);

    /**
     * @brief Returns the available songs on the server.
     *
     * @return A list of songs.
     */
    QStringList getMusicList();

    /**
     * @brief Returns the available backgrounds on the server.
     *
     * @return A list of backgrounds.
     */
    QStringList getBackgrounds();

    /**
     * @brief Returns a pointer to a database manager.
     *
     * @return A pointer to a database manager.
     */
    DBManager *getDatabaseManager();

    /**
     * @brief Returns a pointer to the server's Ye Olde Chat Filter
     */
    MedievalParser *getMedievalParser();

    /**
     * @brief Returns a pointer to ACL role handler.
     */
    ACLRolesHandler *getACLRolesHandler();

    /**
     * @brief Returns a pointer to a command extension collection.
     */
    CommandExtensionCollection *getCommandExtensionCollection();

    /**
     * @brief The server-wide global timer.
     */
    QTimer *timer;
    /**
     * @brief the server lockdown timeout.
     *
     * @details this timer been using if it been set from AOClient::CalendarParse, otherwise nothing.
     */
    QTimer *lockdown_timeout;

    /**
     * @brief Set cursed-characters taken.
     * @param target client.
     * @param the list of character taken.
     *
     * @return the list of characters between cursed list or normal list.
     */
    static QStringList SetCCTaken(QPointer<AOClient> client, const QStringList &chars_taken =  QStringList());

    /**
     * @brief Returns whatever a game message may be broadcasted or not.
     *
     * @return True if expired; false otherwise.
     */
    bool isMessageAllowed() const;

    // > === [lockdown system] === <
    /**
     * @brief Get server current lockdown state.
     */
    bool isLockdownState() const;
    /**
     * @brief Check if the client is whitelisted while lockdown stated.
     * @param hashed 12 length (sha256).
     * @return true if server are not lockdown or if client is whitelisted, false otherwise.
     */
    bool ClientWhitelisted(const QByteArray &c_hashid);
    /**
     * @brief Get the list of whitelisted client (lockdown version).
     */
    QVector<QByteArray> Getwhitelistclient();
    /**
     * @brief the (un)registering the client <hashid>.
     * @param hashed 12 length (sha256).
     * @param true if register, otherwise unregister.
     * @return true if it are (un)registered, false otherwise.
     */
    bool LockdownRegister(const QByteArray &c_hashid, const bool create);
    /**
     * @brief Set server lockdown state.
     */
    void setlockdownstate(bool state = false);
    /**
     * @brief Set/start server lockdown with time.
     * @param the most not below 1 and it must millsec, otherwise it does nothing.
     */
    void startlockdown(const long long time);
    // > ======================== <

    /**
     * @brief Starts a global timer that determines whatever a game message may be broadcasted or not.
     *
     * @param f_duration The duration of the message floodguard timer.
     */
    void startMessageFloodguard(int f_duration);

    /**
     * @brief Attempts to parse a IPv6 mapped IPv4 to an IPv4.
     */
    QHostAddress parseToIPv4(QHostAddress f_remote_ip);

    bool RegisterClienthwid(const int c_index);

  public slots:
    /**
     * @brief Convenience class to call a reload of available configuraiton elements.
     */
    void reloadSettings();

    /**
     * @brief Handles a new connection.
     *
     * @details The function creates an AOClient to represent the user, assigns a user ID to them, and
     * checks if the client is banned.
     */
    void clientConnected();

    /**
     * @brief Method to construct and reconstruct Discord Webhook Integration.
     *
     * @details Constructs or rebuilds Discord Object during server startup and configuration reload.
     */
    void handleDiscordIntegration();

    /**
     * @brief Marks a userID as free and ads it back to the available client id queue.
     */
    void markIDFree(AOClient *f_client);

  signals:

    /**
      * @brief sends the all area to remove an disconnect client from areadata::owner.
      */
    void RemoveDisconnectCA(const int id);

    /**
     * @brief Sends the server name and description, emitted by /reload.
     *
     * @param p_name The server name.
     * @param p_desc The server description.
     */
    void reloadRequest(QString p_name, QString p_desc);

    /**
     * @brief Tell areas to change area name based their index if doesn't same from the list.
     * @param The list of area names.
     */
    void ReloadAreas(const QStringList &list);

    /**
     * @brief Tell clients to send all type of arups.
     */
    void ArupClient();

    /**
     * @brief Triggers the m_socket of the all client to closed.
     * @param The reason of close via [KK] packet if persents.
     */
    void Forcedcloseclients(const QString &reason = QString());
    /**
     * @brief This signal is emitted whenever the current player count has changed.
     *
     * @param f_current_player The player count at the time the signal was emitted.
     */
    void playerCountUpdated(int f_current_players);

    /**
     * @brief Triggers a partial update of the modern advertiser as some information, such as ports
     * can't be updated while the server is running.
     */
    void updateHTTPConfiguration();

    /**
     * @brief Sends a modcall webhook request, emitted by AOClient::pktModcall.
     *
     * @param f_name The character and OOC name & area of the client who sent the modcall.
     * @param r_name The name & area of the [regarding] who been reported from the caller.
     * @param f_reason The reason the client specified for the modcall.
     * @param f_buffer The area's log buffer.
     */
    void modcallWebhookRequest(const QPair<QString, QString> &f_name, const QPair<QString, QString> &r_name, const QString &f_reason, const QQueue<QString> &f_buffer);

    /**
     * @brief Sends a ban webhook request, emitted by AOClient::cmdBan
     * @param f_ipid The IPID of the banned client.
     * @param f_moderator The moderator who issued the ban.
     * @param f_duration The duration of the ban in a human readable format.
     * @param f_reason The reason for the ban.
     * @param f_banID The ID of the issued ban.
     */
    void banWebhookRequest(const QString &f_ipid, const QPair<int, QString> &f_moderator, const QString &f_duration, const QString &f_reason, const int &f_banID, const int &f_count);

    void UnbanWebhookRequested(const QString &f_ipid, const QPair<QPair<int, QString>, QPair<int, QString>> &f_moderator, const int &f_banID, const int &f_ban_duration, const QDateTime &f_date, const QStringList &f_reason);

    /**
     * @brief Signal connected to universal logger. Logs a client connection attempt.
     * @param f_ip_address The IP Address of the incoming connection.
     * @param f_ipid The IPID of the incoming connection.
     * @param f_hdid The HDID of the incoming connection.
     */
    void logConnectionAttempt(const QString &f_ip_address, const QString &f_ipid, const QString &f_hwid);

    /* > broadcast to client(s) via signal(s) < */
    /**
     * @brief A signals of <vs_frame> for sending an <vs_audio#<user_id>#<b64_frame>> packet to clients.
     * @param The client_id from.
     * @param The frame of byte(s).
     * @param Target area.
     */
    void broadcastVFrame(const int c_from, const QByteArray &frame_byte, const int area_id);
    /**
     * @brief A signals of user <vs_speak> toggle for sending an <vs_speak#<user_id>#<toggle>> packet to clients.
     * @param The client_id from.
     * @param toggle of speak(s).
     * @param Target area.
     */
    void broadcastVState(const int c_from, const bool toggle, const int area_id);
    /**
     * @brief A signals of <vs_join> nor <vs_leave> for sending an <vs_join#<user_id>> nor <vs_leave#<user_id>> packet to clients.
     * @param The client_id from.
     * @param The boolean of if client are leave or not.
     * @param Target area.
     */
    void broadcastVJoinLeave(const int c_from, const bool isleave, const int area_id);

    /* > broadcase to client via signal(s) (AOClient::AuthenticateType version) < */
    void broadcastCAuth(AOPacket *packet, const AOClient::AuthenticateType AuthType);
    void broadcastCAuth(AOPacket *packet, const AOClient::AuthenticateType AuthType, const int areaID);

  private:
    /**
     * @brief Listens for incoming websocket connections.
     */
    QWebSocketServer *server;

    /**
     * @brief Handles Discord webhooks.
     */
    Discord *discord;

    /**
     * @brief Handles HTTP server advertising.
     */
    ServerPublisher *server_publisher;

    /**
     * @brief Handles the universal log framework.
     */
    ULogger *logger;

    /**
     * @brief Handles all musiclists.
     */
    MusicManager *music_manager;

    /**
     * @brief The port through which the server will accept WebSocket connections.
     */
    int m_port;

    /**
     * @brief The collection of all currently connected clients.
     */
    QVector<QPointer<AOClient>> m_clients;

    /**
     * @brief Collection of all clients with their userID as key.
     */
    QHash<int, QPointer<AOClient>> m_clients_ids;
    /**
     * @brief Collection of all clients with their userIP as key (only for (dis)connect)
     */
    QMultiHash<QHostAddress, QPointer<AOClient>> m_client_ips;
    /**
     * @brief Collection of all clients with their userIPID as key.
     */
    QMultiHash<const QString, QPointer<AOClient>> m_client_ipids;
    /**
     * @brief Collection of all clients with their userHWID as key.
     */
    QMultiHash<const QString, QPointer<AOClient>> m_client_hwids;
    /**
     * @brief The playerlist state observer.
     */
    PlayerStateObserver m_player_state_observer;

    /**
     * @brief Stack of all available IDs for clients. When this is empty the server
     * rejects any new connection attempt.
     */
    QStack<int> m_available_ids;

    /**
     * @brief The overall player count in the server.
     */
    int m_player_count;

    /**
     * @brief The characters available on the server to use.
     */
    QStringList m_characters;


    /**
     * @brief The areas on the server.
     */
    QVector<QPointer<AreaData>> m_areas;

    /**
     * @brief The names of the areas on the server.
     *
     * @details Equivalent to iterating over #areas and getting the area names individually, but grouped together
     * here for faster access.
     */
    QStringList m_area_names;

    /**
     * @brief The available songs on the server.
     *
     * @details Does **not** include the area names, the actual music list packet should be constructed from
     * #area_names and this combined.
     */
    QStringList m_music_list;

    /**
     * @brief The backgrounds on the server that may be used in areas.
     */
    QStringList m_backgrounds;

    /**
     * @brief Collection of all IPs that are banned.
     */
    QStringList m_ipban_list;

    /**
     * @brief Timer until the next IC message can be sent.
     */
    QTimer *m_message_floodguard_timer;

    /**
     * @brief If false, IC messages will be rejected.
     */
    bool m_can_send_ic_messages = true;

    /**
     * @brief The server lockdown state.
     */
    bool m_lockdown_mode = false;

    /**
     * @brief The server whitelist of client while lockdown state.
     *
     * @details This were the client's hashid been stored until server restart/off.
     */
    QVector<QByteArray> m_lockdown_whitelist;
    /**
     * @brief The database manager on the server, used to store users' bans and authorisation details.
     */
    DBManager *db_manager;

    /**
     * @brief Medieval mode text parser class
     */
    MedievalParser *medieval_parser;

    /**
     * @see ACLRolesHandler
     */
    ACLRolesHandler *acl_roles_handler;

    /**
     * @see CommandExtensionCollection
     */
    CommandExtensionCollection *command_extension_collection;

    /**
     * @brief Connects new AOClient to logger and disconnect handling.
     **/
    void hookupAOClient(AOClient *client);

  private slots:
    /**
     * @brief Increase the current player count by one.
     */
    void increasePlayerCount();

    /**
     * @brief Decrease the current player count based on the client id provided.
     *
     * @param f_client_id The client id of the client to check.
     */
    void decreasePlayerCount();

    /**
     * @brief Allow game messages to be broadcasted.
     */
    void allowMessage();
};

#endif // SERVER_H
