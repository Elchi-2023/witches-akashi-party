//////////////////////////////////////////////////////////////////////////////////////
//    akashi - a server for Attorney Online 2                                       //
//    Copyright (C) 2020  scatterflower                                           //
//                                                                                  //
//    This program is free software: you can redistribute it and/or modify          //
//    it under the terms of the GNU Affero General Public License as                //
//    published by the Free Software Foundation, either version 3 of the            //
//    License, or (at your option) any later version.                               //
//                                                                                  //
//    This program is distributed in the hope that it will be useful,               //
//    but WITHOUT ANY WARRANTY{} without even the implied warranty of                //
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                 //
//    GNU Affero General Public License for more details.                           //
//                                                                                  //
//    You should have received a copy of the GNU Affero General Public License      //
//    along with this program.  If not, see <https://www.gnu.org/licenses/>.        //
//////////////////////////////////////////////////////////////////////////////////////
#include "aoclient.h"

#include <QQueue>

#include "area_data.h"
#include "config_manager.h"
#include "db_manager.h"
#include "music_manager.h"
#include "packet/packet_factory.h"
#include "server.h"

void AOClient::sendEvidenceList(AreaData *area) const
{
    for (int index : area->joinedIDs()){
        auto l_client = server->getClientByID(index);
        if (l_client.isNull())
            continue;
        l_client->updateEvidenceList(area);
    }
}

void AOClient::updateEvidenceList(AreaData *area)
{
    QStringList l_evidence_list;
    QString l_evidence_format("%1&%2&%3");

    const QList<AreaData::Evidence> l_area_evidence = area->evidence();
    for (const AreaData::Evidence &evidence : l_area_evidence) {
        if (!checkPermission(ACLRole::CM) && area->eviMod() == AreaData::EvidenceMod::HIDDEN_CM) {
            QRegularExpression l_regex("<owner=(.*?)>");
            QRegularExpressionMatch l_match = l_regex.match(evidence.description);
            if (l_match.hasMatch()) {
                QStringList owners = l_match.captured(1).split(",");
                if (!owners.contains("all", Qt::CaseSensitivity::CaseInsensitive) && !owners.contains(m_pos, Qt::CaseSensitivity::CaseInsensitive)) {
                    continue;
                }
            }
            // no match = show it to all
        }
        l_evidence_list.append(l_evidence_format.arg(evidence.name, evidence.description, evidence.image));
    }

    sendPacket(PacketFactory::createPacket("LE", l_evidence_list));
}

QString AOClient::dezalgo(QString p_text)
{
    QRegularExpression rxp("([̴̵̶̷̸̡̢̧̨̛̖̗̘̙̜̝̞̟̠̣̤̥̦̩̪̫̬̭̮̯̰̱̲̳̹̺̻̼͇͈͉͍͎̀́̂̃̄̅̆̇̈̉̊̋̌̍̎̏̐̑̒̓̔̽̾̿̀́͂̓̈́͆͊͋͌̕̚ͅ͏͓͔͕͖͙͚͐͑͒͗͛ͣͤͥͦͧͨͩͪͫͬͭͮͯ͘͜͟͢͝͞͠͡])");
    QString filtered = p_text.replace(rxp, "");
    return filtered;
}

bool AOClient::checkEvidenceAccess(AreaData *area)
{
    switch (area->eviMod()) {
    case AreaData::EvidenceMod::FFA:
        return true;
    case AreaData::EvidenceMod::CM:
    case AreaData::EvidenceMod::HIDDEN_CM:
        return checkPermission(ACLRole::CM);
    case AreaData::EvidenceMod::MOD:
        return isMAuthenticated();
    default:
        return false;
    }
}

void AOClient::updateJudgeLog(AreaData *area, AOClient *client, QString action)
{
    QString l_timestamp = QTime::currentTime().toString("hh:mm:ss");
    QString l_uid = QString::number(client->clientId());
    QString l_char_name = client->character();
    QString l_ipid = client->getIpid();
    QString l_message = action;
    QString l_logmessage = QString("[%1]: [%2] %3 (%4) %5").arg(l_timestamp, l_uid, l_char_name, l_ipid, l_message);
    area->appendJudgelog(l_logmessage);
}

QString AOClient::decodeMessage(QString incoming_message)
{
    QString decoded_message = incoming_message.replace("<num>", "#")
                                  .replace("<percent>", "%")
                                  .replace("<dollar>", "$")
                                  .replace("<and>", "&");
    return decoded_message;
}

bool AOClient::loginAttempt(QString message){
    const auto CurrentArea = server->getAreaById(areaId());
    if (CurrentArea.isNull())
        return false;

    auto GetDBManager = server->getDatabaseManager();
    switch (ConfigManager::authType()) {
    case DataTypes::AuthType::SIMPLE:
        if (message == ConfigManager::modpass()) {
            sendPacket("AUTH", {"1"});
            if (m_version.release <= 2 && m_version.major <= 9 && m_version.minor <= 0)
                sendServerMessage("Logged in as a moderator.", "[Login Prompt]");
            m_acl_role_id = ACLRolesHandler::SUPER_ID;
            m_authenticated_type = AuthenticateType::MODERATOR;
            Q_EMIT ModeratorObserver();
            emit logLogin((character() + " " + characterName()), name(), "Moderator", m_ipid, CurrentArea.isNull() ? "[NULL]" : CurrentArea->name(), isAuthenticated());
            return true;
        }
        else {
            sendPacket("AUTH", {"0"}); // Client: "Login unsuccessful."
            sendServerMessage("Incorrect password.", "[Login Prompt]");
            emit logLogin((character() + " " + characterName()), name(), "Moderator", m_ipid, CurrentArea.isNull() ? "[NULL]" : CurrentArea->name(), isAuthenticated());
        }
        break;
    case DataTypes::AuthType::ADVANCED:
        if (message.split(" ").size() >= 2){
            const QPair<QString, QString> user_login{message.split(" ")[0], message.split(" ")[1]};
            const QHash<int, AuthenticateType> roletype{{0, AuthenticateType::VIP}, {1, AuthenticateType::MODERATOR}, {2, AuthenticateType::ROOT}};
            if (GetDBManager->authenticate(user_login.first, user_login.second)){
                m_acl_role_id = GetDBManager->getACL(user_login.first);
                m_moderator_name = user_login.first;
                m_authenticated_type = roletype[GetDBManager->getUserType(user_login.first)];
                sendPacket("AUTH", {QString::number(isMAuthenticated())});
                if (m_version.release <= 2 && m_version.major <= 9 && m_version.minor <= 0) // legecy client(?)..
                    sendServerMessage(QString("Logged in as a %1.").arg(QStringList({"VIP", "Moderator", "[ROOT]"})[m_authenticated_type]));
                switch (m_authenticated_type){
                case AuthenticateType::ROOT:
                    sendServerMessage(QString("Hello and welcome, %1").arg(m_moderator_name), "[Login Prompt]");
                    Q_EMIT ModeratorObserver();
                    break;
                case AuthenticateType::MODERATOR:
                    sendServerMessage(QString("Welcome moderator %1.").arg(m_moderator_name), "[Login Prompt]");
                    Q_EMIT ModeratorObserver();
                    break;
                default:
                    sendServerMessage("Welcome, " + user_login.first, "[Login Prompt]");
                    break;
                }
                emit logLogin((character() + " " + characterName()), name(), user_login.first, m_ipid, CurrentArea.isNull() ? "[NULL]" : CurrentArea->name(), isAuthenticated());
                return true;
            }
            else {
                sendPacket("AUTH", {"0"});
                sendServerMessage("Incorrect password.", "[Login Prompt]");
                emit logLogin((character() + " " + characterName()), name(), user_login.first, m_ipid, CurrentArea.isNull() ? "[NULL]" : CurrentArea->name(), isAuthenticated());
            }
        }
        else
            sendServerMessage("You must specify a username and a password", "[Login Prompt]");
        break;
    }
    return false;
}
