#include "packet/packet_ct.h"

#include "config_manager.h"
#include "packet/packet_factory.h"
#include "server.h"

#include <QDebug>
#include <QRegularExpression>
#include <QPointer>

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

void PacketCT::handlePacket(AreaData *area, AOClient &client) const
{
    if (client.m_is_ooc_muted && !client.m_authenticated) {
        client.sendServerMessage("You are OOC muted, and cannot speak.");
        return;
    }

    const QString Name = client.dezalgo(m_content[0]).remove(QRegularExpression("\\[|\\]|\\{|\\}|\\#|\\$|\\%|\\&")).remove(QString::fromUtf8("\xE2\x80\x8B")).remove(QString::fromUtf8("\xE2\x80\x8E")); // no fucky wucky shit here
    if (Name.trimmed().isEmpty() || Name == ConfigManager::serverName()) /* impersonation & empty name protection */
        return;

    if (Name.length() > 30) {
        client.sendServerMessage("Your name is too long! Please limit it to under 30 characters.");
        return;
    }

    client.setName(Name);
    if (client.m_is_logging_in){
        if (m_content[1].toLower() == "/cancel"){
            client.m_is_logging_in = false;
            client.sendServerMessage("Exiting login prompt.");
            client.totalAttempt = qMakePair(0, 0); /* reset the counts */
            return;
        }

        const bool Pass = client.loginAttempt(m_content[1]);
        if (Pass)
            client.totalAttempt = qMakePair(0, 0); /* reset the counts */
        else{
            ++client.totalAttempt.first;
            if (client.totalAttempt.first > 3){
                ++client.totalAttempt.second;
                for (auto I : client.getServer()->getClients()){
                    if (!QPointer<AOClient>(I).isNull() && I->m_authenticated)
                        I->sendPacket("CT", {"[LOGIN]", QString("A user %1 (aka %2) attempted to logining, %3 tries.").arg(client.m_ipid, client.name(), QString::number(client.totalAttempt.second)), "1"});
                }
                client.totalAttempt.first = 0;
            }
            client.sendServerMessage("Try again.");
        }
        return;
    }

    QString l_message = client.dezalgo(m_content[1]);

    if (l_message.length() == 0 || l_message.length() > ConfigManager::maxCharacters())
        return;

    if (!ConfigManager::filterList().isEmpty()) {

        QString l_censor_message = l_message.toLower();

        foreach (const QString &regex, ConfigManager::filterList()) {
            QRegularExpression re(regex, QRegularExpression::CaseInsensitiveOption);
            l_message.replace(re, "❌");
        }
    }

    if (l_message.at(0) == '/') {
        QStringList l_cmd_argv = l_message.split(" ", Qt::SkipEmptyParts);
        QString l_command = l_cmd_argv[0].trimmed().toLower();
        l_command = l_command.right(l_command.length() - 1);
        l_cmd_argv.removeFirst();
        int l_cmd_argc = l_cmd_argv.length();

        client.handleCommand(l_command, l_cmd_argc, l_cmd_argv);
        emit client.logCMD((client.character() + " " + client.characterName()), client.m_ipid, client.name(), l_command, l_cmd_argv, client.getServer()->getAreaById(client.areaId())->name());
        return;
    }
    else if (!client.m_is_ooc_muted){
        if (client.m_is_gimped)
            l_message = ConfigManager::gimpList().at((client.genRand(1, ConfigManager::gimpList().size() - 1)));
        if (client.m_is_shaken) {
            QStringList l_parts = l_message.split(QRegularExpression(R"([^A-Za-z0-9]+)"));

            std::random_device rng;
            std::mt19937 urng(rng());
            std::shuffle(l_parts.begin(), l_parts.end(), urng);

            l_message = l_parts.join(" ");
        }
        if (client.m_is_disemvoweled)
            l_message = QString(l_message).remove(QRegularExpression("[AEIOUaeiou]"));
        
        AOPacket *final_packet = PacketFactory::createPacket("CT", {client.name(), l_message, "0"});
        client.getServer()->broadcast(final_packet, client.areaId());
    }
    else
        client.sendServerMessage("You are OOC muted, and cannot speak. (even you are moderator)");
    
    emit client.logOOC((client.character() + " " + client.characterName()), client.name(), client.m_ipid, area->name(), l_message);
}
