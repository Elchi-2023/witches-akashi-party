#include "packet/packet_ct.h"

#include "config_manager.h"
#include "packet/packet_factory.h"
#include "server.h"

#include <QDebug>
#include <QRegularExpression>

PacketCT::PacketCT(QStringList &contents) :
    AOPacket(contents)
{
}

PacketInfo PacketCT::getPacketInfo() const
{
    PacketInfo info{
        .acl_permission = ACLRole::Permission::NONE,
        .min_args = 2,
        .header = "CT"};
    return info;
}

static QString NormalizeName(QString s){ /* > normalize of [zero-width & any invisible unicode] < */
    static const ushort BadChars[] = {
        0x200B, 0x200C, 0x200D, 0x2060, 0xFEFF, 0x180E,
        0x200E, 0x200F,
        0x202A, 0x202B, 0x202C, 0x202D, 0x202E,
        0x2066, 0x2067, 0x2068, 0x2069, 0x2064
    }; // thanks to google(s).. i guess..

    for (ushort u : BadChars)
        s.remove(QChar(u));

    return s.trimmed().remove(QRegularExpression("\\[|\\]|\\{|\\}|\\#|\\$|\\%|\\&"));
}

void PacketCT::handlePacket(AreaData *area, AOClient &client) const{
    if (QPointer<AreaData>(area).isNull())
        return; // safely first..

    const QString Name = NormalizeName(AOClient::dezalgo(m_content[0]));
    if (Name.isEmpty() || Name.normalized(QString::NormalizationForm_KC).compare(ConfigManager::serverName().normalized(QString::NormalizationForm_KC), Qt::CaseInsensitive) == 0) /* impersonation & empty name protection */
        return;
    else if (Name.length() >= 31)
        client.sendServerMessage("Your name is too long! Please limit it to under 30 characters.");
    else{
        client.setName(Name);
        QString l_message = AOClient::dezalgo(m_content[1]);
        if (l_message.isEmpty())
            return;
        else if (client.m_is_logging_in){
            if (client.isAccessBlocked(AOClient::BlockType::OOC)){
                client.sendServerMessage("You are OOC muted, and cannot speak.\nExiting login prompt.", "[Login Prompt]");
                client.m_is_logging_in = false;
            }
            else if (l_message.size() <= ConfigManager::maxCharacters()){
                if (m_content[1].toLower() == "/cancel"){
                    client.m_is_logging_in = false;
                    client.sendServerMessage("Exiting login prompt.", "[Login Prompt]");
                    client.totalAttempt = qMakePair(0, 0); /* reset the counts */
                }
                else{
                    client.m_is_logging_in = !client.loginAttempt(l_message);
                    if (client.m_is_logging_in){
                        ++client.totalAttempt.first;
                        if (client.totalAttempt.first > 3){
                            ++client.totalAttempt.second;
                            const QPointer<Server> current_server = client.getServer();
                            if (!current_server.isNull())
                                current_server->broadcast(PacketCT::CreateMessageS(QString("[ALERT] A user %1 (aka %2) attempted to logining, %3 tries.").arg(client.m_ipid, client.name(), QString::number(client.totalAttempt.second))), AOClient::AuthenticateType::MODERATOR);
                            qInfo() << "[I][AKASHI][Login Prompt]: " << client.m_ipid << " (aka " << client.name() << ") attempting to logining, " << client.totalAttempt.second << " tries.";
                            client.totalAttempt.first = 0;
                        }
                        client.sendServerMessage("Please try again or /cancel to exit", "[Login Prompt]");
                    }
                    else
                        client.totalAttempt = qMakePair(0, 0); /* reset the counts */
                }
            }
            else
                client.sendServerBroadcast(QString("Your messages is too long! Please limit it to under %1 characters.").arg(QString::number(ConfigManager::maxCharacters())));
        }
        else{
            if (l_message.length() <= ConfigManager::maxCharacters()){
                auto current_rate = client.GetRateTick("CT");
                if (client.isAccessBlocked(AOClient::BlockType::OOC)){
                    if (client.m_authenticated_type == AOClient::AuthenticateType::ROOT){ // [ROOT] bypass moment..
                        if (current_rate.restart() > 15){
                            if (l_message.at(0) == '/'){ // it can using commands..
                                QPair<QString, QStringList> l_commands = {QString(), l_message.split(' ', Qt::SkipEmptyParts)};
                                l_commands.first = l_commands.second.takeFirst().toLower().remove(0, 1); // you might wondering about the "remove(0, 1)"?... cause 'we' needs remove the "/" from first list..
                                client.handleCommand(l_commands);
                                if (l_commands.first != "pm") // privacy matter.., i mean.. 'we' supposen't seeing the "pm" people.. are we?..
                                    emit client.logCMD((client.character() + " " + client.characterName()), client.m_ipid, client.name(), l_commands.first, l_commands.second, area->name());
                            }
                            else if (client.isCursed(AOClient::CurseType::FULL)){ // but.. [ROOT] cannot avoiding the fully curses..
                                l_message = AOClient::MessageToGimped(l_message);
                                l_message = AOClient::MessageToMediveal(l_message);
                                l_message = AOClient::MessageShaked(l_message);
                                l_message = AOClient::MessageToUwU(l_message);
                                l_message = AOClient::MessageToPigify(l_message);
                                l_message = AOClient::MessageDisemvowel(l_message);
                                client.getServer()->broadcast(PacketCT::CreateMessage(l_message, client.name()), client.areaId());
                            }
                            else{ // even some of curses..
                                if (client.isCursed(AOClient::CurseType::GIMP))
                                    l_message = AOClient::MessageToGimped(l_message);
                                if (client.isCursed(AOClient::CurseType::MEDIEVAL) || area->isMedievalMode())
                                    l_message = AOClient::MessageToMediveal(l_message);
                                if (client.isCursed(AOClient::CurseType::SHAKE))
                                    l_message = AOClient::MessageShaked(l_message);
                                if (client.isCursed(AOClient::CurseType::UWUIFY))
                                    l_message = client.MessageToUwU(l_message);
                                if (client.isCursed(AOClient::CurseType::PIGIFY))
                                    l_message = AOClient::MessageToPigify(l_message);
                                if (client.isCursed(AOClient::CurseType::DISEMVOWEL))
                                    l_message = AOClient::MessageDisemvowel(l_message);
                                client.getServer()->broadcast(PacketCT::CreateMessage(l_message, client.name()), client.areaId());
                            }
                        }
                    }
                    else // non-[root]..
                        client.sendServerMessage("You are OOC muted, and cannot speak.");
                }
                else if (current_rate.restart() >= 20){
                    if (l_message.at(0) == '/'){
                        QPair<QString, QStringList> l_commands = {QString(), l_message.split(' ', Qt::SkipEmptyParts)};
                        l_commands.first = l_commands.second.takeFirst().toLower().remove(0, 1);
                        client.handleCommand(l_commands);
                        if (l_commands.first != "pm") // privacy matter.., i mean.. 'we' supposen't seeing the "pm" people.. are we?..
                            emit client.logCMD((client.character() + " " + client.characterName()), client.m_ipid, client.name(), l_commands.first, l_commands.second, area->name());
                    }
                    else{
                        if (client.isCursed(AOClient::CurseType::FULL)){
                            l_message = AOClient::MessageToGimped(l_message);
                            l_message = AOClient::MessageToMediveal(l_message);
                            l_message = AOClient::MessageShaked(l_message);
                            l_message = AOClient::MessageToUwU(l_message);
                            l_message = AOClient::MessageToPigify(l_message);
                            l_message = AOClient::MessageDisemvowel(l_message);
                        }
                        else{
                            if (client.isCursed(AOClient::CurseType::GIMP))
                                l_message = AOClient::MessageToGimped(l_message);
                            if (client.isCursed(AOClient::CurseType::MEDIEVAL) || area->isMedievalMode())
                                l_message = AOClient::MessageToMediveal(l_message);
                            if (client.isCursed(AOClient::CurseType::SHAKE))
                                l_message = AOClient::MessageShaked(l_message);
                            if (client.isCursed(AOClient::CurseType::UWUIFY))
                                l_message = AOClient::MessageToUwU(l_message);
                            if (client.isCursed(AOClient::CurseType::PIGIFY))
                                l_message = AOClient::MessageToPigify(l_message);
                            if (client.isCursed(AOClient::CurseType::DISEMVOWEL))
                                l_message = AOClient::MessageDisemvowel(l_message);
                        }
                        client.getServer()->broadcast(PacketCT::CreateMessage(l_message, client.name()), client.areaId());
                        Q_EMIT client.logOOC((client.character() + " " + client.characterName()), client.name(), {client.clientId(), client.m_ipid}, area->name(), l_message);
                    }
                }
                else
                    client.sendServerMessage("Do not spamming Out-of-character Message, slow down..");
            }
            else
                client.sendServerBroadcast(QString("Your messages is too long! Please limit it to under %1 characters.").arg(QString::number(ConfigManager::maxCharacters())));

        }
    }
}

AOPacket *PacketCT::CreateMessage(const QString &Message, const QString &cname){
    return PacketFactory::createPacket("CT", {cname.isEmpty() ? ConfigManager::serverTag() : cname, Message, "0"});
}
AOPacket *PacketCT::CreateMessageS(const QString &Message, const QString &cname){
    return PacketFactory::createPacket("CT", {cname.isEmpty() ? ConfigManager::serverTag() : cname, Message, "1"});
}
