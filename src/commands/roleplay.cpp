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

// This file is for commands under the roleplay category in aoclient.h
// Be sure to register the command in the header before adding it here!

void AOClient::cmdFlip(int argc, QStringList argv){
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    sendServerMessageArea(QString("[%1] %2 filpped a coin and got %3.").arg(QString::number(clientId()), character().isEmpty() ? name() : character(), QStringList({"heads", "tails"})[AOClient::genRand(0, 1)]));
}

void AOClient::cmdRoll(int argc, QStringList argv)
{
    int l_sides = 6;
    int l_dice = 1;
    QStringList results;

    if (argc >= 1) {
        if (argv[0].contains('d')) {
            QStringList l_arguments = argv[0].split('d');

            bool l_dice_ok;
            bool l_sides_ok;
            l_dice = l_arguments[0].toInt(&l_dice_ok);
            l_sides = l_arguments[1].toInt(&l_sides_ok);

            if (argv[0].contains('+')) {
                bool l_mod_ok;
                QStringList l_modifier = l_arguments[1].split('+');
                int modifier = l_modifier[1].toInt(&l_mod_ok);
                l_sides = l_modifier[0].toInt(&l_sides_ok);

                if (l_mod_ok && l_dice_ok && l_sides_ok)
                    diceThrower(l_sides, l_dice, false, modifier);
                else
                    sendServerMessage("Invalid dice notation.");
                return;
            }
            else if (argv[0].contains('-')) {
                bool l_mod_ok;
                QStringList l_modifier = l_arguments[1].split('-');
                int modifier = l_modifier[1].toInt(&l_mod_ok);
                l_sides = l_modifier[0].toInt(&l_sides_ok);

                if (l_mod_ok && l_dice_ok && l_sides_ok)
                    diceThrower(l_sides, l_dice, false, -modifier);
                else
                    sendServerMessage("Invalid dice notation.");
                return;
            }
            else if (l_dice_ok && l_sides_ok) {
                diceThrower(l_sides, l_dice, false);
                return;
            }
            else {
                sendServerMessage("Invalid dice notation.");
                return;
            }
        }
        else
            l_sides = qBound(1, argv[0].toInt(), ConfigManager::diceMaxValue());
    }
    if (argc == 2)
        l_dice = qBound(1, argv[1].toInt(), ConfigManager::diceMaxDice());
    diceThrower(l_sides, l_dice, false);
}

void AOClient::cmdRollA(int argc, QStringList argv)
{
    Q_UNUSED(argc);

    const QString l_dice_name = argv.join(" ");

    if (ConfigManager::diceFaces(l_dice_name).isEmpty()) {
        qWarning() << "Unknown dice.";
        sendServerMessage("Unknown dice.");
    }
    else
        sendServerMessageArea("[" + QString::number(clientId()) + "] " + QString(character().isEmpty() ? name() : character()) + " rolled from the \"" + l_dice_name + "\" set and got: " + ConfigManager::diceFaces(l_dice_name).at((genRand(0, ConfigManager::diceFaces(l_dice_name).size() - 1))));
}

void AOClient::cmdRollP(int argc, QStringList argv)
{
    int l_sides = 6;
    int l_dice = 1;
    QStringList results;

    if (argc >= 1) {
        if (argv[0].contains('d')) {
            QStringList l_arguments = argv[0].split('d');

            bool l_dice_ok;
            bool l_sides_ok;
            l_dice = l_arguments[0].toInt(&l_dice_ok);
            l_sides = l_arguments[1].toInt(&l_sides_ok);

            if (argv[0].contains('+')) {
                bool l_mod_ok;
                QStringList l_modifier = l_arguments[1].split('+');
                int modifier = l_modifier[1].toInt(&l_mod_ok);
                l_sides = l_modifier[0].toInt(&l_sides_ok);

                if (l_mod_ok && l_dice_ok && l_sides_ok)
                    diceThrower(l_sides, l_dice, true, modifier);
                else
                    sendServerMessage("Invalid dice notation.");
                return;
            }
            else if (argv[0].contains('-')) {
                bool l_mod_ok;
                QStringList l_modifier = l_arguments[1].split('-');
                int modifier = l_modifier[1].toInt(&l_mod_ok);
                l_sides = l_modifier[0].toInt(&l_sides_ok);

                if (l_mod_ok && l_dice_ok && l_sides_ok)
                    diceThrower(l_sides, l_dice, true, -modifier);
                else
                    sendServerMessage("Invalid dice notation.");
                return;
            }
            else if (l_dice_ok && l_sides_ok) {
                diceThrower(l_sides, l_dice, true);
                return;
            }
            else {
                sendServerMessage("Invalid dice notation.");
                return;
            }
        }
        else
            l_sides = qBound(1, argv[0].toInt(), ConfigManager::diceMaxValue());
    }
    if (argc == 2)
        l_dice = qBound(1, argv[1].toInt(), ConfigManager::diceMaxDice());
    diceThrower(l_sides, l_dice, true);
}

void AOClient::cmdWheel(int argc, QStringList argv)
{
    if (argc < 1){
        sendServerMessage("Usage: /wheel <argument 1> <argument 2>... up to 20");
    }
    else if(argc > 20){
        sendServerMessage("You entered more than 20 arguments...");
    }
    else {
        int l_rand = genRand(0, argv.length() - 1);
        QString l_result = QString("%1 spinned a wheel and got %2").arg(m_ooc_name.isEmpty() ? m_ooc_name : m_current_char, argv[l_rand]);
        sendServerMessageArea(l_result);
    }
}

void AOClient::cmdWheelP(int argc, QStringList argv)
{
    if (argc < 1){
        sendServerMessage("Usage: /wheelp <argument 1> <argument 2>... up to 20");
    }
    else if(argc > 20){
        sendServerMessage("You entered more than 20 arguments...");
    }
    else {
        int l_rand = genRand(0, argv.length() - 1);
        QString l_result = QString("You spinned a wheel and got %1").arg(argv[l_rand]);
        sendServerMessage(l_result);
    }
}

void AOClient::cmdRps(int argc, QStringList argv){
    if (argc == 1){
        if (argv[0].compare("rock", Qt::CaseInsensitive) == 0 || argv[0].compare("paper", Qt::CaseInsensitive) == 0 || argv[0].compare("scissors", Qt::CaseInsensitive) == 0){
            const QString l_choice = argv[0].toLower(), user_challenger("[" + QString::number(clientId()) + "] " + name());
            const QMap<QString, QString> choiceToEmote = {{"rock", "🪨",}, {"paper", "🗞"}, {"scissors", "✂️"}};

            // Check if there's active game
            AOClient *challenger = nullptr;
            for (auto client : server->getClients()){
                if (client->rps_waiting && client != this){
                    challenger = client;
                    break;
                }
            }

            if (challenger){ // Accept challenge and decide winner
                const QString Challenger_name("[" + QString::number(challenger->clientId()) + "] " + challenger->name());
                QString result;
                if (challenger->rps_choice == l_choice)
                    result = "👔It's a tie👔";
                else if ((l_choice == "rock" && challenger->rps_choice == "scissors") ||
                         (l_choice == "paper" && challenger->rps_choice == "rock") ||
                         (l_choice == "scissors" && challenger->rps_choice == "paper"))
                    result = "👑 " + user_challenger + " wins👑";
                else
                    result = "👑 " + Challenger_name + " wins👑";

                // Announce results
                sendServerPacketArea(PacketFactory::createPacket("CT", {"[Rock Paper Scissors]", QString("%1 (%2) ⚔️ (%4) %3.").arg(Challenger_name, choiceToEmote[challenger->rps_choice], user_challenger, choiceToEmote[l_choice]), "1"}));
                sendServerPacketArea(PacketFactory::createPacket("CT", {"[Rock Paper Scissors]", result, "1"}));

                // Reset state
                challenger->rps_waiting = false;
                challenger->rps_choice.clear();
            }
            else{ // Start new game if there's no challenger
                rps_choice = l_choice;
                sendServerMessage("You chose " + l_choice + "!");
                rps_waiting = true;
                sendServerMessageArea("⚔️ " + user_challenger + " wants to play Rock Paper Scissors! Use /rps [choice] to play against them⚔️");

                QTimer::singleShot(120000, this, [this]() {
                    if (rps_waiting) {
                        rps_waiting = false;
                        rps_choice.clear();
                        sendServerMessage("🥀Rock Paper Scissors challenge expired🥀");
                    }
                });
            }
        }
        else
            sendServerMessage("Invalid choice. Please choose rock, paper, or scissors.");
    }
    else
        sendServerMessage("Usage: /rps [rock/paper/scissors]");
}

void AOClient::cmdTimer(int argc, QStringList argv)
{
    auto l_area = server->getAreaById(areaId());
    if (l_area.isNull())
        return;

    // Called without arguments
    // Shows a brief of all timers
    if (argc == 0) {
        QStringList l_timers;
        l_timers.append("Currently active timers:");
        for (int i = 0; i <= 4; i++) {
            l_timers.append(getAreaTimer(l_area->index(), i));
        }
        sendServerMessage(l_timers.join("\n"));
        return;
    }

    // Called with more than one argument
    bool ok;
    int l_timer_id = argv[0].toInt(&ok);
    if (!ok || l_timer_id < 0 || l_timer_id > 4) {
        sendServerMessage("Invalid timer ID. Timer ID must be a whole number between 0 and 4.");
        return;
    }

    // Called with one argument
    // Shows the status of one timer
    if (argc == 1) {
        sendServerMessage(getAreaTimer(l_area->index(), l_timer_id));
        return;
    }

    // Called with more than one argument
    // Updates the state of a timer

    // Select the proper timer
    // Check against permissions if global timer is selected
    QTimer *l_requested_timer;
    if (l_timer_id == 0) {
        if (!checkPermission(ACLRole::GLOBAL_TIMER)) {
            sendServerMessage("You are not authorized to alter the global timer.");
            return;
        }
        l_requested_timer = server->timer;
    }
    else
        l_requested_timer = l_area->timers().at(l_timer_id - 1);

    AOPacket *l_show_timer = PacketFactory::createPacket("TI", {QString::number(l_timer_id), "2"});
    AOPacket *l_hide_timer = PacketFactory::createPacket("TI", {QString::number(l_timer_id), "3"});
    bool l_is_global = l_timer_id == 0;

    // Set the timer's time remaining if the second
    // argument is a valid time
    QTime l_requested_time = QTime::fromString(argv[1], "hh:mm:ss");
    if (l_requested_time.isValid()) {
        l_requested_timer->setInterval(QTime(0, 0).msecsTo(l_requested_time));
        l_requested_timer->start();
        sendServerMessage("Set timer " + QString::number(l_timer_id) + " to " + argv[1] + ".");
        AOPacket *l_update_timer = PacketFactory::createPacket("TI", {QString::number(l_timer_id), "0", QString::number(QTime(0, 0).msecsTo(l_requested_time))});
        l_is_global ? server->broadcast(l_show_timer) : sendServerPacketArea(l_show_timer); // Show the timer
        l_is_global ? server->broadcast(l_update_timer) : sendServerPacketArea(l_update_timer);
        return;
    }
    // Otherwise, update the state of the timer
    else {
        if (argv[1] == "start") {
            l_requested_timer->start();
            sendServerMessage("Started timer " + QString::number(l_timer_id) + ".");
            AOPacket *l_update_timer = PacketFactory::createPacket("TI", {QString::number(l_timer_id), "0", QString::number(QTime(0, 0).msecsTo(QTime(0, 0).addMSecs(l_requested_timer->remainingTime())))});
            l_is_global ? server->broadcast(l_show_timer) : sendServerPacketArea(l_show_timer);
            l_is_global ? server->broadcast(l_update_timer) : sendServerPacketArea(l_update_timer);
        }
        else if (argv[1] == "pause" || argv[1] == "stop") {
            l_requested_timer->setInterval(l_requested_timer->remainingTime());
            l_requested_timer->stop();
            sendServerMessage("Stopped timer " + QString::number(l_timer_id) + ".");
            AOPacket *l_update_timer = PacketFactory::createPacket("TI", {QString::number(l_timer_id), "1", QString::number(QTime(0, 0).msecsTo(QTime(0, 0).addMSecs(l_requested_timer->interval())))});
            l_is_global ? server->broadcast(l_update_timer) : sendServerPacketArea(l_update_timer);
        }
        else if (argv[1] == "hide" || argv[1] == "unset") {
            l_requested_timer->setInterval(0);
            l_requested_timer->stop();
            sendServerMessage("Hid timer " + QString::number(l_timer_id) + ".");
            // Hide the timer
            l_is_global ? server->broadcast(l_hide_timer) : sendServerPacketArea(l_hide_timer);
        }
    }
}

void AOClient::cmdNoteCard(int argc, QStringList argv){
    Q_UNUSED(argc);

    auto l_area = server->getAreaById(areaId());
    if (l_area.isNull())
        return;

    QString l_notecard = argv.join(" ");
    l_area->addNotecard(QString(character().isEmpty() ? "Spectator" : character()), l_notecard);
    sendServerMessageArea("[" + QString::number(clientId()) + "] " + QString(character().isEmpty() ? "Spectator" : character()) + " wrote a note card.");
}

void AOClient::cmdNoteCardClear(int argc, QStringList argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    auto l_area = server->getAreaById(areaId());
    if (l_area.isNull())
        return;

    if (!l_area->addNotecard(QString(character().isEmpty() ? "Spectator" : character()), QString()))
        sendServerMessageArea("[" + QString::number(clientId()) + "] " + QString(character().isEmpty() ? "Spectator" : character()) + " erased their note card.");
}

void AOClient::cmdNoteCardReveal(int argc, QStringList argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    auto l_area = server->getAreaById(areaId());
    if (l_area.isNull())
        return;

    const QStringList l_notecards = l_area->getNotecards();

    if (l_notecards.isEmpty())
        sendServerMessage("There are no cards to reveal in this area.");
    else
        sendServerMessageArea("Note cards have been revealed.\n · " + l_notecards.join("\n · "));
}

void AOClient::cmd8Ball(int argc, QStringList argv){
    Q_UNUSED(argc);

    if (ConfigManager::magic8BallAnswers().isEmpty()) {
        qWarning().noquote().nospace() << "An client id " << clientId() << "tried to using /8ball but 8ball.txt is empty!";
        sendServerMessage("8ball are unavailable due of 8ball-list (aka 8ball.txt) empty.");
    }
    else{
        const QString l_sender_message = argv.join(" ");
        sendServerMessageArea("[" + QString::number(clientId()) + "] " + QString(character().isEmpty() ? name() : character()) + " asked the magic 8-ball, \"" + l_sender_message + "\" and the answer is: " + ConfigManager::magic8BallAnswers().at((genRand(1, ConfigManager::magic8BallAnswers().size() - 1))));
    }
}

void AOClient::cmdSubTheme(int argc, QStringList argv)
{
    Q_UNUSED(argc);

    QString l_subtheme = argv.join(" ");
    auto l_area = server->getAreaById(areaId());
    if (l_area.isNull())
        return;

    for (int Index : l_area->joinedIDs()){
        auto client = server->getClientByID(Index);
        if (client.isNull())
            continue;
        client->sendPacket("ST", {l_subtheme, "1"});
    }
    sendServerMessageArea("Subtheme was set to " + l_subtheme);
}
