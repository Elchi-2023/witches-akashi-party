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
                l_holiday_list << QString(i.key().toLower() == m_holiday_mode.second.toLower() ? " 👉 [" + i.key() + "]" : " · [" + i.key() + "]"); /* pointing out which user holiday mode on, otherwise */
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

void AOClient::cmdPair(int argc, QStringList argv)
{
    if (argc < 1){
        sendServerMessage("Please inserts target ID if you want pairing someone.");
        return;
    }

    bool ok;
    QList<AOClient *> l_targets;
    int l_target_id = argv[0].toInt(&ok);

    if (!ok){
        sendServerMessage("That does not look like a valid ID.");
        return;
    }
    else{
        auto l_target_client = QPointer<AOClient>(server->getClientByID(l_target_id));
        if (l_target_client.isNull()) {
            sendServerMessage("Target ID not found!");
            return;
        }

        if (l_target_client == this) {
            sendServerMessage("You cannot pair with yourself!");
            return;
        }

        auto current_area = server->getAreaById(areaId());
        if (!current_area->joinedIDs().contains(l_target_client->clientId()))
            sendServerMessage("That target weren't on this area, make sure you check if that target were on this area.");
        else if (current_area->addPairSync(clientId(), l_target_client->clientId())){ /* oh this?.. well.. user choices the target that'll adds/change */
            sendServerMessage("You are now paired with " + QString(l_target_client->characterName().isEmpty() ? "client id " + QString::number(l_target_client->clientId()) : l_target_client->characterName()) + " and synced with that target, Make sure that targets also selected you.");
            m_pair_order = 0;
        }
        else
            sendServerMessage("You are already synced pairing with that target.");
    }
}

void AOClient::cmdUnPair(int argc, QStringList argv) 
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    auto current_area = server->getAreaById(areaId());
    if (current_area->removePairSync(clientId())){
        sendServerMessage("You are not longer paired.");
        m_pair_order = -1;
    }
    else
        sendServerMessage("You are not pairing with anyone, do /pair id if you want pairing someone.");
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
                if (!m_version.is_webao && m_version.major > 8)
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
            if (m_version.is_webao) /* since webao doesn't tell user about what's the pair silder value.. why not? */
                sendServerMessage(QString("Your slider offset values is: horizontal: %1 and vertical: %2.").arg(current_offset.first).arg(current_offset.second));
            else if (m_version.major > 8)
                sendServerMessage(QString("Your client-side offset is: [X: %1, Y: %2].").arg(current_offset.first).arg(current_offset.second));
            else
                sendServerMessage(QString("Your client-side X-offset is: %1.").arg(current_offset.first));
        }
        else{
            const QPair<int, int> current_override_offset(qMakePair(m_offset_override.split("&").size() >= 1 ? m_offset_override.split("&")[0].toInt() : 0, m_offset_override.split("&").size() >= 2 ? m_offset_override.split("&")[1].toInt() : 0));
            if (!m_version.is_webao){
                if (m_version.major > 8) /* just tell the user of the diff for non-legecy client user */
                    sendServerMessage(QString("Your offset (server-side) is: X: %1 and Y: %2.\nDo [/offset rst] for using your client-side offset.").arg(current_override_offset.first).arg(current_override_offset.second));
                else /* due of line 214.. this just in case */
                    sendServerMessage(QString("Your offset is: X: %1 and Y (server-side, not supported by your client): %2.\n(note: You cannot using /offset [value] cause you using legecy client.").arg(current_override_offset.first).arg(current_override_offset.second));
            }
            else /* webao terms style(s) */
                sendServerMessage(QString("Your offset (server-side) values: horizontal: %1 and vertical: %2\nDo [/offset rst] for using your webao/client-side offset.").arg(current_override_offset.first).arg(current_override_offset.second));
        }
    }
    else if (argv[0].compare("rst", Qt::CaseInsensitive) == 0){
        if (!m_version.is_webao && m_version.major <= 8)
            sendServerMessage("This command param become nothing since you using legecy client.");
        else if (m_offset_override.isEmpty())
            sendServerMessage("Your offset (server-side) is reseted and be using client-side offset instead.");
        else{
            sendServerMessage("Your offset (server-side) now reseted, you can use client-side offset now.");
            m_offset_override.clear();
        }
    }
    else{
        if (!m_version.is_webao && m_version.major <= 8){ /* yeah.. i'm not sure if og legecy client could show the y-offset (unless if user is using custom legecy client..) */
            sendServerMessage("You cannot using this command (aka /offset [value]) cause you using legecy client.");
            return;
        }

        const QPair<int, int> current_override_offset(qMakePair(m_offset_override.split("&").size() >= 1 ? m_offset_override.split("&")[0].toInt() : 0, m_offset_override.split("&").size() >= 2 ? m_offset_override.split("&")[1].toInt() : 0));

        switch (argc){ /* why not "case 0" if you may ask?.. cause QStringList::isEmpty() helped for checker of "size() == 0 or argc == 0" anyways */
        case 1: /* if user change/set X-offset */
            {
                bool X_Pass = false; /* qt6/msvc compiler warning moment if w/o default value, just in case. . */
                const int Target_X = qBound(-100, argv[0].toInt(&X_Pass) , 100); /* [https:://doc.qt.io/qt-5/qtcore/qtglobal.html#qBound] (if you are qt6 user just.. change "qt-5" to "qt-6") should explained about qbound is. . */

                if (X_Pass){
                    if (m_offset_override.isEmpty()){ /* > setup < */
                        if (m_version.is_webao) /* another webao terms style(s) */
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

                if (argv[0].compare("*", Qt::CaseInsensitive) == 0){ /* keep X-offset as is, focus on Y-offset */
                    if (V_pass.second){
                        if (m_offset_override.isEmpty()){
                            if (m_version.is_webao)
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
                    if (V_pass.second){ /* > vaild Y-offset should be included < */
                        if (m_offset_override.isEmpty()){
                            if (m_version.is_webao)
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
                    else{ /* > otherwise, X-offset will be focused, even the "*" or invaild < */
                        if (m_offset_override.isEmpty()){
                            if (m_version.is_webao)
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
    updateEvidenceList(server->getAreaById(areaId()));
}

void AOClient::cmdForcePos(int argc, QStringList argv)
{
    Q_UNUSED(argc);

    bool ok;
    QList<AOClient *> l_targets;
    int l_target_id = argv[1].toInt(&ok);
    if (ok){
        auto l_target_client = QPointer<AOClient>(server->getClientByID(l_target_id));
        if (l_target_client.isNull())
            sendServerMessage("Target ID not found!");
        else if (!server->getAreaById(areaId())->joinedIDs().contains(l_target_client->clientId()))
            sendServerMessage("Target not in this area.");
        else{
            l_target_client->sendServerMessage("Position forcibly changed by CM.");
            l_target_client->changePosition(argv[0]);
            sendServerMessage("Forced a single clients into pos " + argv[0] + ".");
        }
    }
    else if (argv[1] == "*"){ /* force all clients in the area */
        const auto current_area = server->getAreaById(areaId());
        int vaildClient = 0;
        for (int Index : current_area->joinedIDs()){
            auto target = QPointer<AOClient>(server->getClientByID(Index));
            if (target.isNull())
                continue;
            target->sendServerMessage("Position forcibly changed by CM.");
            target->changePosition(argv[0]);
            ++vaildClient;
        }
        sendServerMessage("Forced a " + QString::number(vaildClient) + " clients into pos " + argv[0] + ".");
    }
    else
        sendServerMessage("That does not look like a valid ID.");
}

void AOClient::cmdG(int argc, QStringList argv)
{
    Q_UNUSED(argc);

    QString l_sender_name = name();
    QString l_sender_area = server->getAreaName(areaId());
    QString l_sender_message = argv.join(" ");
    if (m_is_gimped)
        l_sender_message = ConfigManager::gimpList().at((genRand(1, ConfigManager::gimpList().size() - 1)));
    if (m_is_medieval || server->getAreaById(areaId())->isMedievalMode())
        l_sender_message = server->getMedievalParser()->degrootify(l_sender_message);
    if (m_is_shaken) {
        QStringList l_parts = l_sender_message.split(QRegularExpression(R"([^A-Za-z0-9]+)"));

        std::random_device rng;
        std::mt19937 urng(rng());
        std::shuffle(l_parts.begin(), l_parts.end(), urng);

        l_sender_message = l_parts.join(" ");
    }
    if (m_is_disemvoweled)
        l_sender_message = QString(l_sender_message).remove(QRegularExpression("[AEIOUaeiou]"));

    for (AOClient *I : server->getClients()){
        if (QPointer<AOClient>(I).isNull() || !I->m_joined || !I->m_global_enabled)
            continue;

        /* formatting would be like "[user area][user name]" instead of "[username]" */
        QStringList Name({"[" + l_sender_area + "]", "[" + l_sender_name + "]"});
        if (I->isAuthenticated())
            Name.prepend("[" + m_ipid + "]");
        I->sendPacket("CT", {"[G]" + Name.join(""), l_sender_message});
    }
}

void AOClient::cmdNeed(int argc, QStringList argv)
{
    Q_UNUSED(argc);

    QString l_sender_area = server->getAreaName(areaId());
    QString l_sender_message = argv.join(" ");
    server->broadcast(PacketFactory::createPacket("CT", {ConfigManager::serverTag(), "=== Advert ===\n[" + l_sender_area + "] needs " + l_sender_message + "."}), Server::TARGET_TYPE::ADVERT);
}

void AOClient::cmdSwitch(int argc, QStringList argv)
{
    Q_UNUSED(argc);

    int l_selected_char_id = server->getCharID(argv.join(" "));
    if (l_selected_char_id == -1) {
        sendServerMessage("That does not look like a valid character.");
        return;
    }
    if (changeCharacter(l_selected_char_id)) {
        m_char_id = l_selected_char_id;
    }
    else {
        sendServerMessage("The character you picked is either taken or invalid.");
    }
}

void AOClient::cmdRandomChar(int argc, QStringList argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    AreaData *l_area = server->getAreaById(areaId());
    int l_selected_char_id;
    bool l_taken = true;
    while (l_taken) {
        l_selected_char_id = genRand(0, server->getCharacterCount() - 1);
        if (!l_area->charactersTaken().contains(l_selected_char_id))
            l_taken = false;
    }
    if (changeCharacter(l_selected_char_id))
        m_char_id = l_selected_char_id;
}

void AOClient::cmdToggleGlobal(int argc, QStringList argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    m_global_enabled = !m_global_enabled;
    QString l_str_en = m_global_enabled ? "shown" : "hidden";
    sendServerMessage("Global chat set to " + l_str_en);
}

void AOClient::cmdPM(int argc, QStringList argv)
{
    Q_UNUSED(argc);

    bool ok;
    int l_target_id = argv[0].toInt(&ok);
    if (!ok) {
        sendServerMessage("That does not look like a valid ID.");
        return;
    }
    else{
        auto l_target_client = QPointer<AOClient>(server->getClientByID(l_target_id));
        if (l_target_client.isNull())
            sendServerMessage("No client with that ID found.");
        else if (l_target_client->m_pm_mute)
            sendServerMessage("That user is not recieving PMs.");
        else{
            QString l_message = argv.mid(1).join(" "); //...which means it will not end up as part of the
            if (m_is_gimped)
                l_message = ConfigManager::gimpList().at((genRand(1, ConfigManager::gimpList().size() - 1)));
            if (m_is_medieval || server->getAreaById(areaId())->isMedievalMode())
                l_message = server->getMedievalParser()->degrootify(l_message);
            if (m_is_shaken) {
                QStringList l_parts = l_message.split(QRegularExpression(R"([^A-Za-z0-9]+)"));

                std::random_device rng;
                std::mt19937 urng(rng());
                std::shuffle(l_parts.begin(), l_parts.end(), urng);

                l_message = l_parts.join(" ");
            }
            if (m_is_disemvoweled)
                l_message = QString(l_message).remove(QRegularExpression("[AEIOUaeiou]"));

            l_target_client->sendServerMessage("Message from " + name() + " (" + QString::number(clientId()) + "): " + l_message);
            sendServerMessage("PM sent to " + QString::number(l_target_id) + ". Message: " + l_message);
        }
    }
}

void AOClient::cmdAnnounce(int argc, QStringList argv)
{
    Q_UNUSED(argc);

    sendServerBroadcast("=== Announcement ===\r\n" + argv.join(" ") + "\r\n=============");
}

void AOClient::cmdM(int argc, QStringList argv)
{
    Q_UNUSED(argc);

    QString l_sender_name = name();
    QString l_sender_message = argv.join(" ");
    server->broadcast(PacketFactory::createPacket("CT", {"[M]" + l_sender_name, l_sender_message}), Server::TARGET_TYPE::MODCHAT);
}

void AOClient::cmdGM(int argc, QStringList argv)
{
    Q_UNUSED(argc);

    QString l_sender_name = name();
    QString l_sender_area = server->getAreaName(areaId());
    QString l_sender_message = argv.join(" ");
    server->broadcast(PacketFactory::createPacket("CT", {"[G][" + l_sender_area + "]" + "[" + l_sender_name + "][M]", l_sender_message}), Server::TARGET_TYPE::MODCHAT);
}

void AOClient::cmdLM(int argc, QStringList argv)
{
    Q_UNUSED(argc);

    QString l_sender_name = name();
    QString l_sender_message = argv.join(" ");
    server->broadcast(PacketFactory::createPacket("CT", {"[" + l_sender_name + "][M]", l_sender_message}), areaId());
}

void AOClient::cmdGimp(int argc, QStringList argv)
{
    Q_UNUSED(argc);

    bool conv_ok = false;
    int l_uid = argv[0].toInt(&conv_ok);
    if (!conv_ok) {
        sendServerMessage("Invalid user ID.");
        return;
    }

    AOClient *l_target = server->getClientByID(l_uid);

    if (l_target == nullptr) {
        sendServerMessage("No client with that ID found.");
        return;
    }

    if (l_target->m_is_gimped)
        sendServerMessage("That player is already gimped!");
    else {
        sendServerMessage("Gimped player.");
        l_target->sendServerMessage("You have been gimped! " + getReprimand());
    }
    l_target->m_is_gimped = true;
}

void AOClient::cmdUnGimp(int argc, QStringList argv)
{
    Q_UNUSED(argc);

    bool conv_ok = false;
    int l_uid = argv[0].toInt(&conv_ok);
    if (!conv_ok) {
        sendServerMessage("Invalid user ID.");
        return;
    }

    AOClient *l_target = server->getClientByID(l_uid);

    if (l_target == nullptr) {
        sendServerMessage("No client with that ID found.");
        return;
    }

    if (!(l_target->m_is_gimped))
        sendServerMessage("That player is not gimped!");
    else {
        sendServerMessage("Ungimped player.");
        l_target->sendServerMessage("A moderator has ungimped you! " + getReprimand(true));
    }
    l_target->m_is_gimped = false;
}

void AOClient::cmdDisemvowel(int argc, QStringList argv)
{
    Q_UNUSED(argc);

    bool conv_ok = false;
    int l_uid = argv[0].toInt(&conv_ok);
    if (!conv_ok) {
        sendServerMessage("Invalid user ID.");
        return;
    }

    AOClient *l_target = server->getClientByID(l_uid);

    if (l_target == nullptr) {
        sendServerMessage("No client with that ID found.");
        return;
    }

    if (l_target->m_is_disemvoweled)
        sendServerMessage("That player is already disemvoweled!");
    else {
        sendServerMessage("Disemvoweled player.");
        l_target->sendServerMessage("You have been disemvoweled! " + getReprimand());
    }
    l_target->m_is_disemvoweled = true;
}

void AOClient::cmdUnDisemvowel(int argc, QStringList argv)
{
    Q_UNUSED(argc);

    bool conv_ok = false;
    int l_uid = argv[0].toInt(&conv_ok);
    if (!conv_ok) {
        sendServerMessage("Invalid user ID.");
        return;
    }

    AOClient *l_target = server->getClientByID(l_uid);

    if (l_target == nullptr) {
        sendServerMessage("No client with that ID found.");
        return;
    }

    if (!(l_target->m_is_disemvoweled))
        sendServerMessage("That player is not disemvoweled!");
    else {
        sendServerMessage("Undisemvoweled player.");
        l_target->sendServerMessage("A moderator has undisemvoweled you! " + getReprimand(true));
    }
    l_target->m_is_disemvoweled = false;
}

void AOClient::cmdShake(int argc, QStringList argv)
{
    Q_UNUSED(argc);

    bool conv_ok = false;
    int l_uid = argv[0].toInt(&conv_ok);
    if (!conv_ok) {
        sendServerMessage("Invalid user ID.");
        return;
    }

    AOClient *l_target = server->getClientByID(l_uid);

    if (l_target == nullptr) {
        sendServerMessage("No client with that ID found.");
        return;
    }

    if (l_target->m_is_shaken)
        sendServerMessage("That player is already shaken!");
    else {
        sendServerMessage("Shook player.");
        l_target->sendServerMessage("A moderator has shaken your words! " + getReprimand());
    }
    l_target->m_is_shaken = true;
}

void AOClient::cmdUnShake(int argc, QStringList argv)
{
    Q_UNUSED(argc);

    bool conv_ok = false;
    int l_uid = argv[0].toInt(&conv_ok);
    if (!conv_ok) {
        sendServerMessage("Invalid user ID.");
        return;
    }

    AOClient *l_target = server->getClientByID(l_uid);

    if (l_target == nullptr) {
        sendServerMessage("No client with that ID found.");
        return;
    }

    if (!(l_target->m_is_shaken))
        sendServerMessage("That player is not shaken!");
    else {
        sendServerMessage("Unshook player.");
        l_target->sendServerMessage("A moderator has unshook you! " + getReprimand(true));
    }
    l_target->m_is_shaken = false;
}

void AOClient::cmdMedieval(int argc, QStringList argv)
{
    Q_UNUSED(argc);

    bool conv_ok = false;
    int l_uid = argv[0].toInt(&conv_ok);
    if (!conv_ok) {
        sendServerMessage("Invalid user ID.");
        return;
    }

    AOClient *l_target = server->getClientByID(l_uid);

    if (l_target == nullptr) {
        sendServerMessage("No client with that ID found.");
        return;
    }

    if (l_target->m_is_medieval)
        sendServerMessage("That player is already speaking Ye Olde English!");
    else {
        sendServerMessage("It is done, sire.");
        l_target->sendServerMessage("Forsooth! Thine speech will henceforth be Ye Olde!");
    }
    l_target->m_is_medieval = true;
}

void AOClient::cmdUnMedieval(int argc, QStringList argv)
{
    Q_UNUSED(argc);

    bool conv_ok = false;
    int l_uid = argv[0].toInt(&conv_ok);
    if (!conv_ok) {
        sendServerMessage("Invalid user ID.");
        return;
    }

    AOClient *l_target = server->getClientByID(l_uid);

    if (l_target == nullptr) {
        sendServerMessage("No client with that ID found.");
        return;
    }

    if (!(l_target->m_is_medieval))
        sendServerMessage("That player is not shaken!");
    else {
        sendServerMessage("Un-medieval'd player.");
        l_target->sendServerMessage("Hark! Thine speech hast been returneth to normal.");
    }
    l_target->m_is_medieval = false;
}

void AOClient::cmdMutePM(int argc, QStringList argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    m_pm_mute = !m_pm_mute;
    QString l_str_en = m_pm_mute ? "muted" : "unmuted";
    sendServerMessage("PM's are now " + l_str_en);
}

void AOClient::cmdToggleAdverts(int argc, QStringList argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    m_advert_enabled = !m_advert_enabled;
    QString l_str_en = m_advert_enabled ? "on" : "off";
    sendServerMessage("Advertisements turned " + l_str_en);
}

void AOClient::cmdToggleAfkMute(int argc, QStringList argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    m_afk_received = !m_afk_received;
    QString l_str_en = m_afk_received ? "received" : "not received";
    sendServerMessage("You are " + l_str_en + " AFK Messages.");
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
        auto current_area = server->getAreaById(areaId());
        for (const int client_id : current_area->joinedIDs()){
            auto l_client = QPointer<AOClient>(server->getClientByID(client_id));
            if (l_client.isNull())
                continue;

            if (l_client == this) /* "this" ... current client (aka user) lol */
                l_client->sendServerMessage("You are AFK, Have an nice day.");
            else if (!l_client->isSpectator() && l_client->m_afk_received) /* lgnored spectator for moment.. */
                l_client->sendServerMessage(QString("[%1] %2 are AFK.").arg(QString::number(clientId()), character().isEmpty() ? "Spectator" : character()));
        }
    }
    else
        sendServerMessage("You are AFK. (unannouncement)");
    ToggleAFK();
}

void AOClient::cmdCharCurse(int argc, QStringList argv)
{
    bool conv_ok = false;
    int l_uid = argv[0].toInt(&conv_ok);
    if (!conv_ok)
        sendServerMessage("Invalid user ID.");
    else{
        auto l_target = QPointer<AOClient>(server->getClientByID(l_uid));
        if (l_target.isNull())
            sendServerMessage("No client with that ID found.");
        else if (l_target->m_is_charcursed)
            sendServerMessage("That player is already charcursed!");
        else{
            switch (argc){
            case 1:
                l_target->m_charcurse_list.append(server->getCharID(l_target->character()));
                break;
            default:
                const QStringList l_char_names = argv.mid(1).join(" ").split(",");

                if (!l_target->m_charcurse_list.isEmpty())
                    l_target->m_charcurse_list.clear();

                for (auto target_char : l_char_names){
                    const int char_id = server->getCharID(target_char);
                    if (char_id == -1) {
                        sendServerMessage("Could not find character: " + target_char);
                        return;
                    }
                    l_target->m_charcurse_list.append(char_id);
                }
                break;
            }

            l_target->m_is_charcursed = true;

            // Kick back to char select screen
            if (!l_target->m_charcurse_list.contains(server->getCharID(l_target->character()))) {
                l_target->changeCharacter(-1);
                server->updateCharsTaken(server->getAreaById(areaId()));
                l_target->sendPacket("DONE");
            }
            else
                server->updateCharsTaken(server->getAreaById(areaId()));

            if (l_target == this)
                l_target->sendServerMessage("You have been charcursed yourself!");
            else{
                l_target->sendServerMessage("You have been charcursed!");
                sendServerMessage("Charcursed player.");
            }
        }
    }
}

void AOClient::cmdUnCharCurse(int argc, QStringList argv)
{
    Q_UNUSED(argc);

    bool conv_ok = false;
    int l_uid = argv[0].toInt(&conv_ok);
    if (!conv_ok)
        sendServerMessage("Invalid user ID.");
    else{
        auto l_target = QPointer<AOClient>(server->getClientByID(l_uid));
        if (l_target.isNull())
            sendServerMessage("No client with that ID found.");
        else if (!l_target->m_is_charcursed)
            sendServerMessage("That player is not charcursed!");
        else{
            l_target->m_is_charcursed = false;
            l_target->m_charcurse_list.clear();
            server->updateCharsTaken(server->getAreaById(areaId()));
            if (l_target == this)
                sendServerMessage("You were uncharcursed yourself.");
            else{
                sendServerMessage("Uncharcursed player.");
                l_target->sendServerMessage("You were uncharcursed.");
            }
        }
    }
}

void AOClient::cmdCharSelect(int argc, QStringList argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    changeCharacter(-1);
    sendPacket("DONE");
}

void AOClient::cmdForceCharSelect(int argc, QStringList argv)
{
    Q_UNUSED(argc);

    bool ok = false;
    int l_target_id = argv[0].toInt(&ok);
    if (!ok)
        sendServerMessage("This ID does not look valid. Please use the client ID.");
    else{
        auto l_target = QPointer<AOClient>(server->getClientByID(l_target_id));
        if (l_target.isNull())
            sendServerMessage("Unable to locate client with ID " + QString::number(l_target_id) + ".");
        else{
            l_target->changeCharacter(-1);
            l_target->sendPacket("DONE");
            sendServerMessage("Client has been forced into character select!");
        }
    }
}

void AOClient::cmdA(int argc, QStringList argv)
{
    Q_UNUSED(argc);

    bool ok;
    int l_area_id = argv[0].toInt(&ok);
    if (ok){
        const auto l_area = server->getAreaById(l_area_id);
        if (!l_area->owners().contains(clientId())) {
            sendServerMessage("You are not CM in that area.");
            return;
        }
        QPair<const QString, QString> l_sender = qMakePair(name(), argv.mid(1).join(" "));
        if (m_is_gimped)
            l_sender.second = ConfigManager::gimpList().at((genRand(1, ConfigManager::gimpList().size() - 1)));
        if (m_is_medieval || l_area->isMedievalMode())
            l_sender.second = server->getMedievalParser()->degrootify(l_sender.second);
        if (m_is_shaken) {
            QStringList l_parts = l_sender.second.split(QRegularExpression(R"([^A-Za-z0-9]+)"));

            std::random_device rng;
            std::mt19937 urng(rng());
            std::shuffle(l_parts.begin(), l_parts.end(), urng);

            l_sender.second = l_parts.join(" ");
        }
        if (m_is_disemvoweled)
            l_sender.second = QString(l_sender.second).remove(QRegularExpression("[AEIOUaeiou]"));

        server->broadcast(PacketFactory::createPacket("CT", {"[CM]" + l_sender.first, l_sender.second}), l_area->index());
    }
    else
        sendServerMessage("This does not look like a valid AreaID.");
}

void AOClient::cmdS(int argc, QStringList argv)
{
    Q_UNUSED(argc);

    QPair<const QString, QString> l_sender = qMakePair(name(), argv.join(" "));
    if (m_is_gimped)
        l_sender.second = ConfigManager::gimpList().at((genRand(1, ConfigManager::gimpList().size() - 1)));
    if (m_is_medieval)
        l_sender.second = server->getMedievalParser()->degrootify(l_sender.second);
    if (m_is_shaken) {
        QStringList l_parts = l_sender.second.split(QRegularExpression(R"([^A-Za-z0-9]+)"));

        std::random_device rng;
        std::mt19937 urng(rng());
        std::shuffle(l_parts.begin(), l_parts.end(), urng);

        l_sender.second = l_parts.join(" ");
    }
    if (m_is_disemvoweled)
        l_sender.second = QString(l_sender.second).remove(QRegularExpression("[AEIOUaeiou]"));

    for (auto Area : server->getAreas()){
        if (Area->owners().contains(clientId()))
            server->broadcast(PacketFactory::createPacket("CT", {"[CM]" + l_sender.first, l_sender.second}), Area->index());
    }
}

void AOClient::cmdFirstPerson(int argc, QStringList argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    m_first_person = !m_first_person;
    QString l_str_en = m_first_person ? "enabled" : "disabled";
    sendServerMessage("First person mode " + l_str_en + ".");
}
