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
#include "packet/packet_ms.h"
#include "packet/packet_ct.h"

// This file is for commands under the messaging category in aoclient.h
// Be sure to register the command in the header before adding it here!

void AOClient::cmdHoliday(int argc, QStringList argv)
{
    Q_UNUSED(argc);

    const auto& l_holiday = ConfigManager::holidaylist();
    const QString target = argv.join(" ").trimmed(); /* never forgot to join the QStringList */

    if (m_holiday_mode.first){
        if (target.isEmpty()){ /* if there are no arguments, send the holiday list to ooc. */
            QStringList l_holiday_list;
            for (auto i = l_holiday.begin(); i != l_holiday.end(); i++)
                l_holiday_list << QString(i.key().toLower() == m_holiday_mode.second.toLower() ? (m_version.type == AOClient::ClientVersion::NDS ? " > [" + i.key() + "]" : " 👉 [" + i.key() + "]") : " · [" + i.key() + "]"); /* pointing out which user holiday mode on, otherwise */
            sendPacket("CT", {"[Holiday Mode]", l_holiday_list.isEmpty() ? "The holiday list unavailable." : "The holiday list available as follows:\n" + l_holiday_list.join('\n') , "1"});
        }
        else{
            if (l_holiday.contains(target.toLower())){
                if (m_holiday_mode.second == target.toLower())
                    sendPacket("CT", {"[Holiday Mode]", QString("You already on %1 holiday mode").arg(m_holiday_mode.second), "1"});
                else{
                    sendPacket("CT", {"[Holiday Mode]", QString("Changes from %1 to %2.").arg(m_holiday_mode.second, target), "1"});
                    m_holiday_mode.second = target;
                }
            }
            else
                sendPacket("CT", {"[Holiday Mode]", QString("%1 Not exist on the holiday list.").arg(target), "1"});
        }
    }
    else{
        if (target.isEmpty()){
            QStringList l_holiday_list;
            for (auto i = l_holiday.begin(); i != l_holiday.end(); i++)
                l_holiday_list << " · [" + i.key() + "]";
            sendPacket("CT", {"[Holiday Mode]", l_holiday_list.isEmpty() ? "The holiday list unavailable." : "The holiday list available as follows:\n" + l_holiday_list.join('\n') , "1"});
        }
        else if (l_holiday.contains(target.toLower())){
            sendPacket("CT", {"[Holiday Mode]", QString("Holiday Mode enable and set to %2 mode.").arg(target), "1"});
            m_holiday_mode = qMakePair(true, target);
        }
        else
            sendPacket("CT", {"[Holiday Mode]", QString("%1 Not exist on the holiday list.").arg(target), "1"});
    }
}

void AOClient::cmdUnHoliday(int argc, QStringList argv)
{
    Q_UNUSED(argc)
    Q_UNUSED(argv)

    if (m_holiday_mode.first){
        m_holiday_mode = qMakePair(false, QString());
        sendPacket("CT", {"[Holiday Mode]", "Holiday Mode disabled", "1"});
    }
    else
        sendPacket("CT", {"[Holiday Mode]", "Holiday Mode are disabled", "1"});
}

void AOClient::cmdPair(int argc, QStringList argv){
    switch (argc){
    case 0:
        sendServerMessage("Please inserts target ID if you want pairing someone.");
        break;
    default:
        bool ok;
        auto target = server->getClientByID(argv[0].toInt(&ok));
        if (ok && !target.isNull()){
            if (target == this)
                sendServerMessage("You cannot pair with yourself!");
            else{
                auto l_area = server->getAreaById(areaId());
                if (l_area.isNull())
                    return;

                if (!l_area->joinedIDs().contains(target->clientId()))
                    sendServerMessage("That target weren't on this area, make sure you check if that target were on this area.");
                else{
                    bool ispaired_other = l_area->get_pair_sync_clientID(target->clientId()) > -1 && l_area->get_pair_sync_clientID(target->clientId()) != clientId(); // checker if target are pairing with someone..
                    const QHash<int, int> current_joined = l_area->PlayerJoinedMap();
                    if (!ispaired_other){ // double check moment (client-side by <char_id>)..
                        const auto cpaired_other = server->getClientByID(current_joined.value(target->m_pairing_with, -1)); // getting client by <char_id>..
                        ispaired_other = target->m_pairing_with > -1 && current_joined.values().contains(target->m_pairing_with) && !cpaired_other.isNull() && cpaired_other->m_pairing_with > -1 && cpaired_other->m_pairing_with != target->m_pairing_with;
                    }

                    if (l_area->addPairSync(clientId(), target->clientId())){ /* oh this?.. well.. user choices the target that'll adds/change */
                        if (ispaired_other){
                            l_area->removePairSync(clientId());
                            sendServerMessage("That target are already paired with someone.");
                        }
                        else {
                            sendServerMessage(QString("You are now paired with %1 and synced with, Make sure that target also selected you.").arg(AOClient::NameWId(target)));
                            m_pair_order = 0;
                        }
                    }
                    else
                        sendServerMessage("You are already synced pairing with that target.");
                }
            }
        }
        else
             sendServerMessage("That does not look like a valid ID/Client.");
        break;
    }
}

void AOClient::cmdUnPair(int argc, QStringList argv) 
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    auto l_area = server->getAreaById(areaId());
    if (l_area.isNull())
        return;

    if (l_area->removePairSync(clientId())){
        sendServerMessage("You are not longer paired.");
        m_pair_order = -1;
    }
    else
        sendServerMessage("You are not pairing with anyone, do /pair <client id> if you want pairing someone.");
}

void AOClient::cmdPairOrder(int argc, QStringList argv)
{
    Q_UNUSED(argc)

    if (argv.isEmpty())
        sendServerMessage("Please insert argument: front (or 0) or behind (or 1).");
    else{
        bool okint;
        const int intvalue = argv[0].toInt(&okint);

        if (okint){
            if (intvalue >= 0 && 1 <= intvalue){
                m_pair_order = intvalue;
                sendServerMessage("Pair order changed to: " + QString(m_pair_order > 0 ? "Behind." : "Front."));
                if (!m_version.is_webao() && m_version.major > 8)
                    sendServerMessage("This commands useful for legecy client (aka 2.8.x and below).\n(unless you are lazy to selecting it on your client instead)");
            }
            else if (intvalue == -1 && m_pair_order > -1){
                m_pair_order = -1;
                sendServerMessage("Reseted pair order to client-side.");
            }
            else
                sendServerMessage("Invalid input. Please insert argument: front (or 0) or behind (or 1).");
        }
        else{
            if (argv[0].compare("behind", Qt::CaseInsensitive) == 0 || argv[0].compare("front", Qt::CaseInsensitive) == 0){
                m_pair_order = int(argv[0].compare("behind", Qt::CaseInsensitive) == 0);
                sendServerMessage("Pair order changed to: " + QString(m_pair_order > 0 ? "Behind." : "Front."));
            }
            else if (argv[0].compare("rst", Qt::CaseInsensitive) == 0){
                m_pair_order = -1;
                sendServerMessage("Reseted pair order to client-side.");
            }
            else
                sendServerMessage("Invalid input. Please insert argument: front (or 0) or behind (or 1).");
        }
    }
}

void AOClient::cmdOffset(int argc, QStringList argv){
    if (argv.isEmpty()){ /* when there is no input, we show the current offset */
        if (m_offset_override.isEmpty()){ /* we check if the override is empty or resetted */
            const QPair<int, int> current_offset(qMakePair(m_offset.split("&").size() >= 1 ? m_offset.split("&")[0].toInt() : 0, m_offset.split("&").size() >= 2 ? m_offset.split("&")[1].toInt() : 0));
            if (m_version.is_webao()) /* since webao doesn't tell user about what's the pair silder value.. why not? */
                sendServerMessage(QString("Your slider offset values is: horizontal: %1% and vertical: %2%.").arg(current_offset.first).arg(current_offset.second));
            else if (m_version.type == ClientVersion::ClientType::NORMAL && m_version.major < 9)
                sendServerMessage(QString("Your client-side X-offset is: %1.").arg(current_offset.first));
            else
                sendServerMessage(QString("Your client-side offset is: [X: %1, Y: %2].").arg(current_offset.first).arg(current_offset.second));
        }
        else{
            const QPair<int, int> current_override_offset(qMakePair(m_offset_override.split("&").size() >= 1 ? m_offset_override.split("&")[0].toInt() : 0, m_offset_override.split("&").size() >= 2 ? m_offset_override.split("&")[1].toInt() : 0));
            if (!m_version.is_webao()){
                if (m_version.type == ClientVersion::ClientType::NORMAL && m_version.major > 8) /* just tell the user of the diff for non-legecy client user */
                    sendServerMessage(QString("Your offset (server-side) is: X: %1 and Y: %2.\nDo [/offset rst] for using your client-side offset.").arg(current_override_offset.first).arg(current_override_offset.second));
                else /* due of line 214.. this just in case */
                    sendServerMessage(QString("Your offset is: X: %1 and Y (server-side, not supported by your client): %2.\n(note: You cannot using /offset [value] cause you using legecy client.").arg(current_override_offset.first).arg(current_override_offset.second));
            }
            else /* webao terms style(s) */
                sendServerMessage(QString("Your offset (server-side) values: horizontal: %1 and vertical: %2\nDo [/offset rst] for using your webao/client-side offset.").arg(current_override_offset.first).arg(current_override_offset.second));
        }
    }
    else if (argv[0].compare("rst", Qt::CaseInsensitive) == 0){
        if (m_offset_override.isEmpty())
            sendServerMessage("Your offset (server-side) is reseted and be using client-side offset instead.");
        else{
            sendServerMessage("Your offset (server-side) now reseted, you can use client-side offset now.");
            m_offset_override.clear();
        }
    }
    else{
        const QPair<int, int> current_override_offset(qMakePair(m_offset_override.split("&").size() >= 1 ? m_offset_override.split("&")[0].toInt() : 0, m_offset_override.split("&").size() >= 2 ? m_offset_override.split("&")[1].toInt() : 0));

        switch (argc){ /* why not "case 0" if you may ask?.. cause QStringList::isEmpty() helped for checker of "size() == 0 or argc == 0" anyways */
        case 1: /* if user change/set X-offset */
        {
            bool X_Pass = false; /* qt6/msvc compiler warning moment if w/o default value, just in case. . */
            const int Target_X = qBound(-100, argv[0].toInt(&X_Pass) , 100); /* [https:://doc.qt.io/qt-5/qtcore/qtglobal.html#qBound] (if you are qt6 user just.. change "qt-5" to "qt-6") should explained about qbound is. . */

            if (X_Pass){
                if (m_offset_override.isEmpty()){ /* > setup < */
                    if (m_version.is_webao()) /* another webao terms style(s) */
                        sendServerMessage(QString("Setup Horizontal-offset (server-side) to [%1] with Vertical-offset default 0.\nin this state, you cannot using your webao/client-side of the pair sliders offset (not matter what, even changing the pair sliders).\nTo revert this: do [/offset rst] if you wish to using using your webao/client-side offset.").arg(QString::number(Target_X)));
                    else
                        sendServerMessage(QString("Setup X-offset (server-side) to [%1] with Y-offset default 0.\nin this state, you cannot using your client-side offset (not matter what, even changing the pair value).\nTo revert this: do [/offset rst] for using your client-side offset.").arg(QString::number(Target_X)));
                    m_offset_override = QStringList({QString::number(Target_X), "0"}).join("&");
                }
                else if (current_override_offset.first != Target_X){ /* > comparing between current X=offset value and target X=offset value < */
                    sendServerMessage(QString("Changes Horizontal/X-offset (server-side) from [%1] to [%2].").arg(QString::number(current_override_offset.first), QString::number(Target_X)));
                    m_offset_override = QStringList({QString::number(Target_X), QString::number(current_override_offset.second)}).join("&");
                }
                else /* otherwise, tell to user if the value are same.. */
                    sendServerMessage(QString("Your Horizontal/X-offset (server-side) is already been sets at %1").arg(current_override_offset.first));
            }
            else
                sendServerMessage(m_offset_override.isEmpty() ? "Invalid setup, type a number between -100 and 100." : "Invalid X-offset, type a number between -100 and 100.");
        }
            break;
        case 2: default: /* if user change/set Both offset */
        {
            QPair<bool, bool> V_pass = {false, false}; /* this time.. using qpair instead */
            const QPair<int, int> Target = qMakePair(qBound(-100, argv[0].toInt(&V_pass.first) , 100), qBound(-100, argv[1].toInt(&V_pass.second) , 100));

            if (argv[0] == "*"){ /* keep X-offset as is, focus on Y-offset */
                if (V_pass.second){
                    if (m_offset_override.isEmpty()){
                        if (m_version.is_webao())
                            sendServerMessage(QString("Setup Vertical-offset (server-side) to [%1] with Horizontal-offset default 0.\nin this state, you cannot using your webao/client-side of the pair sliders offset (not matter what, even changing the pair sliders).\nTo revert this: do [/offset rst] if you wish to using your webao/client-side offset.").arg(QString::number(Target.second)));
                        else
                            sendServerMessage(QString("Setup Y-offset (server-side) to [%1] with X-offset default 0.\nin this state, you cannot using your client-side offset (not matter what, even changing the pair value).\nTo revert this: do [/offset rst] for using your client-side offset.").arg(QString::number(Target.second)));
                        m_offset_override = QStringList({"0", QString::number(Target.second)}).join("&");
                    }
                    else if (current_override_offset.second != Target.second){ /* > comparing between current Y-offset value and target Y-offset value < */
                        sendServerMessage(QString("Changes Vertical/Y-offset (server-side) from [%1] to [%2].").arg(QString::number(current_override_offset.second), QString::number(Target.second)));
                        m_offset_override = QStringList({QString::number(current_override_offset.first), QString::number(Target.second)}).join("&");
                    }
                    else
                        sendServerMessage(QString("Your Vertical/Y-offset (server-side) is already been sets at %1.").arg(current_override_offset.second));
                }
                else
                    sendServerMessage(m_offset_override.isEmpty() ? "Invalid setup, type a number between -100 and 100." : "Invalid Y-offset, type a number between -100 and 100.");
            }
            else if (V_pass.first){ /* > both offset(s) */
                if (V_pass.second){ /* > valid Y-offset should be included < */
                    if (m_offset_override.isEmpty()){
                        if (m_version.is_webao())
                            sendServerMessage(QString("Setup both-offset (server-side) to [Horizontal: %1, Vertical: %2].\nin this state, you cannot using your webao/client-side of the pair sliders offset (not matter what, even changing the pair sliders).\nTo revert this: do [/offset rst] if you wish to using your webao/client-side offset.").arg(QString::number(Target.first), QString::number(Target.second)));
                        else
                            sendServerMessage(QString("Setup both-offset (server-side) to [X: %1, Y:%2] with X-offset default 0.\nin this state, you cannot using your client-side offset (not matter what, even changing the pair value).\nTo revert this: do [/offset rst] for using your client-side offset.").arg(QString::number(Target.first), QString::number(Target.second)));
                        m_offset_override = QStringList({"0", QString::number(Target.second)}).join("&");
                    }
                    else if (current_override_offset != Target){ /* > comparing between current offsets value and target offsets value < */
                        if (current_override_offset.first != Target.first)
                            sendServerMessage(QString("Changes Horizontal/X-offset (server-side) from [%1] to [%2].").arg(QString::number(current_override_offset.first), QString::number(Target.first)));
                        else if (current_override_offset.second != Target.second)
                            sendServerMessage(QString("Changes Vertical/Y-offset (server-side) from [%1] to [%2].").arg(QString::number(current_override_offset.second), QString::number(Target.second)));
                        else
                            sendServerMessage(QString("Changes both-offset (server-side) from [X: %1, Y: %2] to [X: %3, Y: %4].").arg(QString::number(current_override_offset.first), QString::number(current_override_offset.second), QString::number(Target.first), QString::number(Target.second)));
                        m_offset_override = QStringList({QString::number(Target.first), QString::number(Target.second)}).join("&");
                    }
                    else
                        sendServerMessage(QString("both-offset (server-side) is already been sets at [X: %1, Y: %2].").arg(QString::number(current_override_offset.first), QString::number(current_override_offset.second)));

                }
                else{ /* > otherwise, X-offset will be focused, even the "*" or Invalid < */
                    if (m_offset_override.isEmpty()){
                        if (m_version.is_webao())
                            sendServerMessage(QString("Setup Horizontal-offset (server-side) to [%1] with Vertical-offset default 0.\nin this state, you cannot using your webao/client-side of the pair sliders offset (not matter what, even changing the pair sliders).\nTo revert this: do [/offset rst] if you wish to using your webao/client-side offset.").arg(QString::number(Target.first)));
                        else
                            sendServerMessage(QString("Setup X-offset (server-side) to [%1] with Y-offset default 0.\nin this state, you cannot using your client-side offset (not matter what, even changing the pair value).\nTo revert this: do [/offset rst] for using your client-side offset.").arg(QString::number(Target.first)));
                        m_offset_override = QStringList({QString::number(Target.first), "0"}).join("&");
                    }
                    else if (current_override_offset.first != Target.first){
                        sendServerMessage(QString("Changes Horizontal/X-offset (server-side) from [%1] to [%2].").arg(QString::number(current_override_offset.second), QString::number(Target.second)));
                        m_offset_override = QStringList({QString::number(current_override_offset.first), QString::number(Target.second)}).join("&");
                    }
                    else
                        sendServerMessage(QString("Your Horizontal/X-offset (server-side) is already been set at %1.").arg(current_override_offset.first));
                }
            }
            else
                sendServerMessage(m_offset_override.isEmpty() ? "Invalid setups, type a number between -100 and 100." : "Invalid offsets, type a number between -100 and 100.");
        }
            break;
        }
    }
}

void AOClient::cmdPos(int argc, QStringList argv)
{
    Q_UNUSED(argc);

    changePosition(argv[0]);
    auto l_area = server->getAreaById(areaId());
    if (l_area.isNull())
        return;

    updateEvidenceList(l_area);
}

void AOClient::cmdForcePos(int argc, QStringList argv)
{
    Q_UNUSED(argc);

    bool ok;
    QList<AOClient *> l_targets;
    int l_target_id = argv[1].toInt(&ok);
    if (ok){
        auto l_target_client = server->getClientByID(l_target_id);
        if (l_target_client.isNull())
            sendServerMessage("Target ID not found!");
        else if (server->getAreaById(areaId()).isNull() || !server->getAreaById(areaId())->joinedIDs().contains(l_target_client->clientId()))
            sendServerMessage("Target not in this area.");
        else{
            l_target_client->sendServerMessage("Position forcibly changed by CM.");
            l_target_client->changePosition(argv[0]);
            sendServerMessage("Forced a single clients into pos " + argv[0] + ".");
        }
    }
    else if (argv[1] == "*"){ /* force all clients in the area */
        const auto current_area = server->getAreaById(areaId());
        int validClient = 0;
        for (int Index : current_area->joinedIDs()){
            auto target = server->getClientByID(Index);
            if (target.isNull())
                continue;
            target->sendServerMessage("Position forcibly changed by CM.");
            target->changePosition(argv[0]);
            ++validClient;
        }
        sendServerMessage("Forced a " + QString::number(validClient) + " clients into pos " + argv[0] + ".");
    }
    else
        sendServerMessage("That does not look like a valid ID.");
}

void AOClient::cmdG(int argc, QStringList argv)
{
    Q_UNUSED(argc);
    auto current_area = server->getAreaById(areaId());
    if (current_area.isNull())
        return;
    const QPair<QString, QString> l_sender{name(), current_area->name()};

    QString l_sender_message = argv.join(" ");
    if (isCursed(AOClient::CurseType::FULL)){
        l_sender_message = AOClient::MessageToGimped(l_sender_message);
        l_sender_message = AOClient::MessageToMediveal(l_sender_message);
        l_sender_message = AOClient::MessageShaked(l_sender_message);
        l_sender_message = AOClient::MessageToUwU(l_sender_message);
        l_sender_message = AOClient::MessageToPigify(l_sender_message);
        l_sender_message = AOClient::MessageDisemvowel(l_sender_message);
    }
    else{
        if (isCursed(AOClient::CurseType::GIMP))
            l_sender_message = AOClient::MessageToGimped(l_sender_message);
        if (isCursed(AOClient::CurseType::MEDIEVAL) || current_area->isMedievalMode())
            l_sender_message = AOClient::MessageToMediveal(l_sender_message);
        if (isCursed(AOClient::CurseType::SHAKE))
            l_sender_message = AOClient::MessageShaked(l_sender_message);
        if (isCursed(AOClient::CurseType::UWUIFY))
            l_sender_message = AOClient::MessageToUwU(l_sender_message);
        if (isCursed(AOClient::CurseType::PIGIFY))
            l_sender_message = AOClient::MessageToPigify(l_sender_message);
        if (isCursed(AOClient::CurseType::DISEMVOWEL))
            l_sender_message = AOClient::MessageDisemvowel(l_sender_message);
    }

    server->broadcastCAuth(PacketCT::CreateMessage(l_sender_message, "[G][" + l_sender.second + "][" + l_sender.first + "]"), AOClient::AuthenticateType::VIP); // <vip_or_normal>..
    server->broadcastCAuth(PacketCT::CreateMessage(l_sender_message, "[G][" + l_sender.second + "][" + l_sender.first + "][" + m_ipid + "]"), AOClient::AuthenticateType::MODERATOR);
}

void AOClient::cmdNeed(int argc, QStringList argv){
    Q_UNUSED(argc);
    server->broadcast(PacketCT::CreateMessageS(QString("\n=== Advert ===\n%1 needs %2\nin area: %3\n==============").arg(AOClient::NameWId(this), argv.join(" "), server->getAreaName(areaId()))));
}

void AOClient::cmdSwitch(int argc, QStringList argv)
{
    Q_UNUSED(argc);

    const int l_selected_char_id = server->getCharID(argv.join(" "));
    switch (l_selected_char_id){
    case -1:
        sendServerMessage("That does not look like a valid character.");
        break;
    default:
        if (changeCharacter(l_selected_char_id)){
            auto l_area = server->getAreaById(areaId());
            m_char_id = l_selected_char_id;
            if (!l_area.isNull() && l_area->owners().contains(clientId()))
                arup(ARUPType::CM, true);
        }
        else
            sendServerMessage("The character you picked is either taken or invalid.");
        break;
    }
}

void AOClient::cmdRandomChar(int argc, QStringList argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    auto l_area = server->getAreaById(areaId());
    if (l_area.isNull())
        return;

    int l_selected_char_id = genRand(0, server->getCharacterCount() - 1);
    while (l_area->charactersTaken().contains(l_selected_char_id))
        l_selected_char_id = genRand(0, server->getCharacterCount() - 1);

    if (changeCharacter(l_selected_char_id)){
        m_char_id = l_selected_char_id;
        if (l_area->owners().contains(clientId()))
            arup(ARUPType::CM, true);
    }
}

void AOClient::cmdToggleGlobal(int argc, QStringList argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    sendServerMessage("Global chat set to " + QStringList({"hidden", "shown"})[(m_global_enabled = !m_global_enabled)]);
}

void AOClient::cmdPM(int argc, QStringList argv)
{
    Q_UNUSED(argc);

    bool ok;
    auto l_target_client = server->getClientByID(argv[0].toInt(&ok));
    if (!ok || l_target_client.isNull())
        sendServerMessage("That does not look like a valid ID/Client.");
    else{
        if (l_target_client->m_pm_mute)
            sendServerMessage("That user is not recieving PMs.");
        else{
            QString l_message = argv.mid(1).join(" "); // ...which means it will not end up as part of the messages
            if (isCursed(AOClient::CurseType::FULL)){
                l_message = AOClient::MessageToGimped(l_message);
                l_message = AOClient::MessageToMediveal(l_message);
                l_message = AOClient::MessageShaked(l_message);
                l_message = AOClient::MessageToUwU(l_message);
                l_message = AOClient::MessageToPigify(l_message);
                l_message = AOClient::MessageDisemvowel(l_message);
            }
            else{
                if (isCursed(AOClient::CurseType::GIMP))
                    l_message = AOClient::MessageToGimped(l_message);
                if (isCursed(AOClient::CurseType::MEDIEVAL))
                    l_message = AOClient::MessageToMediveal(l_message);
                if (isCursed(AOClient::CurseType::SHAKE))
                    l_message = AOClient::MessageShaked(l_message);
                if (isCursed(AOClient::CurseType::UWUIFY))
                    l_message = AOClient::MessageToUwU(l_message);
                if (isCursed(AOClient::CurseType::PIGIFY))
                    l_message = AOClient::MessageToPigify(l_message);
                if (isCursed(AOClient::CurseType::DISEMVOWEL))
                    l_message = AOClient::MessageDisemvowel(l_message);
            }

            l_target_client->sendServerMessage("Message from " + name() + " (" + QString::number(clientId()) + "): " + l_message);
            sendServerMessage("PM sent to " + QString::number(l_target_client->clientId()) + ". Message: " + l_message);
        }
    }
}

void AOClient::cmdAnnounce(int argc, QStringList argv)
{
    Q_UNUSED(argc);

    sendServerBroadcast("\r\n=== Announcement ===\r\n" + argv.join(" ") + "\r\n=============");
}

void AOClient::cmdM(int argc, QStringList argv)
{
    Q_UNUSED(argc);

    QPair<const QString, QString> l_sender = qMakePair(name(), argv.join(' '));
    if (isCursed(AOClient::CurseType::FULL)){
        l_sender.second = AOClient::MessageToGimped(l_sender.second);
        l_sender.second = AOClient::MessageToMediveal(l_sender.second);
        l_sender.second = AOClient::MessageShaked(l_sender.second);
        l_sender.second = AOClient::MessageToUwU(l_sender.second);
        l_sender.second = AOClient::MessageToPigify(l_sender.second);
        l_sender.second = AOClient::MessageDisemvowel(l_sender.second);
    }
    else{
        if (isCursed(AOClient::CurseType::GIMP))
            l_sender.second = AOClient::MessageToGimped(l_sender.second);
        if (isCursed(AOClient::CurseType::MEDIEVAL) || server->getAreaById(areaId())->isMedievalMode())
            l_sender.second = AOClient::MessageToMediveal(l_sender.second);
        if (isCursed(AOClient::CurseType::SHAKE))
            l_sender.second = AOClient::MessageShaked(l_sender.second);
        if (isCursed(AOClient::CurseType::UWUIFY))
            l_sender.second = AOClient::MessageToUwU(l_sender.second);
        if (isCursed(AOClient::CurseType::PIGIFY))
            l_sender.second = AOClient::MessageToPigify(l_sender.second);
        if (isCursed(AOClient::CurseType::DISEMVOWEL))
            l_sender.second = AOClient::MessageDisemvowel(l_sender.second);
    }
    server->broadcast(PacketCT::CreateMessage(QString("[MODCHAT][%1]").arg(isVAuthenticated() ? "VIP][" + l_sender.first : l_sender.first), l_sender.second), Server::TARGET_TYPE::MODCHAT);
}

void AOClient::cmdGM(int argc, QStringList argv)
{
    Q_UNUSED(argc);

    QString l_sender_name = name();
    QString l_sender_area = server->getAreaName(areaId());
    QString l_sender_message = argv.join(" ");
    server->broadcast(PacketCT::CreateMessageS("[G][" + l_sender_area + "]" + "[" + l_sender_name + "][M]", l_sender_message), Server::TARGET_TYPE::MODCHAT);
}

void AOClient::cmdLM(int argc, QStringList argv)
{
    Q_UNUSED(argc);

    QString l_sender_name = name();
    QString l_sender_message = argv.join(" ");
    server->broadcast(PacketCT::CreateMessage(argv.join(" "), "[M][" + name() + "]"), areaId());
}

void AOClient::cmdMutePM(int argc, QStringList argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    sendServerMessage("PM's are now " + QStringList({"unmuted", "muted"})[(m_pm_mute = !m_pm_mute)]);
}

void AOClient::cmdToggleAdverts(int argc, QStringList argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    sendServerMessage("Advertisements turned " + QStringList({"off", "on"})[(m_advert_enabled = !m_advert_enabled)]);
}

void AOClient::cmdToggleAfkMute(int argc, QStringList argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    sendServerMessage("AFK notifications are now " + QStringList({"hidden", "shown"})[(m_afk_received = !m_afk_received)] + ".");
}

void AOClient::cmdToggleAfkannounce(int argc, QStringList argv){
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    m_afk_announcement = !m_afk_announcement;
    const QStringList String({"You AFK Announcements will not sending to public when (due to inactivity) or /afk.", "You AFK Announcements will sending to public when (due to inactivity) or /afk."});
    sendServerMessage(String[m_afk_announcement]);
}

void AOClient::cmdAfk(int argc, QStringList argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    if (m_afk_announcement){
        if (m_char_id >= 0){
            auto l_area = server->getAreaById(areaId());
            if (l_area.isNull())
                sendServerMessage("You are now AFK. (unannouncement)");
            else{
                if (l_area->GetRegisteredVoiceID(true).contains(clientId())){ // if user are in <vc_users> and <vc_peers>..
                    l_area->SetVoicePeerState(clientId(), false); // unset
                    server->broadcastVState(clientId(), false, l_area->index());
                    sendServerPacketArea(PacketFactory::createPacket("VS_PEERS", {l_area->GetRegisteredVoice(true)}));
                }

                for (const int client_id : l_area->joinedIDs()){
                    auto l_client = server->getClientByID(client_id);
                    if (l_client.isNull())
                        continue;

                    l_client->sendServerMessage(l_client == this ? "You are now AFK, Have an nice day." : QString("[%1] %2 are now AFK.").arg(QString::number(clientId()), character().isEmpty() ? "[Spectator]" : character()));
                }
            }
        }
    }
    else
        sendServerMessage("You are now AFK. (unannouncement)");
    ToggleAFK();
}

void AOClient::cmdCharCurse(int argc, QStringList argv){
    auto current_area = server->getAreaById(areaId());
    if (current_area.isNull())
        return;

    bool conv_ok = false;
    auto l_target = server->getClientByID(argv[0].toInt(&conv_ok));
    if (!conv_ok || l_target.isNull())
        sendServerMessage("No client with that ID found.");
    else if (l_target->isCursed(CurseType::CCURSE))
        sendServerMessage("That player is already charcursed!");
    else{
        switch (argc){
        case 1:
            if (server->getCharID(l_target->character()) <= -1) // preventing target getting stuck in [Spectator]...
                return;
            l_target->m_charcurse_list.append(server->getCharID(l_target->character()));
            break;
        default:
            const QStringList l_char_names = argv.mid(1).join(" ").trimmed().split(", ");
            QList<int> target_charcurse;

            for (auto target_char : l_char_names){
                const int char_id = server->getCharID(target_char);
                if (char_id <= -1)
                    continue;
                target_charcurse.append(char_id);
            }

            if (target_charcurse.isEmpty()){
                if (!l_target->m_charcurse_list.isEmpty())
                    l_target->m_charcurse_list.clear();
                sendServerMessage("Could not found characters you requested to charcursed that target.");
                return;
            }
            else if (l_target->m_charcurse_list != target_charcurse)
                l_target->m_charcurse_list = target_charcurse;
            break;
        }

        l_target->SetCursed(CurseType::CCURSE, true);

        // Kick back to char select screen
        if (!l_target->m_charcurse_list.contains(server->getCharID(l_target->character()))){
            l_target->changeCharacter(-1);
            if (current_area->owners().contains(clientId()))
                arup(ARUPType::CM, true);
            l_target->sendPacket("DONE");
        }
        server->updateCharsTaken(current_area);

        if (l_target == this)
            l_target->sendServerMessage("You have been charcursed yourself!");
        else{
            l_target->sendServerMessage("You have been charcursed!");
            sendServerMessage("Charcursed player.");
        }
    }
}

void AOClient::cmdUnCharCurse(int argc, QStringList argv){
    Q_UNUSED(argc);
    auto current_area = server->getAreaById(areaId());
    if (current_area.isNull())
        return;

    bool conv_ok = false;
    auto l_target = server->getClientByID(argv[0].toInt(&conv_ok));
    if (!conv_ok || l_target.isNull())
        sendServerMessage("No client with that ID found.");
    else if (!l_target->isCursed(CurseType::CCURSE))
        sendServerMessage("That player is not charcursed!");
    else{
        l_target->SetCursed(CurseType::CCURSE, false);
        l_target->m_charcurse_list.clear();
        server->updateCharsTaken(current_area);
        if (l_target == this)
            sendServerMessage("You were uncharcursed yourself.");
        else{
            sendServerMessage("Uncharcursed player.");
            l_target->sendServerMessage("You were uncharcursed.");
        }
    }
}

void AOClient::cmdCharSelect(int argc, QStringList argv){
    switch (argc){
    case 0:
        changeCharacter(-1);
        sendPacket("DONE");
        break;
    default:
        if (checkPermission(ACLRole::FORCE_CHARSELECT))
            cmdForceCharSelect(argc, argv);
        else{
            changeCharacter(-1);
            sendPacket("DONE");
        }
        break;
    }
}

void AOClient::cmdForceCharSelect(int argc, QStringList argv)
{
    Q_UNUSED(argc);

    bool ok = false;
    auto l_target = QPointer<AOClient>(server->getClientByID(argv[0].toInt(&ok)));
    if (!ok)
        sendServerMessage("This ID does not look valid. Please use the client ID.");
    else if (l_target.isNull())
        sendServerMessage("Unable to locate client with ID " + argv[0] + ".");
    else{
        l_target->changeCharacter(-1);
        l_target->sendPacket("DONE");
        sendServerMessage("Client has been forced into character select!");
    }
}

void AOClient::cmdA(int argc, QStringList argv)
{
    Q_UNUSED(argc);

    bool ok;
    int l_area_id = argv[0].toInt(&ok);
    if (ok){
        const auto l_area = server->getAreaById(l_area_id);
        if (l_area.isNull())
            return;
        else if (!l_area->owners().contains(clientId()))
            sendServerMessage("You are not CM in that area.");
        else{
            QPair<const QString, QString> l_sender = qMakePair(name(), argv.mid(1).join(" "));
            if (isCursed(AOClient::CurseType::FULL)){
                l_sender.second = AOClient::MessageToGimped(l_sender.second);
                l_sender.second = AOClient::MessageToMediveal(l_sender.second);
                l_sender.second = AOClient::MessageShaked(l_sender.second);
                l_sender.second = AOClient::MessageToUwU(l_sender.second);
                l_sender.second = AOClient::MessageToPigify(l_sender.second);
                l_sender.second = AOClient::MessageDisemvowel(l_sender.second);
            }
            else{
                if (isCursed(AOClient::CurseType::GIMP))
                    l_sender.second = AOClient::MessageToGimped(l_sender.second);
                if (isCursed(AOClient::CurseType::MEDIEVAL) || l_area->isMedievalMode())
                    l_sender.second = AOClient::MessageToMediveal(l_sender.second);
                if (isCursed(AOClient::CurseType::SHAKE))
                    l_sender.second = AOClient::MessageShaked(l_sender.second);
                if (isCursed(AOClient::CurseType::UWUIFY))
                    l_sender.second = AOClient::MessageToUwU(l_sender.second);
                if (isCursed(AOClient::CurseType::PIGIFY))
                    l_sender.second = AOClient::MessageToPigify(l_sender.second);
                if (isCursed(AOClient::CurseType::DISEMVOWEL))
                    l_sender.second = AOClient::MessageDisemvowel(l_sender.second);
            }

            server->broadcast(PacketCT::CreateMessage(l_sender.second, "[CM] " + l_sender.first), l_area->index());
        }
    }
    else
        sendServerMessage("This does not look like a valid AreaID.");
}

void AOClient::cmdS(int argc, QStringList argv)
{
    Q_UNUSED(argc);

    for (auto Area : server->getAreas()){
        if (!Area.isNull() && Area->owners().contains(clientId())){
            QPair<const QString, QString> l_sender = qMakePair(name(), argv.join(" "));
            if (isCursed(AOClient::CurseType::FULL)){
                l_sender.second = AOClient::MessageToGimped(l_sender.second);
                l_sender.second = AOClient::MessageToMediveal(l_sender.second);
                l_sender.second = AOClient::MessageShaked(l_sender.second);
                l_sender.second = AOClient::MessageToUwU(l_sender.second);
                l_sender.second = AOClient::MessageToPigify(l_sender.second);
                l_sender.second = AOClient::MessageDisemvowel(l_sender.second);
            }
            else{
                if (isCursed(AOClient::CurseType::GIMP))
                    l_sender.second = AOClient::MessageToGimped(l_sender.second);
                if (isCursed(AOClient::CurseType::MEDIEVAL) || Area->isMedievalMode())
                    l_sender.second = AOClient::MessageToMediveal(l_sender.second);
                if (isCursed(AOClient::CurseType::SHAKE))
                    l_sender.second = AOClient::MessageShaked(l_sender.second);
                if (isCursed(AOClient::CurseType::UWUIFY))
                    l_sender.second = AOClient::MessageToUwU(l_sender.second);
                if (isCursed(AOClient::CurseType::PIGIFY))
                    l_sender.second = AOClient::MessageToPigify(l_sender.second);
                if (isCursed(AOClient::CurseType::DISEMVOWEL))
                    l_sender.second = AOClient::MessageDisemvowel(l_sender.second);
            }
            server->broadcast(PacketCT::CreateMessage(l_sender.second, "[CM] " + l_sender.first), Area->index());
        }
    }
}

void AOClient::cmdFirstPerson(int argc, QStringList argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    sendServerMessage("First person mode " + QStringList({"disable.", "enable."})[(m_first_person = !m_first_person)]);
}
