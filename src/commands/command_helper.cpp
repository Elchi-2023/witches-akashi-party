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

// This file is for functions used by various commands, defined in the command helper function category in aoclient.h
// Be sure to register the command in the header before adding it here!

void AOClient::cmdDefault(int argc, QStringList argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    sendServerMessage("Invalid command.");
    return;
}

QStringList AOClient::buildAreaList(int area_idx)
{
    QStringList entries;
    const auto area = server->getAreaById(area_idx);
    const bool ld_state = server->isLockdownState();
    if (!area.isNull()){ // only valid area we needs..
        QStringList title{"[" + QString::number(area_idx) + "]", area->name()};
        const auto current_type = m_version.type;
        if (area->lockStatus() > AreaData::LockStatus::FREE)
            title[0].prepend(current_type == ClientVersion::ClientType::NDS ? QStringList({"[LOCKED]", "[SPECT]"})[area->lockStatus() -1] : QStringList({"[🔒]", "[🔐👁]"})[area->lockStatus() -1]);
        if (area->playerCount() > 0)
            current_type == ClientVersion::ClientType::NDS ? title.append("[P: " + QString::number(area->playerCount()) + "]") : title.append("[👥: " + QString::number(area->playerCount()) + "]");
        if (area->status() > AreaData::Status::IDLE)
            current_type == ClientVersion::ClientType::NDS ? title.append(QStringList({"[RP]", "[CASING]", "[LFP]", "[RECESS]", "[GAMING]"})[area->status() -1]) : title.append(QStringList({"[🎭]", "[💼]", "[🔍]", "[⏳]", "[🎲]"})[area->status() -1]);
        entries.append("=== " + title.join(" ") + " ===");
        const auto current_vmap = area->GetRegisteredVoiceMap();

        for (int Index : area->joinedIDs()){
            const auto client = server->getClientByID(Index);
            if (!client.isNull()){
                QStringList Entry("[" + QString::number(client->clientId()) + "] ");
                Entry.append(client->isSpectator() ? "[Spectator]" : client->character());
                if (!client->characterName().isEmpty())
                    Entry.replace(Entry.size() -1, Entry.last() + " (" + client->characterName() + ")");
                if (client->isVAuthenticated())
                    Entry.prepend(area->owners().contains(client->clientId()) ? "[V-CM]" : "[VIP]");
                else if (area->owners().contains(client->clientId()))
                    Entry.prepend("[CM]");

                switch (client->m_version.type){
                case ClientVersion::ClientType::NORMAL: default:
                    if (current_type == ClientVersion::ClientType::NDS)
                        Entry.prepend("[CLIENT]");
                    break;
                case ClientVersion::ClientType::WEBAO:
                    current_type == ClientVersion::ClientType::NDS ? Entry.prepend("[WEB]") : Entry.prepend("[🌐]");
                    break;
                case ClientVersion::ClientType::WEBAOPHONE:
                    current_type == ClientVersion::ClientType::NDS ? Entry.prepend("[WEB:PHONE]") : Entry.prepend("[🌐📱]");
                    break;
                case ClientVersion::ClientType::NDS:
                    current_type == ClientVersion::ClientType::NDS ? Entry.prepend("[(NDS)]") : Entry.prepend("[🎮(NDS)]");
                    break;
                }

                if (client->UserAFK())
                    Entry.prepend(current_type == ClientVersion::ClientType::NDS ? "[AFK]" : "[💤]");
                if (current_vmap.contains(Index))
                    Entry.prepend(current_type == ClientVersion::ClientType::NDS ? "[VC] " : QStringList({"[🔉] ", "[🎙] "})[current_vmap[Index]]);
                current_type == ClientVersion::ClientType::NDS ? Entry.prepend(client == this ? "[YOU]" : " * ") : Entry.prepend(client == this ? " ➤ " : " · ");
                if (!client->m_pos.isEmpty())
                    Entry.append(" <" + client->m_pos + ">");

                switch (m_authenticated_type){
                case AOClient::AuthenticateType::NONE:
                    if (area->owners().contains(clientId()) && !client->name().isEmpty()) /* > if user is [CM] and in current area can see ooc-name (only if user in the area) < */
                        Entry.append(" : " + client->name());
                    break;
                case AOClient::AuthenticateType::VIP:
                    if (!client->name().isEmpty()) /* > [VIP] can see ooc-name cause.. why not?.. < */
                        Entry.append(" : " + client->name());
                    break;
                default: /* > if user is [MODS/ROOT] < */
                    QStringList info(client->getIpid());
                    if (!client->name().isEmpty()){ // get client(s) ooc-name..
                        info.append(client->name());
                        if (client->isVAuthenticated() && client->name().compare(client->m_moderator_name, Qt::CaseInsensitive) != 0) // add [VIP] name if it doesn't same like ooc-name..
                            info.append("(V: " + client->m_moderator_name + ")");
                    }
                    if (ld_state)
                        info[0].append(" (" + AOClient::calcutateHashid(client) + ")");
                    Entry.append("\n└─ [" + info.join(" | ") + "]");
                    break;
                }
                entries.append(Entry.join(""));
            }
        }
    }
    return entries;
}

int AOClient::genRand(int min, int max){
    return min > max ? min : QRandomGenerator::system()->bounded(min, max +1);
}
void AOClient::diceThrower(const int sides, const int dice, const int roll_modifier, const bool p_roll){
    if (sides < 1 || dice < 1 || sides > ConfigManager::diceMaxValue() || dice > ConfigManager::diceMaxDice())
        sendServerMessage("Dice or side number out of bounds.");
    else{
        QStringList results;
        for (int i = 1; i <= dice; i++){
            const int side_results = AOClient::genRand(1, sides);
            results.append(roll_modifier == 0 ? QString::number(side_results) : QString("%1(%2): %3").arg(QString::number(side_results), roll_modifier < 0 ? QString::number(roll_modifier) : "+" + QString::number(roll_modifier), QString::number(qMax(1, side_results + roll_modifier))));
        }
        p_roll ? sendServerMessage(QString("You rolled a %1d%2. Results: %3").arg(QString::number(dice), QString::number(sides), results.size() == 1 ? results[0] : "\n[" + results.join(", ") + "]")) : sendServerMessageArea(QString("%1 rolled a %2d%3. Results: %4").arg(QStringList({"[" + QString::number(clientId()) + "]", QString(character().isEmpty() ? name() : character())}).join(' '), QString::number(dice), QString::number(sides), results.size() == 1 ? results[0] : "\n[" + results.join(", ") + "]"));
    }
}

QString AOClient::getAreaTimer(int area_idx, int timer_idx)
{
    AreaData *l_area = server->getAreaById(area_idx);
    switch (timer_idx){
    case 0:
        return QString("Global timer is %1").arg(server->timer->isActive() ? ("at " + EpochToString(server->timer->remainingTimeAsDuration())) : "inactive.");
    case 1: case 2: case 3: case 4:
        return QString("Timer %1 is %2").arg(QString::number(timer_idx), l_area->timers().at(timer_idx - 1)->isActive() ? ("at " + EpochToString(l_area->timers().at(timer_idx - 1)->remainingTimeAsDuration())) : "inactive.");
    default:
        return "Invalid timer ID.";
    }
}

long long AOClient::CalendarParse(const QString &input, const bool toMS){
    QRegularExpression timePattern(R"((?:(\d+)y)?(?:(\d+)mo)?(?:(\d+)w)?(?:(\d+)d)?(?:(\d+)h)?(?:(\d+)m)?(?:(\d+)s)?)");
    QRegularExpressionMatch match = timePattern.match(input);

    if (!match.hasMatch() || input.isEmpty())
        return -1;

    auto current_date = QDateTime::currentDateTime();

    if (!match.captured(1).isEmpty()) // years
        current_date = current_date.addYears(match.captured(1).toInt());
    if (!match.captured(2).isEmpty()) // months
        current_date = current_date.addMonths(match.captured(2).toInt());
    if (!match.captured(3).isEmpty()) // weeks
        current_date = current_date.addDays(7 * match.captured(3).toInt());
    if (!match.captured(4).isEmpty()) // days
        current_date = current_date.addDays(match.captured(4).toInt());
    if (!match.captured(5).isEmpty()) // hours
        current_date = current_date.addSecs(3600 * match.captured(5).toLongLong());
    if (!match.captured(6).isEmpty()) // minutes
        current_date = current_date.addSecs(60 * match.captured(6).toLongLong());
    if (!match.captured(7).isEmpty()) // seconds
        current_date = current_date.addSecs(match.captured(7).toLongLong());

    return toMS ? QDateTime::currentDateTime().msecsTo(current_date) : QDateTime::currentDateTime().secsTo(current_date);
}
QDateTime AOClient::CalendarParseTo(const QString &input){
    QRegularExpression timePattern(R"((?:(\d+)y)?(?:(\d+)mo)?(?:(\d+)w)?(?:(\d+)d)?(?:(\d+)h)?(?:(\d+)m)?(?:(\d+)s)?)");
    QRegularExpressionMatch match = timePattern.match(input);

    if (!match.hasMatch() || input.isEmpty())
        return QDateTime();

    auto current_date = QDateTime::currentDateTime();

    if (!match.captured(1).isEmpty()) // years
        current_date = current_date.addYears(match.captured(1).toInt());
    if (!match.captured(2).isEmpty()) // months
        current_date = current_date.addMonths(match.captured(2).toInt());
    if (!match.captured(3).isEmpty()) // weeks
        current_date = current_date.addDays(7 * match.captured(3).toInt());
    if (!match.captured(4).isEmpty()) // days
        current_date = current_date.addDays(match.captured(4).toInt());
    if (!match.captured(5).isEmpty()) // hours
        current_date = current_date.addSecs(3600 * match.captured(5).toLongLong());
    if (!match.captured(6).isEmpty()) // minutes
        current_date = current_date.addSecs(60 * match.captured(6).toLongLong());
    if (!match.captured(7).isEmpty()) // seconds
        current_date = current_date.addSecs(match.captured(7).toLongLong());
    return current_date;
}

QString AOClient::EpochToString(const std::chrono::seconds input, const bool verbosity){
    using namespace std::chrono;

    if (input.count() <= 0)
        return verbosity ? QStringLiteral("0 sec") : QStringLiteral("Expired");

#if __cplusplus > 201703L
    const auto sec   = duration_cast<seconds>(input % minutes(1));
    const auto min   = duration_cast<minutes>(input % hours(1));
    const auto hour  = duration_cast<hours>(input % days(1));
    const auto day   = duration_cast<days>(input % weeks(1));
    const auto week  = duration_cast<weeks>(input % months(1));
    const auto month = duration_cast<months>(input % years(1));
    const auto year  = duration_cast<years>(input);
#else
    // Pre-C++20 helper durations
    using Years  = duration<int64_t, std::ratio<31556952>>;
    using Months = duration<int64_t, std::ratio<2629746>>;
    using Weeks  = duration<int64_t, std::ratio<604800>>;
    using Days   = duration<int64_t, std::ratio<86400>>;

    const auto sec   = duration_cast<seconds>(input % minutes(1));
    const auto min   = duration_cast<minutes>(input % hours(1));
    const auto hour  = duration_cast<hours>(input % Days(1));
    const auto day   = duration_cast<Days>(input % Weeks(1));
    const auto week  = duration_cast<Weeks>(input % Months(1));
    const auto month = duration_cast<Months>(input % Years(1));
    const auto year  = duration_cast<Years>(input);
#endif

    // Verbose form: "1 Year 2 Month 3 Week ..."
    if (verbosity) {
        QStringList parts;
        if (year.count()  > 0) parts << QString::number(year.count())  + (year.count()  == 1 ? " Year"  : " Years");
        if (month.count() > 0) parts << QString::number(month.count()) + (month.count() == 1 ? " Month" : " Months");
        if (week.count()  > 0) parts << QString::number(week.count())  + (week.count()  == 1 ? " Week"  : " Weeks");
        if (day.count()   > 0) parts << QString::number(day.count())   + (day.count()   == 1 ? " Day"   : " Days");
        if (hour.count()  > 0) parts << QString::number(hour.count())  + (hour.count()  == 1 ? " Hour"  : " Hours");
        if (min.count()   > 0) parts << QString::number(min.count())   + (min.count()   == 1 ? " Minute": " Minutes");
        if (sec.count()   > 0) parts << QString::number(sec.count())   + (sec.count()   == 1 ? " sec"   : " secs");
        return parts.join(' ');
    }

    // compact colon-separated form, choose highest non-zero unit as head..
    QStringList compact;
    if (year.count()  > 0)
        compact = QStringList{QString::number(year.count()), QString::number(month.count()), QString::number(week.count()), QString::number(day.count()), QString::number(hour.count()), QString::number(min.count()), QString::number(sec.count())};
    else if (month.count() > 0)
        compact = QStringList{QString::number(month.count()), QString::number(week.count()),QString::number(day.count()), QString::number(hour.count()), QString::number(min.count()), QString::number(sec.count())};
    else if (week.count()  > 0)
        compact = QStringList{QString::number(week.count()), QString::number(day.count()), QString::number(hour.count()), QString::number(min.count()), QString::number(sec.count())};
    else if (day.count()   > 0)
        compact = QStringList{QString::number(day.count()), QString::number(hour.count()), QString::number(min.count()), QString::number(sec.count()) };
    else if (hour.count()  > 0)
        compact = QStringList{QString::number(hour.count()), QString::number(min.count()), QString::number(sec.count())};
    else if (min.count()   > 0)
        compact = QStringList{QString::number(min.count()), QString::number(sec.count())};
    else
        compact = QStringList{QString::number(sec.count())};


    return compact.join(':');
}

QString AOClient::EpochToString(const std::chrono::milliseconds minput, const bool verbosity){
    using namespace std::chrono;

    if (minput.count() <= 0)
        return verbosity ? QStringLiteral("0 msec") : QStringLiteral("Expired");

    // Millisecond -> second -> minute -> hour -> day -> week -> month -> year breakdown
    const auto mill = duration_cast<milliseconds>(minput % seconds(1));
    const auto sec  = duration_cast<seconds>(minput % minutes(1));
    const auto min  = duration_cast<minutes>(minput % hours(1));

#if __cplusplus > 201703L
    const auto hour  = duration_cast<hours>(minput % days(1));
    const auto day   = duration_cast<days>(minput % weeks(1));
    const auto week  = duration_cast<weeks>(minput % months(1)); // C++20 months
    const auto month = duration_cast<months>(minput % years(1)); // C++20 years/months
    const auto year  = duration_cast<years>(minput);
#else
    // Pre-C++20 helper durations (approx values)
    using Years  = duration<int64_t, std::ratio<31556952>>;
    using Months = duration<int64_t, std::ratio<2629746>>;
    using Weeks  = duration<int64_t, std::ratio<604800>>;
    using Days   = duration<int64_t, std::ratio<86400>>;

    const auto hour  = duration_cast<hours>(minput % Days(1));
    const auto day   = duration_cast<Days>(minput % Weeks(1));
    const auto week  = duration_cast<Weeks>(minput % Months(1));
    const auto month = duration_cast<Months>(minput % Years(1));
    const auto year  = duration_cast<Years>(minput);
#endif

    if (verbosity) {
        QStringList parts;
        if (year.count()  > 0) parts << QString::number(year.count())  + (year.count()  == 1 ? " Year"   : " Years");
        if (month.count() > 0) parts << QString::number(month.count()) + (month.count() == 1 ? " Month"  : " Months");
        if (week.count()  > 0) parts << QString::number(week.count())  + (week.count()  == 1 ? " Week"   : " Weeks");
        if (day.count()   > 0) parts << QString::number(day.count())   + (day.count()   == 1 ? " Day"    : " Days");
        if (hour.count()  > 0) parts << QString::number(hour.count())  + (hour.count()  == 1 ? " Hour"   : " Hours");
        if (min.count()   > 0) parts << QString::number(min.count())   + (min.count()   == 1 ? " Minute" : " Minutes");
        if (sec.count()   > 0) parts << QString::number(sec.count())   + (sec.count()   == 1 ? " sec"    : " secs");
        // Always show milliseconds in verbose form (even if zero)
        parts << QString::number(mill.count()) + (mill.count() == 1 ? " msec" : " msecs");
        return parts.join(' ');
    }

    // compact colon-separated form: choose highest non-zero unit as head...
    QStringList compact;
    if (year.count()  > 0)
        compact = QStringList{QString::number(year.count()), QString::number(month.count()), QString::number(week.count()), QString::number(day.count()), QString::number(hour.count()), QString::number(min.count()), QString::number(sec.count())};
    else if (month.count() > 0)
        compact = QStringList{QString::number(month.count()), QString::number(week.count()),QString::number(day.count()), QString::number(hour.count()), QString::number(min.count()), QString::number(sec.count())};
    else if (week.count()  > 0)
        compact = QStringList{QString::number(week.count()), QString::number(day.count()), QString::number(hour.count()), QString::number(min.count()), QString::number(sec.count())};
    else if (day.count()   > 0)
        compact = QStringList{QString::number(day.count()), QString::number(hour.count()), QString::number(min.count()), QString::number(sec.count()) };
    else if (hour.count() > 0)
        compact = QStringList{QString::number(hour.count()), QString::number(min.count()), QString::number(sec.count())};
    else if (min.count() > 0)
        compact = QStringList{QString::number(min.count()), QString::number(sec.count())};
    else if (sec.count() > 0)
        compact = QStringList{QString::number(sec.count())};
    else
        compact = QStringList() << QString::number(mill.count());

    return compact.join(':');
}


QString AOClient::getReprimand(bool f_positive){
    const QStringList words = f_positive ? ConfigManager::praiseList() : ConfigManager::reprimandsList();
    return words.isEmpty() ? "" : words[genRand(0, words.size() -1)];
}

bool isMixedCasehelper(const QString &s){
    return std::any_of(s.begin(), s.end(), [](QChar c){return c.isUpper();}) && std::any_of(s.begin(), s.end(), [](QChar c){return c.isLower();});
}

bool AOClient::checkPasswordRequirements(QString f_username, QString f_password){
    const QString l_decoded_password = decodeMessage(f_password);
    if (ConfigManager::passwordRequirements()){
        const QPair<int, int> passwordboundLength({qMax(0, ConfigManager::passwordMinLength()), qMax(0, ConfigManager::passwordMaxLength())});

        if (passwordboundLength.first > l_decoded_password.length() || (passwordboundLength.second >= 1 && passwordboundLength.second < l_decoded_password.length()))
            return false;

        if (ConfigManager::passwordRequireMixCase() && !isMixedCasehelper(l_decoded_password))
            return false;
        if (ConfigManager::passwordRequireNumbers() && !QRegularExpression("[0123456789]").match(l_decoded_password).hasMatch())
            return false;
        if (ConfigManager::passwordRequireSpecialCharacters() && !QRegularExpression("[^A-Za-z0-9]").match(l_decoded_password).hasMatch())
            return false;
        if (!ConfigManager::passwordCanContainUsername() && l_decoded_password.contains(f_username, Qt::CaseInsensitive))
            return false;

    }
    return true;
}

void AOClient::sendNotice(QString f_notice, bool f_global)
{
    QString l_message = "A moderator sent this ";
    if (f_global)
        l_message += "server-wide ";
    l_message += "notice:\n\n" + f_notice;
    sendServerMessageArea(l_message);
    AOPacket *l_packet = PacketFactory::createPacket("BB", {l_message});
    if (f_global)
        server->broadcast(l_packet);
    else
        server->broadcast(l_packet, areaId());
}
