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

    sendServerMessageArea(QString("[%1] %2 filpped a coin and got %3.").arg(QString::number(clientId()), character().isEmpty() ? name() : character(), QStringList({"heads", "tails"})[AOClient::genRand(0, 2)]));
}

void AOClient::cmdRoll(int argc, QStringList argv){
    switch (argc){
    case 0:
        diceThrower(6, 1);
        break;
    case 1:
        if (argv[0].toLower().contains(QRegularExpression("(\\d+)([+-])(\\d+)d(\\d+)"))){ // check if modifier persent, like "10+2d6", "3-1d20"..
            const auto Matcher = QRegularExpression("(\\d+)([+-])(\\d+)d(\\d+)").match(argv[0].toLower());
            diceThrower(Matcher.captured(4).toInt(), Matcher.captured(1).toInt(), Matcher.captured(2) == "+" ? Matcher.captured(3).toInt() : -Matcher.captured(3).toInt());
        }
        else if (argv[0].toLower().contains(QRegularExpression("(\\d+)d(\\d+)"))){ // plain XdY mostly like "2d6", "1d20"..
            const auto Matcher = QRegularExpression("(\\d+)d(\\d+)").match(argv[0].toLower());
            diceThrower(Matcher.captured(2).toInt(), Matcher.captured(1).toInt());
        }
        else if (argv[0].toLower().contains(QRegularExpression("d(\\d+)"))){ // single die be like "d6", "d20"..
            const auto Matcher = QRegularExpression("d(\\d+)").match(argv[0].toLower());
            diceThrower(Matcher.captured(1).toInt(), 1);
        }
        else{
            bool sides_ok;
            const int sides = argv[0].toInt(&sides_ok);
            sides_ok ? diceThrower(sides, 1) : sendServerMessage("Invalid dice param.");
        }
        break;
    default:
        QPair<bool, bool> roll_ok;
        const QPair<int, int> rolldice({argv[0].toInt(&roll_ok.first), argv[1].toInt(&roll_ok.second)});

        if (roll_ok.first && roll_ok.second)
            diceThrower(rolldice.first, rolldice.second);
        else
            sendServerMessage("Invalid dice param.");
        break;
    }
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

void AOClient::cmdRollP(int argc, QStringList argv){
    switch (argc){
    case 0:
        diceThrower(6, 1, 0, true);
        break;
    case 1:
        if (argv[0].toLower().contains(QRegularExpression("(\\d+)([+-])(\\d+)d(\\d+)"))){ // check if modifier persent, like "10+2d6", "3-1d20"..
            const auto Matcher = QRegularExpression("(\\d+)([+-])(\\d+)d(\\d+)").match(argv[0].toLower());
            diceThrower(Matcher.captured(4).toInt(), Matcher.captured(1).toInt(), Matcher.captured(2) == "+" ? Matcher.captured(3).toInt() : -Matcher.captured(3).toInt(), true);
        }
        else if (argv[0].toLower().contains(QRegularExpression("(\\d+)d(\\d+)"))){ // plain XdY mostly like "2d6", "1d20"..
            const auto Matcher = QRegularExpression("(\\d+)d(\\d+)").match(argv[0].toLower());
            diceThrower(Matcher.captured(2).toInt(), Matcher.captured(1).toInt(), 0 , true);
        }
        else if (argv[0].toLower().contains(QRegularExpression("d(\\d+)"))){ // single die be like "d6", "d20"..
            const auto Matcher = QRegularExpression("d(\\d+)").match(argv[0].toLower());
            diceThrower(Matcher.captured(1).toInt(), 1, 0, true);
        }
        else{
            bool sides_ok;
            const int sides = argv[0].toInt(&sides_ok);
            sides_ok ? diceThrower(sides, 1, 0, true) : sendServerMessage("Invalid dice param.");
        }
        break;
    default:
        QPair<bool, bool> roll_ok;
        const QPair<int, int> rolldice({argv[0].toInt(&roll_ok.first), argv[1].toInt(&roll_ok.second)});

        if (roll_ok.first && roll_ok.second)
            diceThrower(rolldice.first, rolldice.second, 0, true);
        else
            sendServerMessage("Invalid dice param.");
        break;
    }
}

void AOClient::cmdWheel(int argc, QStringList argv)
{
    switch (argc) {
    case 0:
        sendServerMessage("Usage: /wheel <argument 1> <argument 2> ... up to 20");
        break;
    default:
        if (argc > 20) {
            sendServerMessage("You entered more than 20 arguments.");
        }
        else {
            int l_rand = genRand(0, argc - 1);
            QString sender_name = character().isEmpty() ? m_ooc_name : character();
            sendServerMessageArea(QString("[%1] %2 spun a wheel and got %3.")
                .arg(QString::number(clientId()), sender_name, argv[l_rand]));
        }
        break;
    }
}

void AOClient::cmdWheelP(int argc, QStringList argv)
{
    switch (argc) {
    case 0:
        sendServerMessage("Usage: /wheelp <argument 1> <argument 2> ... up to 20");
        break;
    default:
        if (argc > 20) {
            sendServerMessage("You entered more than 20 arguments.");
        }
        else {
            int l_rand = genRand(0, argc - 1);
            sendServerMessage(QString("You spun a wheel and got %1.").arg(argv[l_rand]));
        }
        break;
    }
}

void AOClient::cmdRps(int argc, QStringList argv){
    auto l_area = server->getAreaById(areaId());
    if (l_area.isNull())
        return;

    switch (argc){
    case 1:
        if (argv[0].compare("cancel", Qt::CaseInsensitive) == 0){
            switch (l_area->GetRPSFighter().first){
            case -1:
                sendServerMessage("There is nothing game goes on.", "[Rock-Paper-Scissors]");
                break;
            default:
                if (l_area->GetRPSFighter().first == clientId()){
                    sendServerMessageArea(QString("🥀 %1 canceling the game 🥀").arg("[" + QString::number(clientId()) + "] " + name()), "[Rock-Paper-Scissors]");
                    l_area->SetRPSFighter(-1);
                }
                else
                    sendServerMessage("You cannot canceling the current game.", "[Rock-Paper-Scissors]");
                break;
            }
        }
        else if (argv[0].compare("rock", Qt::CaseInsensitive) == 0 || argv[0].compare("paper", Qt::CaseInsensitive) == 0 || argv[0].compare("scissors", Qt::CaseInsensitive) == 0){
            const QString l_choice = argv[0].toLower(), user_challenger("[" + QString::number(clientId()) + "] " + name());
            const QMap<QString, QString> choiceToEmote = {{"rock", "🪨",}, {"paper", "🗞"}, {"scissors", "✂️"}};

            switch (l_area->GetRPSFighter().first){
            case -1: // setup
                sendServerMessage(QString("You chose %1!").arg(l_choice), "[Rock-Paper-Scissors]");
                l_area->SetRPSFighter(clientId(), l_choice);
                sendServerMessageArea(QString("⚔️ %1 wants to play the game, Use /rps [choice] to play against them ⚔️").arg(user_challenger), "[Rock-Paper-Scissors]");
                break;
            default: // id were setuped..
                if (l_area->GetRPSFighter().first == clientId())
                    sendServerMessage("You cannot against yourself!", "[Rock-Paper-Scissors]");
                else if (server->getClientByID(l_area->GetRPSFighter().first).isNull()){ // forced cancel if challenger client id are "null"..
                    l_area->SetRPSFighter(-1);
                    sendServerMessageArea("🥀 The game are forced cancelled (due of the challenger client not longer exist) 🥀", "[Rock-Paper-Scissors]");
                }
                else{ // Accept challenge and decide winner
                    const auto fighter_client = server->getClientByID(l_area->GetRPSFighter().first);
                    const QString fighter_name("[" + QString::number(fighter_client->clientId()) + "] " + fighter_client->name());

                    // Announce results
                    sendServerMessageArea(QString("%1 (%2) ⚔️ (%4) %3.").arg(fighter_name, choiceToEmote[l_area->GetRPSFighter().second], user_challenger, choiceToEmote[l_choice]), "[Rock-Paper-Scissors]");
                    sendServerMessage(l_area->GetRPSFighter().second == l_choice ? "👔 It's a tie 👔" : QString("👑 %1 winner(s) 👑").arg((l_choice == "rock" && l_area->GetRPSFighter().second == "scissors") || (l_choice == "paper" && l_area->GetRPSFighter().second == "rock") || (l_choice == "scissors" && l_area->GetRPSFighter().second == "paper") ? user_challenger : fighter_name));
                    l_area->SetRPSFighter(-1);
                }
                break;

            }
        }
        else
            sendServerMessage("Invalid choice. Please choose rock, paper, or scissors.");
        break;
    default:
        sendServerMessage("Usage: /rps [rock/paper/scissors]");
        break;
    }
}

void AOClient::cmdTimer(int argc, QStringList argv){
    Q_UNUSED(argc)

    auto l_area = server->getAreaById(areaId());
    if (l_area.isNull())
        return;

    if (argv.isEmpty()){ // Called without arguments, Shows a brief of all timers.
        QStringList l_timers;
        l_timers.append("Currently active timers:");
        for (int i = 0; i <= 4; i++)
            l_timers.append(getAreaTimer(l_area->index(), i));
        sendServerMessage(l_timers.join("\n"));
    }
    else{ // Called with more than one argument
        bool ok;
        const int l_timer_id = argv[0].toInt(&ok);

        if (ok){
            if (argv.size() == 1) // Called with one argument, Shows the status of one timer
                sendServerMessage(getAreaTimer(l_area->index(), l_timer_id));
            else if (!checkPermission(ACLRole::CM)) // only if user is [CM] or [Moderator] (had CM perms) can using this params.
                sendServerMessage(isMAuthenticated() ? "You do not have permission to use that param." : "You must become CM to use that param.");
            else{ // Called with more than one argument, Updates the state of a timer..
                QPointer<QTimer> l_requested_timer; // just in case..

                switch (l_timer_id){// Select the proper timer..
                case 0: // Check against permissions if global timer is selected
                    if (checkPermission(ACLRole::GLOBAL_TIMER))
                        l_requested_timer = server->timer;
                    else{
                        sendServerMessage("You are not authorized to alter the global timer.");
                        return;
                    }
                    break;
                case 1: case 2: case 3: case 4:
                    l_requested_timer = l_area->timers().at(l_timer_id - 1);
                    break;
                default:
                    sendServerMessage("Invalid timer ID. Timer ID must be a whole number between 0 and 4.");
                    return;
                }

                if (l_requested_timer.isNull())
                    sendServerMessage("Cannot calling that timer.");
                else{
                    AOPacket *l_show_timer = PacketFactory::createPacket("TI", {QString::number(l_timer_id), "2"});
                    AOPacket *l_hide_timer = PacketFactory::createPacket("TI", {QString::number(l_timer_id), "3"});
                    const bool l_is_global = l_requested_timer == server->timer;

                    /* > Set the timer's time remaining if the second < */
                    const long long l_requested_time = CalendarParse(argv[1], true);
                    if (l_requested_time > 0){ // argument is a valid time
                        if (std::chrono::duration_cast<std::chrono::hours>(std::chrono::milliseconds(l_requested_time)).count() >= 25) // qtimer limted..
                            sendServerMessage(QString("Cannot set timer ID %1 than max 24 hours.").arg(QString::number(l_timer_id)));
                        else{
                            l_requested_timer->setInterval(l_requested_time);
                            sendServerMessage("Set timer " + QString::number(l_timer_id) + " to " + EpochToString(std::chrono::milliseconds(l_requested_time), true) + ".");
                            AOPacket *l_update_timer = PacketFactory::createPacket("TI", {QString::number(l_timer_id), "0", QString::number(l_requested_time)});
                            l_is_global ? server->broadcast(l_show_timer) : sendServerPacketArea(l_show_timer); // Show the timer
                            l_is_global ? server->broadcast(l_update_timer) : sendServerPacketArea(l_update_timer);
                        }
                    } // Otherwise, update the state of the timer
                    else if (argv[1].compare("start", Qt::CaseInsensitive) == 0){
                        l_requested_timer->start();
                        sendServerMessage("Started timer " + QString::number(l_timer_id) + ".");
                        AOPacket *l_update_timer = PacketFactory::createPacket("TI", {QString::number(l_timer_id), "0", QString::number(l_requested_timer->remainingTimeAsDuration().count())});
                        l_is_global ? server->broadcast(l_show_timer) : sendServerPacketArea(l_show_timer);
                        l_is_global ? server->broadcast(l_update_timer) : sendServerPacketArea(l_update_timer);
                    }
                    else if (argv[1].compare("pause", Qt::CaseInsensitive) == 0 || argv[1].compare("stop", Qt::CaseInsensitive) == 0){
                        l_requested_timer->setInterval(l_requested_timer->remainingTime());
                        l_requested_timer->stop();
                        sendServerMessage("Stopped timer " + QString::number(l_timer_id) + ".");
                        AOPacket *l_update_timer = PacketFactory::createPacket("TI", {QString::number(l_timer_id), "1", QString::number(l_requested_timer->interval())});
                        l_is_global ? server->broadcast(l_update_timer) : sendServerPacketArea(l_update_timer);
                    }
                    else if (argv[1].compare("hide", Qt::CaseInsensitive) == 0 || argv[1].compare("unset", Qt::CaseInsensitive) == 0){ // Hide the timer
                        l_requested_timer->setInterval(0);
                        l_requested_timer->stop();
                        sendServerMessage("Hid timer " + QString::number(l_timer_id) + ".");
                        l_is_global ? server->broadcast(l_hide_timer) : sendServerPacketArea(l_hide_timer);
                    }
                    else
                        sendServerMessage("Invalid param of timer type");
                }
            }
        }
        else
            sendServerMessage("Invalid timer ID. Timer ID must be a whole number between 0 and 4.");
    }
}

void AOClient::cmdNoteCard(int argc, QStringList argv){
    Q_UNUSED(argc);

    auto l_area = server->getAreaById(areaId());
    if (l_area.isNull())
        return;

    QString l_notecard = argv.join(" ");
    l_area->addNotecard(QString(character().isEmpty() ? "[Spectator]" : character()), l_notecard);
    sendServerMessageArea("[" + QString::number(clientId()) + "] " + QString(character().isEmpty() ? "[Spectator]" : character()) + " wrote a note card.");
}

void AOClient::cmdNoteCardClear(int argc, QStringList argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    auto l_area = server->getAreaById(areaId());
    if (l_area.isNull())
        return;

    if (!l_area->addNotecard(QString(character().isEmpty() ? "[Spectator]" : character()), QString()))
        sendServerMessageArea("[" + QString::number(clientId()) + "] " + QString(character().isEmpty() ? "[Spectator]" : character()) + " erased their note card.");
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
