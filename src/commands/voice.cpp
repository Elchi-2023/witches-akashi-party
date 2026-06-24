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
#include "aoclient.h"

#include "area_data.h"
#include "config_manager.h"
#include "packet/packet_factory.h"
#include "server.h"

void AOClient::cmdVBlock(int argc, QStringList argv){
    bool cid_ok;
    auto target = server->getClientByID(argv[0].toInt(&cid_ok));
    if (cid_ok && target){
        auto target_area = server->getAreaById(target->areaId());
        if (target_area){
            if (isVAuthenticated()) // even <vip> have perms.. still tho..
                sendServerMessage("This command for Moderator only.");
            else if (target->isAccessBlocked(BlockType::VOICE)){
                QString Message("That target already voice-chat blocked.");
                if (target->GetVCBlockTimer()->isActive())
                    Message.append(QString(" (Until [%1] left)").arg(AOClient::EpochToString(std::chrono::duration_cast<std::chrono::seconds>(target->GetVCBlockTimer()->remainingTimeAsDuration()))));
                if (!target->GetVCBlockReason().isEmpty())
                    Message.append("\nReason: " + target->GetVCBlockReason());
                sendServerMessage(Message);
            }
            else{
                target->SetAccessBlock(BlockType::VOICE, true);
                const auto c_vclist = target_area->GetRegisteredVoiceMap();
                QString Message("Blocking " + AOClient::NameWId(target) + " in voice-chat.");
                QString Notfy("You been blocked from voice-chat by moderator.");
                if (!c_vclist.contains(target->clientId()))
                    Message.append(" (and that target cannot join)");
                else if (c_vclist[target->clientId()]){
                    Message.append(" (and that target cannot speak, they cannot rejoin if they are disconnected/leave)");
                    Notfy.append(" (and you cannot speak)");
                    target_area->SetVoicePeerState(target->clientId(), false);
                    Q_EMIT server->broadcastVState(target->clientId(), false, target_area->index());
                }
                if (argc >= 2){ // <reason or time>
                    const qint64 time = AOClient::CalendarParse(argv[1], true);
                    if (time >= 0){ // assuming 2nd param is <time>
                        if (std::chrono::duration_cast<std::chrono::seconds>(std::chrono::milliseconds(time)).count() >= 1){
                            Message.append(QString("\nfor [%1]").arg(AOClient::EpochToString(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::milliseconds(time)), true)));
                            Notfy.append(QString("\nfor [%1]").arg(AOClient::EpochToString(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::milliseconds(time)), true)));
                            target->GetVCBlockTimer()->start(time);
                        }

                        const QString reason = argv.mid(2).join(" ");
                        if (!reason.isEmpty()){
                            target->m_vcblock_reason = reason;
                            Message.append("\nReason: " + reason);
                            Notfy.append("\nReason: " + reason);
                        }
                    }
                    else{ // otherwise, just assuming it's <reason>
                        target->m_vcblock_reason = argv.mid(0).join(" ");
                        Message.append("\nReason: " + target->m_vcblock_reason);
                        Notfy.append("\nReason: " + target->m_vcblock_reason);
                    }
                }
                sendServerMessage(Message);
                target->sendServerMessage(Notfy + "\n(" + AOClient::getReprimand(false) + ")");
            }
        }
    }
    else
        sendServerMessage("Invalid client target.");
}
void AOClient::cmdVUBlock(int argc, QStringList argv){
    Q_UNUSED(argc)

    if (isVAuthenticated()) // even <vip> have perms.. still tho..
        sendServerMessage("This command for Moderator only.");
    else{
        bool cid_ok;
        auto target = server->getClientByID(argv[0].toInt(&cid_ok));
        if (cid_ok && target){
            if (target->isAccessBlocked(BlockType::VOICE)){
                sendServerMessage("Unblocking " + AOClient::NameWId(target) + " from voice-chat.");
                target->SetAccessBlock(BlockType::VOICE, false);
                target->sendServerMessage("You are been released from blocked voice-chat by moderator. " + AOClient::getReprimand(true));
            }
            else
                sendServerMessage("That target unblocked from voice-chat blocked.");
        }
        else
            sendServerMessage("Invalid client target.");
    }
}
void AOClient::cmdVKick(int argc, QStringList argv){
    bool cid_ok;
    auto target = server->getClientByID(argv[0].toInt(&cid_ok));
    if (cid_ok && target){
        auto area = server->getAreaById(target->areaId());
        if (area){
            const QVector<int> getuserid = area->GetRegisteredVoiceID();
            if (isMAuthenticated()){ // [moderator/root] can kick someone with/out being in voice-chat..
                if (area->RegisterVoice(target->clientId(), true)){
                    if (target == this)
                        sendServerMessage("Are you sure about you want been kicked yourself from the voice-chat?, here goes nothing.. don't blame me.");
                    else{
                        target->sendServerMessage("You are kicked from voice-chat by Moderator");
                        sendServerMessage(QString("Kicking %1 from voice-chat in %1 area.").arg(AOClient::NameWId(target), area->index() == areaId() ? "this" : QString("[%1] %2").arg(area->index()).arg(area->name())));
                    }
                    server->broadcastVJoinLeave(target->clientId(), true, area->index());
                }
                else
                    sendServerMessage("That target aren't in voice-chat.");

                if (argc >= 2) // kick-and-block..
                    checkPermission(ACLRole::MUTE) ? this->cmdVBlock(argc, argv) : sendServerMessage("\n(You need <Mute> permit for block someone's voice-chat)");
            }
            else if (getuserid.contains(clientId())){ // user must in voice-chat..
                if (!area->joinedIDs().contains(target->clientId()))
                    sendServerMessage("That target aren't in this area.");
                else if (area->RegisterVoice(target->clientId(), true)){
                    if (target == this)
                        sendServerMessage("Are you sure about you want been kicked yourself from the voice-chat?, here goes nothing.. don't blame me.");
                    else{
                        target->sendServerBroadcast(QString("You are kicked from voice-chat by %1").arg(AOClient::NameWId(this)));
                        sendServerMessage(QString("Kicking %1 from voice-chat.").arg(AOClient::NameWId(target)));
                    }
                    server->broadcastVJoinLeave(target->clientId(), true, area->index());
                }
                else
                    sendServerMessage("That target aren't in voice-chat.");
            }
            else
                sendServerMessage("You must in voice-chat for using this command.");
        }
    }
    else
        sendServerMessage("Invalid client target.");
}
